#pragma once

namespace LightVolumes
{
    inline void** DeviceSlot = nullptr;
    inline IDirect3DDevice9* Device{ nullptr };
    inline IUnknown* GameDevice{ nullptr };
    inline IDirect3DTexture9* Depth{ nullptr };
    inline IDirect3DSurface9* DepthSurface{ nullptr };
    inline IDirect3DSurface9* OriginalDepth{ nullptr };
    inline IDirect3DPixelShader9* Shader{ nullptr };
    inline bool Captured{ false };
    inline bool Rendering{ false };
    static safetyhook::InlineHook ClearHook, SetDepthHook, GetDepthHook, ResetHook;
    inline unsigned int DepthFrames{ 0 };
    inline unsigned int LastLightCount{ 0 };
    inline float FarScale{ 1.0f };
    inline bool CompactPools{ false };

    inline float FarWeight(float distance)
    {
        const float t{ std::clamp((distance - 100.0f) / 300.0f, 0.0f, 1.0f) };
        return t * t * (3.0f - 2.0f * t) * FarScale;
    }

    inline float Radius(float size, float distance)
    {
        if (CompactPools)
            return size * (1.0f + 0.65f * FarWeight(distance));
        const float current{ std::clamp(distance / 150.0f, 1.0f, 3.0f) };
        const float distant{ std::clamp(distance / 90.0f, 1.0f, 4.5f) };
        return size * (current + (distant - current) * FarWeight(distance));
    }

    inline float Colour(float value, float distance)
    {
        const float weight{ FarWeight(distance) };
        const float distant{ value * 3.0f };
        return value + (distant / (1.0f + distant / 96.0f) - value) * weight;
    }
    inline unsigned int ClearCalls{ 0 };
    inline unsigned int MainClears{ 0 };
    inline unsigned int LastMSAA{ 0 };
    inline unsigned int DepthFormat{ 0 };
    inline unsigned int Support{ 0 };
    inline unsigned int ClearRectCount{ 0 };
    inline unsigned int HookFlags{ 0 };

    template<typename T> void Release(T*& object)
    {
        if (object)
            object->Release();
        object = nullptr;
    }

