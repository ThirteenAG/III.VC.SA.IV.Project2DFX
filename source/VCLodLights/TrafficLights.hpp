#pragma once
#include "../DistantLightLogic.hpp"

namespace LocalTrafficLights
{
    inline float* Wind = nullptr;
    inline bool* bGreenLightsCheat = nullptr;

    inline int FindTrafficLightType(CEntity* light)
    {
        const auto& forward = light->GetMatrix().GetForward();
        return DistantLightLogic::TrafficGroup(forward.x, forward.y);
    }
    inline uint8_t LightForCars1()
    {
        if ((Wind && *Wind > 1.1f) || (bGreenLightsCheat && *bGreenLightsCheat)) return 0;
        return DistantLightLogic::TrafficPhase(CTimer::m_snTimeInMilliseconds, 1, 1);
    }
    inline uint8_t LightForCars2()
    {
        if ((Wind && *Wind > 1.1f) || (bGreenLightsCheat && *bGreenLightsCheat)) return 0;
        return DistantLightLogic::TrafficPhase(CTimer::m_snTimeInMilliseconds, 2, 1);
    }
    inline uint8_t LightForCars1_Visual()
    {
        if (Wind && *Wind > 1.1f) return (CTimer::m_snTimeInMilliseconds & 0x400) ? 3 : 1;
        return LightForCars1();
    }
    inline uint8_t LightForCars2_Visual()
    {
        if (Wind && *Wind > 1.1f) return (CTimer::m_snTimeInMilliseconds & 0x400) ? 3 : 1;
        return LightForCars2();
    }
}
