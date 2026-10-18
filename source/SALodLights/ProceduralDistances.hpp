#pragma once
#include <algorithm>
#include <cmath>

namespace ProceduralDetails::Distances
{
    inline constexpr float DefaultObjectMinDistance = 80.0f;
    inline constexpr float DefaultTriangleCacheRadius = 100.0f;
    inline constexpr float OuterCollisionCacheRadius = 340.0f;
    inline constexpr float MaximumDrawDistance = 150.0f;
    inline constexpr float FadeWidth = 20.0f;
    inline constexpr float SafetyMargin = 20.0f;
    inline constexpr float MaximumCameraMultiplier = 1.8f;

    struct Settings
    {
        float objectDrawDistance = 0.0f;
        float grassDrawDistance = 0.0f;
        float objectMinDistance = DefaultObjectMinDistance;
        float triangleCacheRadius = DefaultTriangleCacheRadius;
    };

    inline float ClampDrawDistance(float value)
    {
        return std::isfinite(value) && value > 0.0f ? std::min(value, MaximumDrawDistance) : 0.0f;
    }

    inline Settings Calculate(float configuredObjectDrawDistance, float configuredGrassDrawDistance = 0.0f)
    {
        constexpr float objectGenerationWindow = 40.0f;

        Settings result;
        result.objectDrawDistance = ClampDrawDistance(configuredObjectDrawDistance);
        result.grassDrawDistance = ClampDrawDistance(configuredGrassDrawDistance);

        if (result.objectDrawDistance != 0.0f)
        {
            // Plan for the stock slider maximum, independently of its current value.
            // With draw capped at 150, the stock object fade is 20 units wide.
            const float fadeEnd = MaximumCameraMultiplier * result.objectDrawDistance + FadeWidth;
            result.objectMinDistance = fadeEnd + SafetyMargin;
            // Preserve the safety margin; only the generation window narrows
            // when the final shared-cache radius is clamped to the outer limit.
            result.triangleCacheRadius = result.objectMinDistance + objectGenerationWindow;
        }

        if (result.grassDrawDistance != 0.0f)
        {
            // Budget for the maximum stock slider and High/Very High FX. Grass
            // has no minimum creation exclusion. Keep triangles beyond its fade.
            // Both consumers use this cache, so neither may shrink the other's range.
            const float grassCacheRadius = result.grassDrawDistance * MaximumCameraMultiplier + FadeWidth + SafetyMargin;
            result.triangleCacheRadius = std::max(result.triangleCacheRadius, grassCacheRadius);
        }
        result.triangleCacheRadius = std::clamp(result.triangleCacheRadius, DefaultTriangleCacheRadius, OuterCollisionCacheRadius);
        return result;
    }
}
