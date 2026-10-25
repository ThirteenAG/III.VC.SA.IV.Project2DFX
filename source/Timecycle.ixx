module;

#include <stdafx.h>

export module Timecycle;

export namespace CTimeCycle
{
    GameRef<float> m_fCurrentFarClip;
    GameRef<float> m_fCurrentFogStart;
    GameRef<float> m_fCurrentSpriteBrightness;
    GameRef<float> m_fCurrentSpriteSize;
}