module;

#include "stdafx.h"
#include "ProceduralDistances.hpp"
#include "ProceduralObjectRecovery.hpp"

export module ProceduralDetails;

import Entity;
import ModelInfo;
import Camera;

namespace ProceduralDetails
{
    namespace
    {
        // One load-time snapshot for all hooks. Patched operands retain pointers
        // to this storage; changing the INI requires restarting the game.
        Distances::Settings distanceSettings;
        float triangleCacheRadiusSquared = 0.0f;

        SafetyHookInline objectVisibilityHook;
        SafetyHookInline objectFadeAlphaHook;
        SafetyHookInline grassRenderHook;
        SafetyHookInline objectSurfaceInitHook;
        SafetyHookInline objectCreationHook;

        static_assert(sizeof(ObjectRecovery::ObjectEntry) == 0x14);
        static_assert(offsetof(ObjectRecovery::ObjectEntry, object) == 0x8);
        static_assert(sizeof(ObjectRecovery::ObjectList) == 0xC);
        static_assert(offsetof(ObjectRecovery::SurfaceInfo, squaredMinDistance) == 0x10);
        static_assert(offsetof(ObjectRecovery::SurfaceInfo, objects) == 0x3C);
        static_assert(sizeof(ObjectRecovery::SurfaceInfo) == 0x48);
        static_assert(sizeof(RwV3d) == 0xC);

        // Substitute a distance only for a synchronous renderer call, then restore
        // it. Object model info is shared by generated and placed instances.
        class ScopedDistanceOverride
        {
        public:
            ScopedDistanceOverride(float& distance, float replacement)
                : value(distance), saved(value)
            {
                value = replacement;
            }

            ~ScopedDistanceOverride()
            {
                value = saved;
            }

            ScopedDistanceOverride(const ScopedDistanceOverride&) = delete;
            ScopedDistanceOverride& operator=(const ScopedDistanceOverride&) = delete;

        private:
            float& value;
            const float saved;
        };

        int __cdecl SetupObjectVisibility(CEntity* entity, float* distance)
        {
            if (!entity->bIsProcObject)
                return objectVisibilityHook.unsafe_ccall<int>(entity, distance);

            auto* model = (*CModelInfo::pp_modelInfoPtrs)[entity->GetModelIndex()];
            ScopedDistanceOverride override(model->m_fDrawDistance, distanceSettings.objectDrawDistance);
            return objectVisibilityHook.unsafe_ccall<int>(entity, distance);
        }

        int __cdecl CalculateObjectFadeAlpha(CBaseModelInfo* model, CEntity* entity, float distance)
        {
            if (!entity->bIsProcObject)
                return objectFadeAlphaHook.unsafe_ccall<int>(model, entity, distance);

            // Alpha is calculated later from the render queue, after the
            // visibility call has already restored the shared model distance.
            ScopedDistanceOverride override(model->m_fDrawDistance, distanceSettings.objectDrawDistance);
            return objectFadeAlphaHook.unsafe_ccall<int>(model, entity, distance);
        }

        // Opaque PPTriPlant* and RpAtomic**; the original owns both layouts.
        void __cdecl RenderGrass(void* plants, int32_t count, void** models)
        {
            // CGrassRenderer::m_farDist. Override at draw time so plant-manager
            // initialization/reloads cannot reset the configured distance.
            auto& farDistance = *reinterpret_cast<float*>(0x8D132C);
            ScopedDistanceOverride override(farDistance, distanceSettings.grassDrawDistance * TheCamera->LODDistMultiplier);
            // The native renderer retains its 20-unit fade, near-camera exclusion
            // and FX-quality behavior (medium halves the range; low hides grass).
            grassRenderHook.unsafe_ccall<void>(plants, count, models);
        }

