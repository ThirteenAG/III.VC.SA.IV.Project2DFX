#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace DistantLightLogic
{
    inline uint8_t TrafficGroup(float forwardX, float forwardY)
    {
        float degrees = std::atan2(forwardY, forwardX) * (180.0f / 3.14159265358979323846f);
        if (degrees < 0.0f) degrees += 360.0f;
        return ((degrees > 60.0f && degrees < 150.0f) || (degrees > 240.0f && degrees < 330.0f)) ? 1 : 2;
    }
    inline uint8_t TrafficPhase(uint32_t time, unsigned group, unsigned divisor)
    {
        uint32_t phase = (time / divisor) & 16383u;
        if (group == 1) return phase < 5000 ? 0 : phase < 6000 ? 1 : 2;
        return phase < 6000 ? 2 : phase < 11000 ? 0 : phase < 12000 ? 1 : 2;
    }
    inline float Pulse(uint32_t time, uint32_t on, uint32_t off)
    {
        uint32_t period = on + off;
        // Match IsBlinkingNeeded: one global phase for every light of a mode.
        uint32_t phase = time % period;
        if (phase >= on) return 0.0f;
        float edge = (std::min)(500.0f, float(on) * .5f);
        float fade = (std::min)(float(phase), float(on - phase)) / edge;
        fade = (std::clamp)(fade, 0.0f, 1.0f);
        return fade * fade * (3.0f - 2.0f * fade);
    }
}
