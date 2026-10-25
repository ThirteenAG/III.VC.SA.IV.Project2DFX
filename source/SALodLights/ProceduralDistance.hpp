#pragma once

#include <cmath>
#include <cstring>
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

    struct Operand
    {
        uintptr_t address;
        uint16_t opcode;
        uintptr_t original;
        float* replacement;
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

    int __fastcall AddObjects(float* surface, void*, void* triangle)
    {
        const float distanceSquared{ surface[4] };
        surface[4] = distanceSquared * scale * scale;
        const int result{ surfaceHook.unsafe_thiscall<int>(surface, triangle) };
        surface[4] = distanceSquared;
        return result;
    }

    bool Validate()
    {
        for (const auto& operand : operands)
        {
            if (*reinterpret_cast<const uint16_t*>(operand.address - 2) != operand.opcode
                || *reinterpret_cast<const uintptr_t*>(operand.address) != operand.original)
                return false;
        }
        return *reinterpret_cast<const float*>(0x859AA4) == 10000.0f
            && *reinterpret_cast<const float*>(0x85F074) == 340.0f
            && *reinterpret_cast<const float*>(0x86BF20) == 115600.0f
            && *reinterpret_cast<const float*>(0x858BA4) == 20.0f
            && *reinterpret_cast<const float*>(0x858C28) == 0.05f
            && *reinterpret_cast<const uint8_t*>(0x5DCE78) == 0x68
            && *reinterpret_cast<const float*>(0x5DCE79) == 100.0f
            && std::memcmp(reinterpret_cast<void*>(0x5A3850), "\x83\xEC\x7C\x53\x55\x56\x8B\xB4\x24\x8C\x00\x00\x00", 13) == 0;
    }

    void Update()
    {
        const float requested{ std::isfinite(fDetailDistance) ? std::clamp(fDetailDistance, 1.0f, 3.0f) : 1.0f };
        const float target{ requested > 1.0f ? (std::min)(requested, PoolScale()) : 1.0f };
        if (!active && target == 1.0f)
            return;

        if (!active)
        {
            if (!Validate())
                return;
            if (!surfaceHook)
                surfaceHook = safetyhook::create_inline(reinterpret_cast<void*>(0x5A3850), AddObjects);
            else if (!surfaceHook.enable())
                return;
            if (!surfaceHook)
                return;
            for (const auto& operand : operands)
                ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = reinterpret_cast<uintptr_t>(operand.replacement);
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
            if (!surfaceHook.disable())
                return;
            for (const auto& operand : operands)
            {
                if (*reinterpret_cast<const uintptr_t*>(operand.address) == reinterpret_cast<uintptr_t>(operand.replacement))
                    ProtectedGameRef<uintptr_t>(reinterpret_cast<uintptr_t*>(operand.address)) = operand.original;
            }
            active = false;
        }
    }
}
