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

        if (result.objectDrawDistance != 0.0f)
        {
            result.objectMinDistance = result.objectDrawDistance * MaximumCameraMultiplier + FadeWidth + SafetyMargin;
            result.triangleCacheRadius = result.objectMinDistance + objectGenerationWindow;
        }
        if (result.grassDrawDistance != 0.0f)
        {
            const float grassCacheRadius = result.grassDrawDistance * MaximumCameraMultiplier + FadeWidth + SafetyMargin;
            result.triangleCacheRadius = (std::max)(result.triangleCacheRadius, grassCacheRadius);
        }
        // Keep the 100-unit floor; only the object window narrows at 340.
        result.triangleCacheRadius = std::clamp(result.triangleCacheRadius, DefaultTriangleCacheRadius, OuterCollisionCacheRadius);
        return result;
    }

    struct SurfaceInfo
    {
        uint8_t properties[0x10];
        float squaredMinDistance;
    };

    static_assert(offsetof(SurfaceInfo, squaredMinDistance) == 0x10);

    DistanceSettings distanceSettings;
    float triangleCacheRadiusSquared = 0.0f;

    SafetyHookInline objectVisibilityHook;
    SafetyHookInline objectFadeAlphaHook;
    SafetyHookInline grassRenderHook;
    SafetyHookInline objectSurfaceInitHook;

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

        // Fade runs after visibility restores the shared distance.
        ScopedDistanceOverride override(model->m_fDrawDistance, distanceSettings.objectDrawDistance);
        return objectFadeAlphaHook.unsafe_ccall<int>(model, entity, distance);
    }

    void __cdecl RenderGrass(void* plants, int32_t count, void** models)
    {
        auto& farDistance = *reinterpret_cast<float*>(0x8D132C); // CGrassRenderer::m_farDist
        ScopedDistanceOverride override(farDistance, distanceSettings.grassDrawDistance * TheCamera->LODDistMultiplier);
        grassRenderHook.unsafe_ccall<void>(plants, count, models);
    }

    bool __fastcall InitObjectSurface(SurfaceInfo* surface, void*, const char* surfaceType, const char* modelName,
        float spacing, float minDist, int32_t minRot, int32_t maxRot,
        float minScale, float maxScale, float minScaleZ, float maxScaleZ,
        float zOffsetMin, float zOffsetMax, bool align, uint8_t useGrid)
    {
        const bool initialized = objectSurfaceInitHook.unsafe_thiscall<bool>(surface, surfaceType, modelName, spacing, minDist,
            minRot, maxRot, minScale, maxScale, minScaleZ, maxScaleZ,
            zOffsetMin, zOffsetMax, align, useGrid);
        if (!initialized)
            return false;

        // SA clamps .dat minimum down to 80; keep any higher value.
        const float minimum = (std::max)(minDist, distanceSettings.objectMinDistance);
        surface->squaredMinDistance = minimum * minimum;
        return true;
    }

    void PatchTriangleCache()
    {
        if (distanceSettings.triangleCacheRadius <= DefaultTriangleCacheRadius)
            return;

        triangleCacheRadiusSquared = distanceSettings.triangleCacheRadius * distanceSettings.triangleCacheRadius;
        // Patch only these comparisons; 10000 is shared.
        for (const auto& range : { std::pair{0x5DBF20, 0x5DC510}, std::pair{0x5DC8B0, 0x5DCD80} })
        {
            auto checks = hook::range_pattern(range.first, range.second, "D8 1D A4 9A 85 00");
            checks.for_each_result([](hook::pattern_match match)
            {
                injector::WriteMemory(match.get<void>(2), &triangleCacheRadiusSquared, true);
            });
        }

        auto sphereRadius = hook::range_pattern(0x5DCD80, 0x5DCF00, "68 00 00 C8 42"); // Unsquared radius.
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
