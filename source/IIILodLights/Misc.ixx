module;

#include <stdafx.h>
#include "rw.h"

export module Misc;

import Entity;

using RwV3D = RwV3d;

export struct RwIm2DVertex
{
    float x, y, z, rhw;
    uint8_t b, g, r, a;
    float u, v;
};

export enum RwPrimitiveType
{
    rwPRIMTYPENAPRIMTYPE = 0,
    rwPRIMTYPELINELIST = 1,
    rwPRIMTYPEPOLYLINE = 2,
    rwPRIMTYPETRILIST = 3,
    rwPRIMTYPETRISTRIP = 4,
    rwPRIMTYPETRIFAN = 5,
    rwPRIMTYPEPOINTLIST = 6,
};

export enum SShadType
{
    SSHADT_NONE,
    SSHADT_DEFAULT,		// SSHAD_RM_COLORONLY (shad_car, shad_ped, shad_heli, shad_bike, shad_rcbaron, bloodpool_64, wincrack_32, lamp_shad_64)
    SSHADT_INTENSIVE,	// SSHAD_RM_OVERLAP (shad_exp, headlight, headlight1, handman)
    SSHADT_NEGATIVE,	// SSHAD_RM_INVCOLOR
    SSHADT4,			// SSHAD_RM_COLORONLY
    SSHADT5,			// SSHAD_RM_COLORONLY
    SSHADT6,			// SSHAD_RM_COLORONLY
    SSHADT8 = 8			// SSHAD_RM_COLORONLY
};

export enum SShadRenderMode
{
    SSHAD_RM_NONE,		// Shadow type: <0, 0, >8.
    SSHAD_RM_COLORONLY, // Shadow type: 1, 4, 5, 6, 8. The texture alpha is present if the alpha channel is set.
    SSHAD_RM_OVERLAP,	// Shadow type: 2. Any shadow alpha greater than 0 does not make any difference.
    SSHAD_RM_INVCOLOR	// Shadow type: 3. Same tip above goes here.
};

export struct RxObjSpace3dVertex	// sizeof = 0x24
{
    RwV3D objVertex;
    RwV3D objNormal;
    DWORD color;
    float u;
    float v;
};

export enum
{
    PHYSICAL_MAX_COLLISIONRECORDS = 6
};

export class CPhysical : public CEntity
{
public:
    int32_t m_audioEntityId;
    float m_phys_unused1;
    void* m_treadable[2]; // car and ped
    uint32_t m_nLastTimeCollided;
    CVector m_vecMoveSpeed; // velocity
    CVector m_vecTurnSpeed; // angular velocity
    CVector m_vecMoveFriction;
    CVector m_vecTurnFriction;
    CVector m_vecMoveSpeedAvg;
    CVector m_vecTurnSpeedAvg;
    float m_fMass;
    float m_fTurnMass; // moment of inertia
    float m_fForceMultiplier;
    float m_fAirResistance;
    float m_fElasticity;
    float m_fBuoyancy;
    CVector m_vecCentreOfMass;
    void* m_entryInfoList;
    void* m_movingListNode;

    int8_t m_phys_unused2;
    uint8_t m_nStaticFrames;
    uint8_t m_nCollisionRecords;
    bool m_bIsVehicleBeingShifted;
    CEntity* m_aCollisionRecords[PHYSICAL_MAX_COLLISIONRECORDS];

    float m_fDistanceTravelled;

    // damaged piece
    float m_fDamageImpulse;
    CEntity* m_pDamageEntity;
    CVector m_vecDamageNormal;
    int16_t m_nDamagePieceType;

    uint8_t bIsHeavy : 1;
    uint8_t bAffectedByGravity : 1;
    uint8_t bInfiniteMass : 1;
    uint8_t bIsInWater : 1;
    uint8_t m_phy_flagA10 : 1; // unused
    uint8_t m_phy_flagA20 : 1; // unused
    uint8_t bHitByTrain : 1;
    uint8_t bSkipLineCol : 1;

    uint8_t m_nSurfaceTouched;
    int8_t m_nZoneLevel;

    CPhysical(void);
    ~CPhysical(void);

