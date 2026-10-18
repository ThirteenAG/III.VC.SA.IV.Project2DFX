module;

#include "stdafx.h"
#include "ProceduralDistances.hpp"
#include "ProceduralRecovery.hpp"

export module ProceduralObjects;

import Entity;
import ModelInfo;

namespace ProceduralObjects
{
    namespace
    {
        // One load-time snapshot for all hooks. Patched operands retain pointers
        // to this storage; changing the INI requires restarting the game.
        Distances::Settings distances;
        float maximumDistanceSquared = 0.0f;

        SafetyHookInline setupEntityVisibilityHook;
        SafetyHookInline calculateFadingAtomicAlphaHook;
        SafetyHookInline initSurfaceHook;
        SafetyHookInline addObjectHook;

        static_assert(sizeof(Recovery::ObjectEntry) == 0x14);
        static_assert(offsetof(Recovery::ObjectEntry, object) == 0x8);
        static_assert(sizeof(Recovery::ObjectList) == 0xC);
        static_assert(offsetof(Recovery::SurfaceInfo, squaredMinDistance) == 0x10);
        static_assert(offsetof(Recovery::SurfaceInfo, objects) == 0x3C);
        static_assert(sizeof(Recovery::SurfaceInfo) == 0x48);
        static_assert(sizeof(RwV3d) == 0xC);

        // Model info is shared by generated and placed instances. Substitute its
        // distance only for a synchronous renderer call, then restore it.
        class ScopedDrawDistance
        {
        public:
            explicit ScopedDrawDistance(CBaseModelInfo* model)
                : value(model->m_fDrawDistance), saved(value)
            {
                value = distances.draw;
            }

            ~ScopedDrawDistance()
            {
                value = saved;
            }

            ScopedDrawDistance(const ScopedDrawDistance&) = delete;
            ScopedDrawDistance& operator=(const ScopedDrawDistance&) = delete;

        private:
            float& value;
            const float saved;
        };

        int __cdecl SetupEntityVisibility(CEntity* entity, float* distance)
        {
            if (!entity->bIsProcObject)
                return setupEntityVisibilityHook.unsafe_ccall<int>(entity, distance);

            auto* model = (*CModelInfo::pp_modelInfoPtrs)[entity->GetModelIndex()];
            ScopedDrawDistance override(model);
            return setupEntityVisibilityHook.unsafe_ccall<int>(entity, distance);
        }

        int __cdecl CalculateFadingAtomicAlpha(CBaseModelInfo* model, CEntity* entity, float distance)
        {
            if (!entity->bIsProcObject)
                return calculateFadingAtomicAlphaHook.unsafe_ccall<int>(model, entity, distance);

            // Alpha is calculated later from the render queue, after the
            // visibility call has already restored the shared model distance.
            ScopedDrawDistance override(model);
            return calculateFadingAtomicAlphaHook.unsafe_ccall<int>(model, entity, distance);
        }

        // __fastcall preserves the original __thiscall ECX and stack arguments.
        void __fastcall InitSurface(Recovery::SurfaceInfo* surface, void*, const char* surfaceType, const char* modelName,
            float spacing, float minDist, int32_t minRot, int32_t maxRot,
            float minScale, float maxScale, float minScaleZ, float maxScaleZ,
            float zOffsetMin, float zOffsetMax, bool align, uint8_t useGrid)
        {
            initSurfaceHook.unsafe_thiscall<void>(surface, surfaceType, modelName, spacing, minDist,
                minRot, maxRot, minScale, maxScale, minScaleZ, maxScaleZ,
                zOffsetMin, zOffsetMax, align, useGrid);

            // The original returns early for unknown models.
            int modelID;
            if (!CModelInfo::GetModelInfo(modelName, &modelID))
                return;

            // Use the .dat argument, rather than the original's 80-unit clamp.
            // Higher authored minima remain authoritative.
            const float minimum = std::max(minDist, distances.minimum);
            surface->squaredMinDistance = minimum * minimum;
        }

        Recovery::ObjectEntry* __fastcall AddObject(
            Recovery::SurfaceInfo* surface, void*, RwV3d position, RwV3d normal, uint8_t lighting)
        {
            // ProcObjectMan::m_ObjectsList, shared by vanilla and OLA storage.
            auto& unused = *reinterpret_cast<Recovery::ObjectList*>(0xBBC8B8);
            const Recovery::AllocationSnapshot snapshot(unused);
            auto* result = addObjectHook.unsafe_thiscall<Recovery::ObjectEntry*>(surface, position, normal, lighting);
            if (!result)
                snapshot.RecoverFailure(unused, *surface);
            return result;
        }

        void PatchTriangleCache()
        {
            if (distances.maximum <= Distances::DefaultMaximum)
                return;

            maximumDistanceSquared = distances.maximum * distances.maximum;
            // Retarget only CPlantMgr's triangle removal/admission comparisons;
            // the original 10000 constant is also used by unrelated systems.
            for (const auto& range : { std::pair{0x5DBF20, 0x5DC510}, std::pair{0x5DC8B0, 0x5DCD80} })
            {
                auto checks = hook::range_pattern(range.first, range.second, "D8 1D A4 9A 85 00");
                checks.for_each_result([](hook::pattern_match match)
                {
                    injector::WriteMemory(match.get<void>(2), &maximumDistanceSquared, true);
                });
            }

            // Collision face-group sphere test uses an immediate, unsquared radius.
            auto sphereRadius = hook::range_pattern(0x5DCD80, 0x5DCF00, "68 00 00 C8 42");
            injector::WriteMemory<float>(sphereRadius.get_first(1), distances.maximum, true);
        }
    }
}

export namespace ProceduralObjects
{
    void InstallHooks(float configuredDrawDistance)
    {
        distances = Distances::Calculate(configuredDrawDistance);

        // Recovery is independent of the optional rendering/generation override.
        addObjectHook = safetyhook::create_inline(0x5A32D0, AddObject);
        if (distances.draw == 0.0f)
            return;

        setupEntityVisibilityHook = safetyhook::create_inline(0x554230, SetupEntityVisibility);
        calculateFadingAtomicAlphaHook = safetyhook::create_inline(0x732500, CalculateFadingAtomicAlpha);
        if (distances.minimum != Distances::DefaultMinimum)
            initSurfaceHook = safetyhook::create_inline(0x5A2EB0, InitSurface);

        PatchTriangleCache();
    }
}
