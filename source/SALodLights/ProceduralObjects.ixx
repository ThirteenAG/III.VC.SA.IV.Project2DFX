module;

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "stdafx.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>

export module ProceduralObjects;

import Entity;
import ModelInfo;
import Camera;

namespace ProceduralObjects
{
    constexpr float DefaultObjectMinDistance = 80.0f;
    constexpr float DefaultTriangleCacheRadius = 100.0f;
    constexpr float OuterCollisionCacheRadius = 340.0f;
    constexpr float MaximumDrawDistance = 150.0f;
    constexpr float FadeWidth = 20.0f;
    constexpr float SafetyMargin = 20.0f;
    constexpr float MaximumCameraMultiplier = 1.8f;

    struct DistanceSettings
    {
        float objectDrawDistance = 0.0f;
        float grassDrawDistance = 0.0f;
        float objectMinDistance = DefaultObjectMinDistance;
        float triangleCacheRadius = DefaultTriangleCacheRadius;
    };

    float ClampDrawDistance(float value)
    {
        return std::isfinite(value) && value > 0.0f ? (std::min)(value, MaximumDrawDistance) : 0.0f;
    }

    DistanceSettings CalculateDistances(float objectDrawDistance, float grassDrawDistance = 0.0f)
    {
        constexpr float objectGenerationWindow = 40.0f;
        DistanceSettings result;
        result.objectDrawDistance = ClampDrawDistance(objectDrawDistance);
        result.grassDrawDistance = ClampDrawDistance(grassDrawDistance);

        // Budget for the maximum stock slider. The 150 cap keeps both fades at 20 units.
        if (result.objectDrawDistance != 0.0f)
        {
            result.objectMinDistance = result.objectDrawDistance * MaximumCameraMultiplier + FadeWidth + SafetyMargin;
            result.triangleCacheRadius = result.objectMinDistance + objectGenerationWindow;
        }
        if (result.grassDrawDistance != 0.0f)
        {
            // Grass needs cached triangles beyond its fade, but no creation exclusion.
            const float grassCacheRadius = result.grassDrawDistance * MaximumCameraMultiplier + FadeWidth + SafetyMargin;
            result.triangleCacheRadius = (std::max)(result.triangleCacheRadius, grassCacheRadius);
        }
        // Preserve the grass cache floor and safety margin; only the object window narrows.
        result.triangleCacheRadius = std::clamp(result.triangleCacheRadius, DefaultTriangleCacheRadius, OuterCollisionCacheRadius);
        return result;
    }

    // Only the field rewritten after native initialization needs a known offset.
    struct SurfaceInfo
    {
        uint8_t properties[0x10];
        float squaredMinDistance;
    };

    static_assert(offsetof(SurfaceInfo, squaredMinDistance) == 0x10);

    // Load-time configuration; patched instructions retain pointers to this storage.
    DistanceSettings distanceSettings;
    float triangleCacheRadiusSquared = 0.0f;

    SafetyHookInline objectVisibilityHook;
    SafetyHookInline objectFadeAlphaHook;
    SafetyHookInline grassRenderHook;
    SafetyHookInline objectSurfaceInitHook;

    // Restore shared model/grass state before another instance is processed.
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

        // Deferred rendering needs its own override after visibility restored the model.
        ScopedDistanceOverride override(model->m_fDrawDistance, distanceSettings.objectDrawDistance);
        return objectFadeAlphaHook.unsafe_ccall<int>(model, entity, distance);
    }

    // Opaque PPTriPlant* and RpAtomic**; the original owns both layouts.
    void __cdecl RenderGrass(void* plants, int32_t count, void** models)
    {
        // Override CGrassRenderer::m_farDist at draw time to survive plant reloads.
        // Native rendering retains its fade, near-camera exclusion and FX-quality scaling.
        auto& farDistance = *reinterpret_cast<float*>(0x8D132C);
        ScopedDistanceOverride override(farDistance, distanceSettings.grassDrawDistance * TheCamera->LODDistMultiplier);
        grassRenderHook.unsafe_ccall<void>(plants, count, models);
    }

    // __fastcall preserves the original __thiscall ECX and stack arguments.
    bool __fastcall InitObjectSurface(SurfaceInfo* surface, void*, const char* surfaceType, const char* modelName,
        float spacing, float minDist, int32_t minRot, int32_t maxRot,
        float minScale, float maxScale, float minScaleZ, float maxScaleZ,
        float zOffsetMin, float zOffsetMax, bool align, uint8_t useGrid)
    {
        // Native SA 1.0 US clamps this argument down to 80. Its return reports
        // whether the model was valid, so no second lookup is needed.
        const bool initialized = objectSurfaceInitHook.unsafe_thiscall<bool>(surface, surfaceType, modelName, spacing, minDist,
            minRot, maxRot, minScale, maxScale, minScaleZ, maxScaleZ,
            zOffsetMin, zOffsetMax, align, useGrid);
        if (!initialized)
            return false;

        // Apply our floor while preserving a higher authored .dat minimum.
        const float minimum = (std::max)(minDist, distanceSettings.objectMinDistance);
        surface->squaredMinDistance = minimum * minimum;
        return true;
    }

    void PatchTriangleCache()
    {
        if (distanceSettings.triangleCacheRadius <= DefaultTriangleCacheRadius)
            return;

        triangleCacheRadiusSquared = distanceSettings.triangleCacheRadius * distanceSettings.triangleCacheRadius;
        // Retarget only CPlantMgr's triangle removal/admission comparisons, not
        // the original 10000 constant shared with unrelated systems.
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

    export void InstallHooks(float objectDrawDistance, float grassDrawDistance)
    {
        distanceSettings = CalculateDistances(objectDrawDistance, grassDrawDistance);

        if (distanceSettings.objectDrawDistance != 0.0f)
        {
            objectVisibilityHook = safetyhook::create_inline(0x554230, SetupObjectVisibility);
            objectFadeAlphaHook = safetyhook::create_inline(0x732500, CalculateObjectFadeAlpha);
            objectSurfaceInitHook = safetyhook::create_inline(0x5A2EB0, InitObjectSurface);
        }
        if (distanceSettings.grassDrawDistance != 0.0f)
            grassRenderHook = safetyhook::create_inline(0x5DAD00, RenderGrass);

        PatchTriangleCache();
    }
}