    // from CEntity
    void Add(void);
    void Remove(void);
    CRect GetBoundRect(void);
    void ProcessControl(void);
    void ProcessShift(void);
    void ProcessCollision(void);

    void RemoveAndAdd(void);
    void AddToMovingList(void);
    void RemoveFromMovingList(void);
    void SetDamagedPieceRecord(uint16_t piece, float impulse, CEntity* entity, CVector dir);
    void AddCollisionRecord(CEntity* ent);
    void AddCollisionRecord_Treadable(CEntity* ent);
    bool GetHasCollidedWith(CEntity* ent);
    void RemoveRefsToEntity(CEntity* ent);
    static void PlacePhysicalRelativeToOtherPhysical(CPhysical* other, CPhysical* phys, CVector localPos);

    // get speed of point p relative to entity center
    CVector GetSpeed(const CVector& r)
    {
        return m_vecMoveSpeed + m_vecMoveFriction + CrossProduct(m_vecTurnFriction + m_vecTurnSpeed, r);
    }
    CVector GetSpeed(void) { return GetSpeed(CVector(0.0f, 0.0f, 0.0f)); }
    float GetMass(const CVector& pos, const CVector& dir)
    {
        return 1.0f / (CrossProduct(pos, dir).MagnitudeSqr() / m_fTurnMass +
                          1.0f / m_fMass);
    }
    float GetMassTweak(const CVector& pos, const CVector& dir, float t)
    {
        return 1.0f / (CrossProduct(pos, dir).MagnitudeSqr() / (m_fTurnMass * t) +
                          1.0f / (m_fMass * t));
    }

    const CVector& GetMoveSpeed() { return m_vecMoveSpeed; }
    void SetMoveSpeed(float x, float y, float z)
    {
        m_vecMoveSpeed.x = x;
        m_vecMoveSpeed.y = y;
        m_vecMoveSpeed.z = z;
    }
    void SetMoveSpeed(const CVector& speed)
    {
        m_vecMoveSpeed = speed;
    }
    const CVector& GetTurnSpeed() { return m_vecTurnSpeed; }
    void SetTurnSpeed(float x, float y, float z)
    {
        m_vecTurnSpeed.x = x;
        m_vecTurnSpeed.y = y;
        m_vecTurnSpeed.z = z;
    }
    const CVector& GetCenterOfMass() { return m_vecCentreOfMass; }
    void SetCenterOfMass(float x, float y, float z)
    {
        m_vecCentreOfMass.x = x;
        m_vecCentreOfMass.y = y;
        m_vecCentreOfMass.z = z;
    }
};

export class CObjectInfo
{
public:
    float m_fMass;
    float m_fTurnMass;
    float m_fAirResistance;
    float m_fElasticity;
    float m_fBuoyancy;
    float m_fUprootLimit;
    float m_fCollisionDamageMultiplier;
    uint8_t m_nCollisionDamageEffect;
    uint8_t m_nSpecialCollisionResponseCases;
    bool m_bCameraToAvoidThisObject;
};

export class CObject : public CPhysical
{
public:
    CMatrix m_objectMatrix;
    float m_fUprootLimit;
    int8_t ObjectCreatedBy;
    int8_t bIsPickup : 1;
    int8_t bPickupObjWithMessage : 1;
    int8_t bOutOfStock : 1;
    int8_t bGlassCracked : 1;
    int8_t bGlassBroken : 1;
    int8_t bHasBeenDamaged : 1;
    int8_t bUseVehicleColours : 1;
    int8_t m_nBonusValue;
    float m_fCollisionDamageMultiplier;
    uint8_t m_nCollisionDamageEffect;
    uint8_t m_nSpecialCollisionResponseCases;
    bool m_bCameraToAvoidThisObject;
    uint32_t m_obj_unused1;
    uint32_t m_nEndOfLifeTime;
    int16_t m_nRefModelIndex;
    CEntity* m_pCurSurface;
    CEntity* m_pCollidingEntity;
    int8_t m_colour1, m_colour2;
};

export namespace CWeather
{
    GameRef<float> Rain;
    GameRef<float> Foggyness;
}

export namespace CDraw
{
	GameRef<float> ms_fNearClipZ;
	GameRef<float> ms_fFarClipZ;
}

