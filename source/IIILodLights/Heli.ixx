module;
#include <cstdint>
#include "rw.h"

export module Heli;

import Entity;
import Vehicle;

enum eHeliNodes
{
    HELI_CHASSIS = 1,
    HELI_TOPROTOR,
    HELI_BACKROTOR,
    HELI_TAIL,
    HELI_TOPKNOT,
    HELI_SKID_LEFT,
    HELI_SKID_RIGHT,
    NUM_HELI_NODES
};

enum
{
    HELI_RANDOM0,
    HELI_RANDOM1,
    HELI_SCRIPT,
    HELI_CATALINA,
    NUM_HELIS
};

enum
{
    HELI_TYPE_RANDOM,
    HELI_TYPE_SCRIPT,
    HELI_TYPE_CATALINA,
};

export class CHeli : public CVehicle
{
public:
    void* m_aHeliNodes[NUM_HELI_NODES];
    int8_t m_heliStatus;
    float m_fSearchLightX;
    float m_fSearchLightY;
    uint32_t m_nExplosionTimer;
    float m_fRotation;
    float m_fAngularSpeed;
    float m_fTargetZ;
    float m_fSearchLightIntensity;
    int8_t m_nHeliId;
    int8_t m_heliType;
    int8_t m_pathState;
    float m_aSearchLightHistoryX[6];
    float m_aSearchLightHistoryY[6];
    uint32_t m_nSearchLightTimer;
    uint32_t m_nShootTimer;
    uint32_t m_nLastShotTime;
    uint32_t m_nBulletDamage;
    float m_fRotorRotation;
    float m_fHeliDustZ[8];
    uint32_t m_nPoliceShoutTimer;
    float m_fTargetOffset;
    bool m_bTestRight;
};

export CHeli** pHelis;
export int16_t* pNumRandomHelis = nullptr;