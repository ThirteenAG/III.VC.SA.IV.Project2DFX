#pragma once

#include <cmath>

namespace ViewDistance
{
    static float current = 0.0f;
    static float sunCore = 2.7335f;
    static float sunCorona = 6.0f;
    static void(__thiscall* processCamera)(CCamera*) = nullptr;
    static void(__cdecl* updateTimecycle)() = nullptr;
    static float savedFar = 0.0f, savedFog = 0.0f;
    static float appliedFar = 0.0f, appliedFog = 0.0f;
    static bool modified = false;

    static safetyhook::MidHook waterRangeHook;
    static uintptr_t waterRangeOffset{};

    void InstallWaterRange(uintptr_t address, uintptr_t offset)
    {
        const uint8_t expected[] = { 0xD9, 0x44, 0x24, static_cast<uint8_t>(offset), 0xD8, 0xC8 };
        if (memcmp(reinterpret_cast<void*>(address), expected, sizeof(expected)) != 0)
            return;
        waterRangeOffset = offset;
        waterRangeHook = safetyhook::create_mid(address, [](SafetyHookContext& context)
        {
            const auto* camera{ Scene->m_pRwCamera };
            if (!camera || !ExteriorView() || fViewDistance <= 0.0f)
                return;
            const float x{ camera->viewWindow.x };
            const float y{ camera->viewWindow.y };
            const float radius{ camera->farPlane * std::sqrt(1.0f + x * x + y * y) + 128.0f };
            auto& range{ *reinterpret_cast<float*>(context.esp + waterRangeOffset) };
            if (std::isfinite(radius))
                range = std::max(range, radius);
        });
    }

    void InstallWater()
    {
        const auto camera{ reinterpret_cast<uintptr_t>(TheCamera.operator->()) };
        if (camera == 0x7E4688)
            InstallWaterRange(0x5C1845, 0x28);
        else if (camera == 0x6FACF8)
            InstallWaterRange(0x555645, 0x68);
    }

    void Restore()
    {
        if (!modified)
            return;
        if (CTimeCycle::m_fCurrentFarClip == appliedFar)
            CTimeCycle::m_fCurrentFarClip = savedFar;
        if (CTimeCycle::m_fCurrentFogStart == appliedFog)
            CTimeCycle::m_fCurrentFogStart = savedFog;
        modified = false;
    }

    void Apply()
    {
        fFarClipMultiplier = 1.0f;
        const float nativeFar = CTimeCycle::m_fCurrentFarClip;
        const float nativeFog = CTimeCycle::m_fCurrentFogStart;
        if (!ExteriorView() || fViewDistance == 0.0f || !std::isfinite(nativeFar) || nativeFar <= 1.0f || !std::isfinite(nativeFog))
        {
            current = 0.0f;
        }
        else
        {
            if (current == 0.0f)
                current = fViewDistance;
            const float dt = std::clamp(CTimer::GetTimeStepInSeconds(), 0.0f, 0.1f);
            current += (fViewDistance - current) * (1.0f - std::exp(-3.0f * dt));
            if (std::abs(current - fViewDistance) < 0.1f)
                current = fViewDistance;

            savedFar = nativeFar;
            savedFog = nativeFog;
            appliedFar = nativeFar + current;
            appliedFog = nativeFog + current;
            modified = true;

            // Shift the native fog ramp without changing its width or the weather blend.
            fFarClipMultiplier = (nativeFar + current) / nativeFar;
            CTimeCycle::m_fCurrentFogStart = appliedFog;
            CTimeCycle::m_fCurrentFarClip = appliedFar;
        }
        sunCore = 2.7335f * fFarClipMultiplier;
        sunCorona = 6.0f * fFarClipMultiplier;
    }

    void __cdecl UpdateTimecycle()
    {
        // Keep native inputs independent of the previous addition.
        Restore();
        updateTimecycle();
        Apply();
    }

    void InstallTimecycle(void* call)
    {
        updateTimecycle = reinterpret_cast<decltype(updateTimecycle)>(injector::GetBranchDestination(call).as_int());
        injector::MakeCALL(call, UpdateTimecycle, true);
    }

    void __fastcall ProcessCamera(CCamera* camera, void*)
    {
        processCamera(camera);
        if (ExteriorView())
            camera->LODDistMultiplier *= fDrawDistance;
    }

    void InstallCamera(void* call)
    {
        InstallWater();
        processCamera = reinterpret_cast<decltype(processCamera)>(injector::GetBranchDestination(call).as_int());
        injector::MakeCALL(call, ProcessCamera, true);
    }
}