export namespace CSprite
{
	GameRef<float> m_f2DNearScreenZ;
	GameRef<float> m_f2DFarScreenZ;
}

export RwTexture** gpCoronaTexture;

export GameRef<CScene> Scene;

export GameRef<RwGlobals*> RwEngineInstance;

export bool(__cdecl* RwRenderStateGet)(RwRenderState nState, void* pParam) = nullptr;
export bool(__cdecl* RwRenderStateSet)(RwRenderState nState, void* pParam) = nullptr;

export int (__cdecl*RwIm2DRenderIndexedPrimitive)(
    RwPrimitiveType primType,
    RwIm2DVertex* vertices,
    unsigned int numVertices,
    uint16_t* indices,
    unsigned int numIndices) = nullptr;

enum RsInputDeviceType : int
{
    rsKEYBOARD,
    rsMOUSE,
    rsPAD
};

enum RsEventStatus : int
{
    rsEVENTERROR,
    rsEVENTPROCESSED,
    rsEVENTNOTPROCESSED
};

enum RsEvent : int
{
    rsCAMERASIZE,
    rsCOMMANDLINE,
    rsFILELOAD,
    rsINITDEBUG,
    rsINPUTDEVICEATTACH,
    rsLEFTBUTTONDOWN,
    rsLEFTBUTTONUP,
    rsMOUSEMOVE,
    rsMOUSEWHEELMOVE,
    rsPLUGINATTACH,
    rsREGISTERIMAGELOADER,
    rsRIGHTBUTTONDOWN,
    rsRIGHTBUTTONUP,
    _rs_13,
    _rs_14,
    _rs_15,
    _rs_16,
    _rs_17,
    _rs_18,
    _rs_19,
    _rs_20,
    rsRWINITIALIZE,
    rsRWTERMINATE,
    rsSELECTDEVICE,
    rsINITIALIZE,
    rsTERMINATE,
    rsIDLE,
    rsFRONTENDIDLE,
    rsKEYDOWN,
    rsKEYUP,
    rsQUITAPP,
    rsPADBUTTONDOWN,
    rsPADBUTTONUP,
    rsPADANALOGUELEFT,
    rsPADANALOGUELEFTRESET,
    rsPADANALOGUERIGHT,
    rsPADANALOGUERIGHTRESET,
    rsPREINITCOMMANDLINE,
    rsACTIVATE,
};

using RsInputEventHandler = RsEventStatus(*)(RsEvent event, void* param);

struct RsInputDevice
{
    RsInputDeviceType inputDeviceType;
    bool              used;
    RsInputEventHandler inputEventHandler;
};

export struct RsGlobalType
{
    const char* appName;
    std::int32_t  width;
    std::int32_t  height;
    std::int32_t  maximumWidth;
    std::int32_t  maximumHeight;
    std::int32_t  maxFPS;
    bool          quit;
    void* ps;
    RsInputDevice keyboard;
    RsInputDevice mouse;
    RsInputDevice pad;
};

export GameRef<RsGlobalType> RsGlobal([]() -> RsGlobalType*
    {
        auto pattern = hook::pattern("8B 0D ? ? ? ? 6A ? 6A ? 52");
        if (!pattern.empty())
            return *pattern.get_first<RsGlobalType*>(2);
        return nullptr;
    });

export namespace CRenderer
{
    GameRef<float> ms_lodDistScale;
}

export namespace CShadows
{
    RwTexture** gpShadowExplosionTex = nullptr;
    bool(__cdecl* StoreStaticShadow)(unsigned int id, unsigned char type, RwTexture* particle, CVector* pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = nullptr;
}

export namespace CGeneral
{
    constexpr int MYRAND_MAX = 65535;

    unsigned int __cdecl GetRandomNumber()
    {
        return rand();
    }

    float GetRandomNumberInRange(float low, float high)
    {
        return low + (high - low) * (CGeneral::GetRandomNumber() / float(MYRAND_MAX + 1));
    }

    int32_t GetRandomNumberInRange(int32_t low, int32_t high)
    {
        return low + (high - low) * (CGeneral::GetRandomNumber() / float(MYRAND_MAX + 1));
    }
}

export namespace CStats
{
    GameRef<int> IndustrialPassed;
}