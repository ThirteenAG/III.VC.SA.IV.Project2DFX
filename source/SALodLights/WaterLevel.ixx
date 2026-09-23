module;

#include <stdafx.h>

export module WaterLevel;

export namespace CWaterLevel
{
    // SA supplies two additional optional wave-amplitude outputs.
    bool (__cdecl* GetWaterLevelNoWaves)(float x, float y, float z, float* level, float* bigWaves, float* smallWaves) = nullptr;
}
