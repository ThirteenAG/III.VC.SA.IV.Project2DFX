module;

#include <stdafx.h>

export module Timer;

export namespace CTimer
{
    GameRef<unsigned int> m_snTimeInMilliseconds;
    GameRef<unsigned int> m_snTimeInMillisecondsPauseMode;
    GameRef<float> ms_fTimeStep;

    // SA/IV effects use the pause-mode clock; III/VC bind their normal clock.
    GameRef<unsigned int>* EffectsTime = &m_snTimeInMillisecondsPauseMode;
    unsigned int GetEffectsTimeInMilliseconds() { return *EffectsTime; }

    float GetTimeStepInSeconds() { return ms_fTimeStep / 50.0f; }
}