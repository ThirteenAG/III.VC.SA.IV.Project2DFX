#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <psapi.h>

namespace ObjectDistance
{
    struct Range
    {
        float limit;
        float width;
    };

    struct IplBounds
    {
        float minX, maxY, maxX, minY;
    };

    struct IplDefinition
    {
        IplBounds bounds;
        char name[18];
        int16_t firstBuilding, lastBuilding, firstDummy, lastDummy, staticIndex;
        uint8_t interior, loaded, requested, disableDynamicStreaming, ignoreWhenDeleted, large;
    };

    // CPool header.
    struct IplPool
    {
        IplDefinition* objects;
        uint8_t* flags;
        int size;
    };

    struct Operand
    {
        uintptr_t address;
        uint16_t opcode;
        uintptr_t original;
        float* replacement;
    };

    struct CreationCheck
    {
        uintptr_t address;
        uint16_t original;
    };

    static float scale{ 1.0f };
    static float triangleSquared{ 10000.0f };
    static float cacheRadius{ 340.0f };
    static float cacheSquared{ 115600.0f };
    static float grassFar{ 60.0f };
    static float grassFadeWidth{ 20.0f };
    static float grassFadeInverse{ 0.05f };
    static bool proceduralScalingActive{};
    static SafetyHookInline alphaHook{}, proceduralVisibilityHook{}, surfaceHook{};
    static SafetyHookMid normalVisibilityHook{}, iplRequestHook{};

    static const CreationCheck creationChecks[]
    {
        { 0x5A332A, 0x777D },
        { 0x5A3355, 0x4C73 }
    };

    static const Operand proceduralScalingOperands[]
    {
        { 0x5DC0D2, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC105, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC132, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC15F, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC18C, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC1B5, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC1DE, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCB8A, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCBBF, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCBEC, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCC19, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCC42, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCC6B, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DCC94, 0x1DD8, 0x859AA4, &triangleSquared },
        { 0x5DC56B, 0x1DD8, 0x86BF20, &cacheSquared },
        { 0x5DC82E, 0x1DD8, 0x86BF20, &cacheSquared },
        { 0x5DC5B5, 0x25D8, 0x85F074, &cacheRadius },
        { 0x5DC5C6, 0x25D8, 0x85F074, &cacheRadius },
        { 0x5DC5DB, 0x05D8, 0x85F074, &cacheRadius },
        { 0x5DC5E9, 0x05D8, 0x85F074, &cacheRadius },
    };

    static const Operand grassRampOperands[]
    {
        { 0x5DAD78, 0x158B, 0x8D132C, &grassFar },
        { 0x5DAD84, 0x05D9, 0x8D132C, &grassFar },
        { 0x5DAD98, 0x05D8, 0x858BA4, &grassFadeWidth },
        { 0x5DAE05, 0x0DD8, 0x858C28, &grassFadeInverse }
    };

    bool UsesNormalCap(const CEntity* entity)
    {
        return entity && !entity->bIsBIGBuilding && !entity->bIsProcObject && !entity->bDontStream;
    }

    bool UsesProceduralRange(const CEntity* entity)
    {
        return entity && entity->bIsProcObject;
    }

    inline bool ValidIplBounds(const IplBounds& b)
    {
        return std::isfinite(b.minX) && std::isfinite(b.maxY) && std::isfinite(b.maxX)
            && std::isfinite(b.minY) && b.minX <= b.maxX && b.minY <= b.maxY;
    }

    inline bool ContainsIplPosition(const IplBounds& b, float x, float y, float extra)
    {
        // The request loop validates bounds once before checking both positions.
        return std::isfinite(x) && std::isfinite(y)
            && x >= b.minX - extra && x <= b.maxX + extra
            && y >= b.minY - extra && y <= b.maxY + extra;
    }

