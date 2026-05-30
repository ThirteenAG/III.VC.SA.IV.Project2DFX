module;

#include <stdafx.h>

export module Timer;

export namespace CTimer
{
    GameRef<unsigned int> m_snTimeInMilliseconds;
    GameRef<unsigned int> m_snTimeInMillisecondsPauseMode;
    GameRef<float> ms_fTimeStep;

    float GetTimeStepInSeconds() { return ms_fTimeStep / 50.0f; }
}