        // __fastcall preserves the original __thiscall ECX and stack arguments.
        void __fastcall InitObjectSurface(ObjectRecovery::SurfaceInfo* surface, void*, const char* surfaceType, const char* modelName,
            float spacing, float minDist, int32_t minRot, int32_t maxRot,
            float minScale, float maxScale, float minScaleZ, float maxScaleZ,
            float zOffsetMin, float zOffsetMax, bool align, uint8_t useGrid)
        {
            objectSurfaceInitHook.unsafe_thiscall<void>(surface, surfaceType, modelName, spacing, minDist,
                minRot, maxRot, minScale, maxScale, minScaleZ, maxScaleZ,
                zOffsetMin, zOffsetMax, align, useGrid);

            // The original returns early for unknown models.
            int modelID;
            if (!CModelInfo::GetModelInfo(modelName, &modelID))
                return;

            // Use the .dat argument, rather than the original's 80-unit clamp.
            // Higher authored minima remain authoritative.
            const float minimum = std::max(minDist, distanceSettings.objectMinDistance);
            surface->squaredMinDistance = minimum * minimum;
        }

        ObjectRecovery::ObjectEntry* __fastcall CreateObject(
            ObjectRecovery::SurfaceInfo* surface, void*, RwV3d position, RwV3d normal, uint8_t lighting)
        {
            // ProcObjectMan::m_ObjectsList, shared by vanilla and OLA storage.
            auto& unused = *reinterpret_cast<ObjectRecovery::ObjectList*>(0xBBC8B8);
            const ObjectRecovery::AllocationSnapshot snapshot(unused);
            auto* result = objectCreationHook.unsafe_thiscall<ObjectRecovery::ObjectEntry*>(surface, position, normal, lighting);
            if (!result)
                snapshot.RecoverFailure(unused, *surface);
            return result;
        }

        void PatchTriangleCache()
        {
            if (distanceSettings.triangleCacheRadius <= Distances::DefaultTriangleCacheRadius)
                return;

            triangleCacheRadiusSquared = distanceSettings.triangleCacheRadius * distanceSettings.triangleCacheRadius;
            // Retarget only CPlantMgr's triangle removal/admission comparisons;
            // the original 10000 constant is also used by unrelated systems.
            for (const auto& range : { std::pair{0x5DBF20, 0x5DC510}, std::pair{0x5DC8B0, 0x5DCD80} })
            {
                auto checks = hook::range_pattern(range.first, range.second, "D8 1D A4 9A 85 00");
                checks.for_each_result([](hook::pattern_match match)
                {
                    injector::WriteMemory(match.get<void>(2), &triangleCacheRadiusSquared, true);
                });
            }

            // Collision face-group sphere test uses an immediate, unsquared radius.
            auto sphereRadius = hook::range_pattern(0x5DCD80, 0x5DCF00, "68 00 00 C8 42");
            injector::WriteMemory<float>(sphereRadius.get_first(1), distanceSettings.triangleCacheRadius, true);
        }
    }
}

export namespace ProceduralDetails
{
    void InstallHooks(float configuredObjectDrawDistance, float configuredGrassDrawDistance)
    {
        distanceSettings = Distances::Calculate(configuredObjectDrawDistance, configuredGrassDrawDistance);

        // Recovery is independent of the optional rendering/generation override.
        objectCreationHook = safetyhook::create_inline(0x5A32D0, CreateObject);
        if (distanceSettings.objectDrawDistance != 0.0f)
        {
            objectVisibilityHook = safetyhook::create_inline(0x554230, SetupObjectVisibility);
            objectFadeAlphaHook = safetyhook::create_inline(0x732500, CalculateObjectFadeAlpha);
            if (distanceSettings.objectMinDistance != Distances::DefaultObjectMinDistance)
                objectSurfaceInitHook = safetyhook::create_inline(0x5A2EB0, InitObjectSurface);
        }
        if (distanceSettings.grassDrawDistance != 0.0f)
            grassRenderHook = safetyhook::create_inline(0x5DAD00, RenderGrass);

        PatchTriangleCache();
    }
}
