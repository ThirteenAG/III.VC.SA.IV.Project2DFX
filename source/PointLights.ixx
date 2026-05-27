module;

#include <stdafx.h>

export module PointLights;

export enum PointLightType
{
    PLTYPE_POINTLIGHT = 0x0,
    PLTYPE_DIRECTIONAL = 0x1,
    PLTYPE_ANTILIGHT = 0x2,
    PLTYPE_ONLYFOGEFFECT_ALWAYS = 0x3,
    PLTYPE_ONLYFOGEFFECT = 0x4,
    PLTYPE_SMOG = 0x5,
};

enum
{
    FOGEFF_OFF = 0x0,
    FOGEFF_ON = 0x1,
    FOGEFF_ALWAYS = 0x2,
};

export namespace CPointLights
{
    void (__cdecl* AddLightWithEntity)(char type, float x, float y, float z, float x_dir, float y_dir, float z_dir, float radius, float red, float green, float blue, char fogType, char generateExtraShadows, void* entityAffected) = nullptr;
    void (__cdecl* AddLightWithoutEntity)(char lightType, float x, float y, float z, float x_dir, float y_dir, float z_dir, float range, float red, float green, float blue, uint8_t fogType, char generateExtraShadows) = nullptr;

    void __cdecl AddLight(PointLightType type, float x, float y, float z, float x_dir, float y_dir, float z_dir, float radius, float red, float green, float blue, char fogType, char generateExtraShadows, void* entityAffected)
    {
        if (AddLightWithEntity)
            return AddLightWithEntity(type, x, y, z, x_dir, y_dir, z_dir, radius, red, green, blue, fogType, generateExtraShadows, entityAffected);
        else if (AddLightWithoutEntity)
            return AddLightWithoutEntity(type, x, y, z, x_dir, y_dir, z_dir, radius, red, green, blue, fogType, generateExtraShadows);
    }
}