    float CalculateProceduralPoolScale()
    {
        using GetLimit = bool(__cdecl*)(const char*, int*, int*, int*);
        static GetLimit getLimit{};
        static ULONGLONG nextLookup{};
        if (!getLimit && GetTickCount64() >= nextLookup)
        {
            nextLookup = GetTickCount64() + 1000;
            HMODULE modules[1024];
            DWORD needed{};
            if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed))
            {
                for (DWORD i{}; i < needed / sizeof(HMODULE) && i < 1024; ++i)
                {
                    getLimit = reinterpret_cast<GetLimit>(GetProcAddress(modules[i], "OpenLA_GetLimit"));
                    if (getLimit)
                        break;
                }
            }
        }
        if (!getLimit)
            return 1.0f;

        float result{ 3.0f };
        for (const auto& [name, vanilla] : { std::pair{ "ProcObjects", 512 },
            std::pair{ "PlantTriangles", 256 }, std::pair{ "PlantCollisionEntries", 40 } })
        {
            int capacity{};
            if (!getLimit(name, nullptr, &capacity, nullptr) || capacity <= vanilla)
                return 1.0f;
            // Ground coverage grows with the square of the radius.
            result = (std::min)(result, std::sqrt(static_cast<float>(capacity) / vanilla));
        }
        return result;
    }

    float ProceduralDrawDistance()
    {
        const int quality{ *reinterpret_cast<const int*>(0xA9AE54) };
        return quality >= 2 ? grassFar : grassFar * *reinterpret_cast<const float*>(0x858B8C);
    }

    inline int CalculateFadeAlpha(Range range, float baseFade, float distance, unsigned char modelAlpha)
    {
        const float fade = std::clamp((range.limit + baseFade - distance) / range.width, 0.0f, 1.0f);
        return static_cast<int>(modelAlpha * fade);
    }

    // Called only after UsesNormalCap accepts the entity.
    Range NormalObjectRange(const CBaseModelInfo* model, const CEntity* entity)
    {
        const float modelDraw = model->m_fDrawDistance;
        const float cameraMultiplier = *reinterpret_cast<const float*>(0xB6F118);
        const float farRadius = *reinterpret_cast<const float*>(0xB76848)
            + model->m_pColModel->m_Sphere.sphere.m_fRadius;
        Range range{ std::min({modelDraw * cameraMultiplier, farRadius, fMaxDrawDistanceForNormalObjects}),
            *reinterpret_cast<const float*>(0x858BA4) };
        if (!entity->m_pLod)
        {
            const float draw = std::min(modelDraw, range.limit);
            if (draw > *reinterpret_cast<const float*>(0x858A28))
                range.width = draw * *reinterpret_cast<const float*>(0x863E0C)
                    + *reinterpret_cast<const float*>(0x85862C);
        }
        return range;
    }

    inline float CalculateIplExtraPadding(float cap, float fade, bool large, bool loaded)
    {
        if (!std::isfinite(cap) || !std::isfinite(fade) || cap <= 300.0f)
            return 0.0f;
        // CIplStore stores already padded bounds, including when restored from
        // CColAccel. Request before the fade end; release farther away than request.
        constexpr float requestLead = 100.0f;
        constexpr float retentionMargin = 50.0f;
        const float nativePadding = large ? 350.0f : 200.0f;
        return std::max(0.0f, cap + std::max(0.0f, fade) + requestLead
            + (loaded ? retentionMargin : 0.0f) - nativePadding);
    }

    int __fastcall AddProceduralObjects(float* surface, void*, void* triangle)
    {
        const float distanceSquared{ surface[4] };
        // No near cutoff, same as grass; ignore procobj.dat MINDIST.
        surface[4] = 0.0f;
        const int result{ surfaceHook.unsafe_thiscall<int>(surface, triangle) };
        surface[4] = distanceSquared;
        return result;
    }

    int __cdecl SetupProceduralVisibility(CEntity* entity, float* distance)
    {
        if (!UsesProceduralRange(entity))
            return proceduralVisibilityHook.unsafe_ccall<int>(entity, distance);
        const int modelIndex{ entity->GetModelIndex() };
        auto** models{ CModelInfo::pp_modelInfoPtrs ? *CModelInfo::pp_modelInfoPtrs : nullptr };
        auto* model = models && modelIndex >= 0 ? models[modelIndex] : nullptr;
        const float lod{ TheCamera->LODDistMultiplier };
        if (!model || lod <= 0.0f)
            return proceduralVisibilityHook.unsafe_ccall<int>(entity, distance);
        const float fitted{ ProceduralDrawDistance() / lod };
        if (fitted <= 0.0f)
            return proceduralVisibilityHook.unsafe_ccall<int>(entity, distance);
        const float saved{ model->m_fDrawDistance };
        model->m_fDrawDistance = fitted;
        const int result{ proceduralVisibilityHook.unsafe_ccall<int>(entity, distance) };
        model->m_fDrawDistance = saved;
        return result;
    }

    int __cdecl CalculateObjectAlpha(CBaseModelInfo* model, CEntity* entity, float distance)
    {
        if (UsesNormalCap(entity))
            return CalculateFadeAlpha(NormalObjectRange(model, entity), *reinterpret_cast<const float*>(0x858BA4),
                distance, static_cast<unsigned char>(model->m_nAlpha));
        if (UsesProceduralRange(entity))
        {
            const float lod{ TheCamera->LODDistMultiplier };
            if (!model || lod <= 0.0f)
                return alphaHook.unsafe_ccall<int>(model, entity, distance);
            const float fitted{ ProceduralDrawDistance() / lod };
            if (fitted <= 0.0f)
                return alphaHook.unsafe_ccall<int>(model, entity, distance);
            const float saved{ model->m_fDrawDistance };
            model->m_fDrawDistance = fitted;
            const int result{ alphaHook.unsafe_ccall<int>(model, entity, distance) };
            model->m_fDrawDistance = saved;
            return result;
        }
        return alphaHook.unsafe_ccall<int>(model, entity, distance);
    }

    void SetupNormalObjectVisibility(SafetyHookContext& ctx)
    {
        const auto* entity = reinterpret_cast<const CEntity*>(ctx.esi);
        if (!UsesNormalCap(entity))
            return;
        const auto range = NormalObjectRange(reinterpret_cast<const CBaseModelInfo*>(ctx.edi), entity);
        *reinterpret_cast<float*>(ctx.esp + 0x14) = range.limit;
        *reinterpret_cast<float*>(ctx.esp + 0x10) = range.width;
    }

    void ExtendIplRequests(const CVector& streamingPosition)
    {
        const auto* pool = *reinterpret_cast<IplPool**>(0x8E3FB0);
        if (!pool || !pool->objects || !pool->flags)
            return;
        const CVector camera = TheCamera->GetCoords();
        const float fade = *reinterpret_cast<const float*>(0x858BA4);
        const bool exterior = CGame::currArea == 0 && CEntryExitManager::ms_exitEnterState == 0;
        for (int slot = 1; slot < pool->size; ++slot)
        {
            if (pool->flags[slot] & 0x80)
                continue;
            auto* def = &pool->objects[slot];
            const bool eligible = exterior && !def->interior && !def->disableDynamicStreaming
                && !def->ignoreWhenDeleted && ValidIplBounds(def->bounds)
                && fMaxDrawDistanceForNormalObjects > 300.0f;
            if (eligible)
            {
                const float extra = CalculateIplExtraPadding(fMaxDrawDistanceForNormalObjects, fade, def->large != 0, def->loaded != 0);
                const bool cameraInside = ContainsIplPosition(def->bounds, camera.x, camera.y, extra);
                const bool streamingInside = ContainsIplPosition(def->bounds, streamingPosition.x, streamingPosition.y, extra);
                if (cameraInside || streamingInside)
                    def->requested = 1;
            }
        }
    }

    void ActivateProceduralScalingPatches()
    {
        for (const auto& operand : proceduralScalingOperands)
            ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = reinterpret_cast<uintptr_t>(operand.replacement);
        for (const auto& check : creationChecks)
            ProtectedGameRef<uint16_t>(reinterpret_cast<uint16_t*>(check.address)) = 0x9090;
        proceduralScalingActive = true;
    }

    void RestoreProceduralScalingPatches()
    {
        for (const auto& operand : proceduralScalingOperands)
        {
            if (*reinterpret_cast<const uintptr_t*>(operand.address) == reinterpret_cast<uintptr_t>(operand.replacement))
                ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = operand.original;
        }
        for (const auto& check : creationChecks)
        {
            if (*reinterpret_cast<const uint16_t*>(check.address) == 0x9090)
                ProtectedGameRef<uint16_t>(reinterpret_cast<uint16_t*>(check.address)) = check.original;
        }
        proceduralScalingActive = false;
    }

    void InstallAlphaHook()
    {
        alphaHook = safetyhook::create_inline(reinterpret_cast<void*>(0x732500), CalculateObjectAlpha);
    }

    void InstallNormalHook()
    {
        // SetupMapEntityVisibility has computed radius/fade width, but has not
        // made visibility/LOD decisions yet. The x87 stack is empty.
        normalVisibilityHook = safetyhook::create_mid(reinterpret_cast<void*>(0x554028), SetupNormalObjectVisibility);
        // outDistance already holds the real distance: the old `distance += cameraScaledModelDistance - normalCap`
        // adjustment is skipped
        injector::MakeJMP(0x5545E2, 0x554636, true);
    }

    void InstallProceduralHooks()
    {
        proceduralVisibilityHook = safetyhook::create_inline(
            reinterpret_cast<void*>(0x554230), SetupProceduralVisibility);
        surfaceHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5A3850), AddProceduralObjects);
        grassFar = *reinterpret_cast<const float*>(0x8D132C);
        for (const auto& operand : grassRampOperands)
            ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = reinterpret_cast<uintptr_t>(operand.replacement);
    }

    void InstallIplStreamingHook()
    {
        // CIplStore::LoadIpls: after native player/mission requests, just before
        // the request/unload loop. The streaming-disabled early return stays intact.
        // ESP + 0x1C is the (possibly velocity-adjusted) by-value position argument.
        iplRequestHook = safetyhook::create_mid(reinterpret_cast<void*>(0x405355), [](SafetyHookContext& ctx)
        {
            ExtendIplRequests(*reinterpret_cast<const CVector*>(ctx.esp + 0x1C));
        });
    }

    void Install()
    {
        // Redirect the renderer's 300 m non-LOD cap to the live distance setting.
        for (auto address : { 0x554230 + 0x3B6 + 2, 0x554230 + 0x3D0 + 2, 0x554230 + 0x3FA + 2, 0x554FE0 + 0x192 + 2, 0x554FE0 + 0x1B8 + 2,
                              0x554FE0 + 0x1DB + 2, 0x554FE0 + 0x24E + 2, 0x554FE0 + 0x258 + 2, 0x554FE0 + 0x262 + 2, 0x554FE0 + 0x314 + 2,
                              0x554FE0 + 0x31E + 2, 0x554FE0 + 0x328 + 2, 0x554FE0 + 0x382 + 2, 0x554FE0 + 0x39A + 2, 0x554FE0 + 0x3A8 + 2,
                              0x5B51E0 + 0x09A + 2 })
            injector::WriteMemory(address, &fMaxDrawDistanceForNormalObjects, true);

        InstallAlphaHook();
        InstallNormalHook();
        InstallProceduralHooks();

        InstallIplStreamingHook();
    }

    void UpdateProceduralDistance()
    {
        const float requested{ std::isfinite(fDetailDistance) ? std::clamp(fDetailDistance, 1.0f, 3.0f) : 1.0f };
        const float target{ requested > 1.0f ? (std::min)(requested, CalculateProceduralPoolScale()) : 1.0f };
        if (!proceduralScalingActive && target > 1.0f)
            ActivateProceduralScalingPatches();

        const float dt{ std::clamp(CTimer::GetTimeStepInSeconds(), 0.0f, 0.1f) };
        scale += (target - scale) * (1.0f - std::exp(-3.0f * dt));
        if (std::abs(scale - target) < 0.001f)
            scale = target;
        triangleSquared = 10000.0f * scale * scale;
        cacheRadius = 340.0f * scale;
        cacheSquared = cacheRadius * cacheRadius;
        grassFar = *reinterpret_cast<const float*>(0x8D132C) * scale;
        grassFadeWidth = 20.0f * scale;
        grassFadeInverse = 0.05f / scale;
        if (!proceduralScalingActive)
            return;
        const float radius{ 100.0f * scale };
        if (*reinterpret_cast<const float*>(0x5DCE79) != radius)
            ProtectedGameRef<float>(reinterpret_cast<float*>(0x5DCE79)) = radius;

        if (scale == 1.0f)
            RestoreProceduralScalingPatches();
    }
}
