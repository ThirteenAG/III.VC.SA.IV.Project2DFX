module;

#include <stdafx.h>

export module WaterLevel;

export namespace CWaterLevel
{
    bool (__cdecl* GetWaterLevelNoWaves)(float x, float y, float z, float* level) = nullptr;
}
