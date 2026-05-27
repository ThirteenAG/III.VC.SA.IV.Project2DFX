module;

#include <stdafx.h>

export module WplInstance;

export struct WplInstance
{
    float PositionX;
    float PositionY;
    float PositionZ;
    float RotationX;
    float RotationY;
    float RotationZ;
    float RotationW;
    unsigned int ModelNameHash;
    unsigned int Unknown1;
    signed int LODIndex;
    unsigned int Unknown2;
    float Unknown3;
};