    inline bool MainTarget(IDirect3DDevice9* device)
    {
        IDirect3DSurface9* target{ nullptr };
        IDirect3DSurface9* back{ nullptr };
        device->GetRenderTarget(0, &target);
        device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back);
        const bool result{ target && target == back };
        Release(target);
        Release(back);
        return result;
    }

    inline void DropDepth()
    {
        Captured = false;
        Release(DepthSurface);
        Release(Depth);
        Release(OriginalDepth);
        Release(Shader);
    }

    inline HRESULT WINAPI SetDepth(IDirect3DDevice9* device, IDirect3DSurface9* surface)
    {
        if (device == Device && surface == OriginalDepth && DepthSurface && MainTarget(device))
            surface = DepthSurface;
        return SetDepthHook.stdcall<HRESULT>(device, surface);
    }

    inline HRESULT WINAPI GetDepth(IDirect3DDevice9* device, IDirect3DSurface9** surface)
    {
        const auto result{ GetDepthHook.stdcall<HRESULT>(device, surface) };
        if (SUCCEEDED(result) && device == Device && !Rendering && *surface == DepthSurface && OriginalDepth)
        {
            (*surface)->Release();
            *surface = OriginalDepth;
            OriginalDepth->AddRef();
        }
        return result;
    }

    inline HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device == Device)
        {
            if (OriginalDepth)
                SetDepthHook.stdcall<HRESULT>(device, OriginalDepth);
            DropDepth();
        }
        return ResetHook.stdcall<HRESULT>(device, parameters);
    }

    inline HRESULT WINAPI Clear(IDirect3DDevice9* device, DWORD count, const D3DRECT* rectangles,
        DWORD flags, D3DCOLOR colour, float z, DWORD stencil)
    {
        if (device == Device)
            ++ClearCalls;
        if (device == Device && (flags & D3DCLEAR_ZBUFFER) && MainTarget(device))
        {
            ++MainClears;
            ClearRectCount = count;
            Captured = false;
            IDirect3DSurface9* current{ nullptr };
            GetDepthHook.stdcall<HRESULT>(device, &current);
            D3DSURFACE_DESC description{};
            if (current)
                current->GetDesc(&description);
            LastMSAA = description.MultiSampleType;
            DepthFormat = description.Format;
            if (current && current != DepthSurface && current != OriginalDepth)
            {
                DropDepth();
                if (description.MultiSampleType == D3DMULTISAMPLE_NONE)
                {
                    IDirect3D9* d3d{ nullptr };
                    D3DDEVICE_CREATION_PARAMETERS creation{};
                    D3DDISPLAYMODE mode{};
                    device->GetDirect3D(&d3d);
                    device->GetCreationParameters(&creation);
                    d3d->GetAdapterDisplayMode(creation.AdapterOrdinal, &mode);
                    const auto format{ static_cast<D3DFORMAT>(MAKEFOURCC('I', 'N', 'T', 'Z')) };
                    const bool supported = SUCCEEDED(d3d->CheckDeviceFormat(creation.AdapterOrdinal, creation.DeviceType,
                        mode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, format));
                    Release(d3d);
                    Support = supported;
                    if (supported && SUCCEEDED(device->CreateTexture(description.Width, description.Height, 1,
                        D3DUSAGE_DEPTHSTENCIL, format, D3DPOOL_DEFAULT, &Depth, nullptr)))
                    {
                        Depth->GetSurfaceLevel(0, &DepthSurface);
                        OriginalDepth = current;
                        OriginalDepth->AddRef();
                    }
                }
            }
            if (DepthSurface && (current == OriginalDepth || current == DepthSurface))
            {
                Captured = SUCCEEDED(SetDepthHook.stdcall<HRESULT>(device, DepthSurface));
            }
            Release(current);
        }
        return ClearHook.stdcall<HRESULT>(device, count, rectangles, flags, colour, z, stencil);
    }

    inline void Prepare()
    {
        if (!DeviceSlot)
            return;
        auto* unknown{ static_cast<IUnknown*>(*DeviceSlot) };
        if (!unknown || unknown == GameDevice)
            return;
        if (Device)
        {
            if (OriginalDepth)
                SetDepthHook.stdcall<HRESULT>(Device, OriginalDepth);
            DropDepth();
            ClearHook.reset();
            SetDepthHook.reset();
            GetDepthHook.reset();
            ResetHook.reset();
            Device = nullptr;
        }
        GameDevice = unknown;
        IDirect3DDevice9* device{ nullptr };
        if (FAILED(unknown->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&device))))
            return;
        Device = device;
        // The game owns the device; keeping a reference would prevent its normal shutdown.
        device->Release();
        auto** table{ *reinterpret_cast<void***>(device) };
        ClearHook = safetyhook::create_inline(table[43], Clear);
        SetDepthHook = safetyhook::create_inline(table[39], SetDepth);
        GetDepthHook = safetyhook::create_inline(table[40], GetDepth);
        ResetHook = safetyhook::create_inline(table[16], Reset);
        HookFlags = (ClearHook ? 1 : 0) | (SetDepthHook ? 2 : 0) | (GetDepthHook ? 4 : 0) | (ResetHook ? 8 : 0);
        if (HookFlags != 15)
        {
            ClearHook.reset();
            SetDepthHook.reset();
            GetDepthHook.reset();
            ResetHook.reset();
            Device = nullptr;
        }
    }

    struct Vertex
    {
        float x, y, z, rhw;
        DWORD colour;
        float u, v;
        float lx, ly, lz, inverseRadius;
        float inverseHeight, weight, axisX, axisY;
    };

    inline D3DMATRIX Multiply(const D3DMATRIX& a, const D3DMATRIX& b)
    {
        D3DMATRIX result{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result.m[i][j] += a.m[i][k] * b.m[k][j];
        return result;
    }

    inline bool Inverse(const D3DMATRIX& matrix, D3DMATRIX& inverse)
    {
        double rows[4][8]{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
            {
                rows[i][j] = matrix.m[i][j];
                rows[i][j + 4] = i == j ? 1.0 : 0.0;
            }
        for (int column = 0; column < 4; ++column)
        {
            int pivot{ column };
            for (int i = column + 1; i < 4; ++i)
                if (std::abs(rows[i][column]) > std::abs(rows[pivot][column]))
                    pivot = i;
            if (std::abs(rows[pivot][column]) < 1.0e-12)
                return false;
            for (int j = 0; j < 8; ++j)
                std::swap(rows[column][j], rows[pivot][j]);
            const double scale{ rows[column][column] };
            for (int j = 0; j < 8; ++j)
                rows[column][j] /= scale;
            for (int i = 0; i < 4; ++i)
                if (i != column)
                {
                    const double factor{ rows[i][column] };
                    for (int j = 0; j < 8; ++j)
                        rows[i][j] -= factor * rows[column][j];
                }
        }
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                inverse.m[i][j] = static_cast<float>(rows[i][j + 4]);
        return true;
    }

    template<typename Pools> bool RenderDepth(const Pools& pools, const RwCamera* camera)
    {
        if (!Device || !Captured || !DepthSurface || !MainTarget(Device))
            return false;
        IDirect3DSurface9* current{ nullptr };
        GetDepthHook.stdcall<HRESULT>(Device, &current);
        const bool matching{ current == DepthSurface };
        Release(current);
        if (!matching)
            return false;
        if (!Shader && FAILED(Device->CreatePixelShader(reinterpret_cast<const DWORD*>(LightVolumesShader), &Shader)))
            return false;
        D3DMATRIX view{}, projection{}, inverse{};
        Device->GetTransform(D3DTS_VIEW, &view);
        Device->GetTransform(D3DTS_PROJECTION, &projection);
        const auto vp{ Multiply(view, projection) };
        if (!Inverse(vp, inverse))
            return false;
        IDirect3DStateBlock9* state{ nullptr };
        if (FAILED(Device->CreateStateBlock(D3DSBT_ALL, &state)))
            return false;
        D3DVIEWPORT9 viewport{};
        Device->GetViewport(&viewport);
        Rendering = true;
        // Unbind depth while sampling it; the original surface is never read or cleared here.
        SetDepthHook.stdcall<HRESULT>(Device, nullptr);
        Device->SetPixelShader(Shader);
        Device->SetVertexShader(nullptr);
        Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX3 | D3DFVF_TEXCOORDSIZE4(1) | D3DFVF_TEXCOORDSIZE4(2));
        Device->SetTexture(0, Depth);
        Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        Device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, 0);
        Device->SetRenderState(D3DRS_ZENABLE, FALSE);
        Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        Device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
        Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        Device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        Device->SetRenderState(D3DRS_COLORWRITEENABLE, 7);
        Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        Device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        Device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
        Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        Device->SetPixelShaderConstantF(0, &inverse.m[0][0], 4);
        const float fog[] = { 1.0f / (std::max)(camera->farPlane - camera->fogPlane, 1.0f), camera->farPlane,
            projection._33, projection._43 };
        Device->SetPixelShaderConstantF(4, fog, 1);
        const float texel[] = { 1.0f / viewport.Width, 1.0f / viewport.Height, 0.0f, 0.0f };
        Device->SetPixelShaderConstantF(5, texel, 1);
        static std::vector<Vertex> vertices;
        vertices.clear();
        for (const auto& pool : pools)
        {
            const auto& lamp{ *pool.lamp };
            if (!std::isfinite(lamp.fPoolBaseZ))
                continue;
            const float radius{ Radius(lamp.pool.size, pool.distance) };
            const float height{ 3.0f };
            const CVector centre{ lamp.vecPos.x, lamp.vecPos.y, lamp.fPoolBaseZ };
            const float viewDepth{ centre.x * view._13 + centre.y * view._23 + centre.z * view._33 + view._43 };
            const float bound{ (std::max)(radius, height) };
            if (viewDepth + bound <= camera->nearPlane || viewDepth - bound >= camera->farPlane)
                continue;
            float left{ 1.0f };
            float right{ -1.0f };
            float top{ 1.0f };
            float bottom{ -1.0f };
            bool clipped{ false };
            for (int corner = 0; corner < 8; ++corner)
            {
                const float p[] = { centre.x + (corner & 1 ? radius : -radius), centre.y + (corner & 2 ? radius : -radius),
                    centre.z + (corner & 4 ? height : -height), 1.0f };
                float clip[4]{};
                for (int j = 0; j < 4; ++j)
                    for (int k = 0; k < 4; ++k)
                        clip[j] += p[k] * vp.m[k][j];
                if (clip[3] <= 0.1f)
                {
                    clipped = true;
                    break;
                }
                left = (std::min)(left, clip[0] / clip[3]);
                right = (std::max)(right, clip[0] / clip[3]);
                top = (std::min)(top, -clip[1] / clip[3]);
                bottom = (std::max)(bottom, -clip[1] / clip[3]);
            }
            if (clipped)
            {
                left = top = -1.0f;
                right = bottom = 1.0f;
            }
            left = std::clamp(left, -1.0f, 1.0f);
            right = std::clamp(right, -1.0f, 1.0f);
            top = std::clamp(top, -1.0f, 1.0f);
            bottom = std::clamp(bottom, -1.0f, 1.0f);
            if (left >= right || top >= bottom)
                continue;
            const auto channel{ [](float value) { return static_cast<DWORD>(std::clamp(value, 0.0f, 255.0f)); } };
            const DWORD colour{ channel(pool.r) << 16 | channel(pool.g) << 8 | channel(pool.b) };
            for (int index : { 0, 1, 2, 2, 1, 3 })
            {
                const float u{ ((index & 1 ? right : left) + 1.0f) * 0.5f };
                const float v{ ((index & 2 ? bottom : top) + 1.0f) * 0.5f };
                vertices.push_back({ viewport.X + u * viewport.Width - 0.5f, viewport.Y + v * viewport.Height - 0.5f,
                    0.0f, 1.0f, colour, u, v, centre.x, centre.y, centre.z, 1.0f / radius, 1.0f / height, CompactPools ? 0.0f : FarWeight(pool.distance), pool.axis.x, pool.axis.y });
            }
        }
        for (size_t offset = 0; offset < vertices.size(); offset += 6 * 256)
        {
            const auto count{ (std::min)(vertices.size() - offset, size_t(6 * 256)) };
            Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, static_cast<UINT>(count / 3), vertices.data() + offset, sizeof(Vertex));
        }
        Device->SetTexture(0, nullptr);
        SetDepthHook.stdcall<HRESULT>(Device, DepthSurface);
        state->Apply();
        state->Release();
        Rendering = false;
        ++DepthFrames;
        LastLightCount = static_cast<unsigned int>(vertices.size() / 6);
        return true;
    }
}
