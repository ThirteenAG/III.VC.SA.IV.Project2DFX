#pragma once
#include "../DistantLightLogic.hpp"

namespace LocalTrafficLights
{
    inline int FindTrafficLightType(CEntity* light)
    {
        const auto& forward = light->GetMatrix().GetForward();
        return DistantLightLogic::TrafficGroup(forward.x, forward.y);
    }
    inline uint8_t LightForCars1()
    {
        return DistantLightLogic::TrafficPhase(CTimer::m_snTimeInMilliseconds, 1, 1);
    }
    inline uint8_t LightForCars2()
    {
        return DistantLightLogic::TrafficPhase(CTimer::m_snTimeInMilliseconds, 2, 1);
    }
}
