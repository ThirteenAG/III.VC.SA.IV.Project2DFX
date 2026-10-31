#pragma once

#include <cmath>
#include <psapi.h>

namespace ProceduralDistance
{
    static float scale{ 1.0f };
    static float triangleSquared{ 10000.0f };
    static float cacheRadius{ 340.0f };
    static float cacheSquared{ 115600.0f };
    static float grassFar{ 60.0f };
    static float fadeWidth{ 20.0f };
    static float fadeInverse{ 0.05f };
    static bool active{};
    static SafetyHookInline surfaceHook{};
    static SafetyHookInline visibilityHook{};
    static SafetyHookInline alphaHook{};

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

    // Creation limits: jge matrix allowance, jae temp-object threshold. Both skip creation.
    static const CreationCheck creationChecks[]
    {
        { 0x5A332A, 0x777D },
        { 0x5A3355, 0x4C73 }
    };

    static const Operand operands[]
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
        { 0x5DAD78, 0x158B, 0x8D132C, &grassFar },
        { 0x5DAD84, 0x05D9, 0x8D132C, &grassFar },
        { 0x5DAD98, 0x05D8, 0x858BA4, &fadeWidth },
        { 0x5DAE05, 0x0DD8, 0x858C28, &fadeInverse }
    };

    float PoolScale()
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

    // Same distance the grass renderer uses, including its medium FX quality half.
    float SharedDraw()
    {
        const int quality{ *reinterpret_cast<const int*>(0xA9AE54) };
        return quality >= 2 ? grassFar : grassFar * *reinterpret_cast<const float*>(0x858B8C);
    }

    int __fastcall AddObjects(float* surface, void*, void* triangle)
    {
        const float distanceSquared{ surface[4] };
        // No near cutoff, same as grass. Ignores procobj.dat MINDIST and the load-time clamp to 80.
        surface[4] = 0.0f;
        const int result{ surfaceHook.unsafe_thiscall<int>(surface, triangle) };
        surface[4] = distanceSquared;
        return result;
    }

    template<typename Fn>
    bool Install(SafetyHookInline& hook, void* target, Fn destination)
    {
        if (!hook)
            hook = safetyhook::create_inline(target, destination);
        else if (!hook.enable())
            return false;
        return static_cast<bool>(hook);
    }

    int __cdecl SetupObjectVisibility(CEntity* entity, float* distance)
    {
        if (!active || !(scale > 1.0f) || !entity || !entity->bIsProcObject)
            return visibilityHook.unsafe_ccall<int>(entity, distance);

        const int modelIndex{ entity->GetModelIndex() };
        auto** models{ CModelInfo::pp_modelInfoPtrs ? *CModelInfo::pp_modelInfoPtrs : nullptr };
        auto* model{ models && modelIndex >= 0 ? models[modelIndex] : nullptr };
        const float lod{ TheCamera->LODDistMultiplier };
        if (!model || lod <= 0.0f)
            return visibilityHook.unsafe_ccall<int>(entity, distance);

        const float draw{ SharedDraw() };
        const float engineFade{ *reinterpret_cast<const float*>(0x858BA4) };
        const float fitted{ (draw + fadeWidth - engineFade) / lod };
        if (fitted <= 0.0f)
            return visibilityHook.unsafe_ccall<int>(entity, distance);

        // The engine culls engineFade past the draw distance. Land that on the grass ramp end.
        const float saved{ model->m_fDrawDistance };
        model->m_fDrawDistance = fitted;
        const int result{ visibilityHook.unsafe_ccall<int>(entity, distance) };
        model->m_fDrawDistance = saved;

        // The caller adds visible entities after this returns. The engine only marks its own
        // last engineFade metres as fading, so flag the rest of the grass ramp first.
        constexpr int visible{ 1 };
        if (result == visible && distance && *distance >= draw && *distance < draw + fadeWidth)
            entity->bDistanceFade = true;
        return result;
    }

    int __cdecl CalculateObjectAlpha(CBaseModelInfo* model, CEntity* entity, float distance)
    {
        if (!active || !(scale > 1.0f) || !entity || !entity->bIsProcObject)
            return alphaHook.unsafe_ccall<int>(model, entity, distance);

        const float fade{ std::clamp((SharedDraw() + fadeWidth - distance) / fadeWidth, 0.0f, 1.0f) };
        return static_cast<int>(static_cast<uint8_t>(model->m_nAlpha) * fade);
    }

    void Update()
    {
        const float requested{ std::isfinite(fDetailDistance) ? std::clamp(fDetailDistance, 1.0f, 3.0f) : 1.0f };
        const float target{ requested > 1.0f ? (std::min)(requested, PoolScale()) : 1.0f };
        if (!active && target == 1.0f)
            return;

        if (!active)
        {
            if (!Install(surfaceHook, reinterpret_cast<void*>(0x5A3850), AddObjects)
                || !Install(visibilityHook, reinterpret_cast<void*>(0x554230), SetupObjectVisibility)
                || !Install(alphaHook, reinterpret_cast<void*>(0x732500), CalculateObjectAlpha))
                return;
            for (const auto& operand : operands)
                ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = reinterpret_cast<uintptr_t>(operand.replacement);
            for (const auto& check : creationChecks)
                ProtectedGameRef<uint16_t>(reinterpret_cast<uint16_t*>(check.address)) = 0x9090;
            active = true;
        }

        const float dt{ std::clamp(CTimer::GetTimeStepInSeconds(), 0.0f, 0.1f) };
        scale += (target - scale) * (1.0f - std::exp(-3.0f * dt));
        if (std::abs(scale - target) < 0.001f)
            scale = target;
        triangleSquared = 10000.0f * scale * scale;
        cacheRadius = 340.0f * scale;
        cacheSquared = cacheRadius * cacheRadius;
        // Preserve the quality setting's half-distance branch and the whole fade ramp.
        grassFar = *reinterpret_cast<const float*>(0x8D132C) * scale;
        fadeWidth = 20.0f * scale;
        fadeInverse = 0.05f / scale;
        const float radius{ 100.0f * scale };
        if (*reinterpret_cast<const float*>(0x5DCE79) != radius)
            ProtectedGameRef<float>(reinterpret_cast<float*>(0x5DCE79)) = radius;

        if (scale == 1.0f)
        {
            if (!surfaceHook.disable() || !visibilityHook.disable() || !alphaHook.disable())
                return;
            for (const auto& operand : operands)
            {
                if (*reinterpret_cast<const uintptr_t*>(operand.address) == reinterpret_cast<uintptr_t>(operand.replacement))
                    ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = operand.original;
            }
            for (const auto& check : creationChecks)
            {
                if (*reinterpret_cast<const uint16_t*>(check.address) == 0x9090)
                    ProtectedGameRef<uint16_t>(reinterpret_cast<uint16_t*>(check.address)) = check.original;
            }
            active = false;
        }
    }
}
