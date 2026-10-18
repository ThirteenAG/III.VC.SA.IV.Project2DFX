#pragma once
#include <algorithm>
#include <cmath>

namespace ProceduralObjects::Distances
{
    inline constexpr float DefaultMinimum = 80.0f;
    inline constexpr float DefaultMaximum = 100.0f;
    inline constexpr float OuterCacheRadius = 340.0f;
    inline constexpr float MaximumDrawDistance = 150.0f;

    struct Settings
    {
        float draw = 0.0f;
        float minimum = DefaultMinimum;
        float maximum = DefaultMaximum;
    };

    inline Settings Calculate(float configuredDrawDistance)
    {
        if (!std::isfinite(configuredDrawDistance) || configuredDrawDistance <= 0.0f)
            return {};

        constexpr float cameraMultiplier = 1.8f;
        constexpr float fadeWidth = 20.0f;
        constexpr float safetyMargin = 30.0f;
        constexpr float generationWindow = 40.0f;

        const float draw = std::min(configuredDrawDistance, MaximumDrawDistance);
        // Plan for the stock slider maximum, independently of its current value.
        // With draw capped at 150, the stock fade is always 20 units wide.
        const float fadeEnd = cameraMultiplier * draw + fadeWidth;
        const float scale = std::min(1.0f, (OuterCacheRadius - fadeEnd) / (safetyMargin + generationWindow));
        const float minimum = fadeEnd + safetyMargin * scale;
        // Preserve the shared grass/triangle cache's vanilla radius at the low end.
        const float maximum = std::clamp(minimum + generationWindow * scale, DefaultMaximum, OuterCacheRadius);
        return { draw, minimum, maximum };
    }
}
