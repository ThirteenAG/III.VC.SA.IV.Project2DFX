module;

#include <stdafx.h>

export module Sprite;

export namespace CSprite
{
    char (__cdecl* CalcScreenCoorsMaxMin)(RwV3d* in, RwV3d* out, float* w, float* h, char checkMaxVisible, char checkMinVisible) = nullptr;
    char (__cdecl* CalcScreenCoorsMax)(RwV3d* in, RwV3d* out, float* w, float* h, char checkMaxVisible) = nullptr;

    char __cdecl CalcScreenCoors(RwV3d* in, RwV3d* out, float* w, float* h, char checkMaxVisible, char checkMinVisible)
    {
        if (CalcScreenCoorsMaxMin)
            return CalcScreenCoorsMaxMin(in, out, w, h, checkMaxVisible, checkMinVisible);
        else if (CalcScreenCoorsMax)
            return CalcScreenCoorsMax(in, out, w, h, checkMaxVisible);
        return 0;
    }

    void (__cdecl* FlushSpriteBuffer)() = nullptr;

    void (__cdecl* RenderOneXLUSprite_Rotate_Aspect)(float x, float y, float z, float halfWidth, float halfHeight,
                        unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw,
                        float rotate, unsigned char aspect) = nullptr;

    void (__cdecl* RenderBufferedOneXLUSprite_Rotate_Aspect)(float x, float y, float z, float halfWidth, float halfHeight,
                        unsigned char red, unsigned char green, unsigned char blue, short alpha, float rhw,
                        float rotate, unsigned char aspect) = nullptr;
}