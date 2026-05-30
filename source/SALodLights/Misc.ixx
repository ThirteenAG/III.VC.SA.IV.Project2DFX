module;

#include <stdafx.h>
#include "rw.h"

export module Misc;

import Entity;

using RwV3D = RwV3d;

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

export class CPhysical : public CEntity
{
private:
    float				pad1; // 56
    int					__pad2; // 60

    unsigned int		b0x01 : 1; // 64
    unsigned int		bApplyGravity : 1;
    unsigned int		bDisableFriction : 1;
    unsigned int		bCollidable : 1;
    unsigned int		b0x10 : 1;
    unsigned int		bDisableMovement : 1;
    unsigned int		b0x40 : 1;
    unsigned int		b0x80 : 1;

    unsigned int		bSubmergedInWater : 1; // 65
    unsigned int		bOnSolidSurface : 1;
    unsigned int		bBroken : 1;
    unsigned int		b0x800 : 1; // ref @ 0x6F5CF0
    unsigned int		b0x1000 : 1;//
    unsigned int		b0x2000 : 1;//
    unsigned int		b0x4000 : 1;//
    unsigned int		b0x8000 : 1;//

    unsigned int		b0x10000 : 1; // 66
    unsigned int		b0x20000 : 1; // ref @ CPhysical__processCollision
    unsigned int		bBulletProof : 1;
    unsigned int		bFireProof : 1;
    unsigned int		bCollisionProof : 1;
    unsigned int		bMeeleProof : 1;
    unsigned int		bInvulnerable : 1;
    unsigned int		bExplosionProof : 1;

    unsigned int		b0x1000000 : 1; // 67
    unsigned int		bAttachedToEntity : 1;
    unsigned int		b0x4000000 : 1;
    unsigned int		bTouchingWater : 1;
    unsigned int		bEnableCollision : 1;
    unsigned int		bDestroyed : 1;
    unsigned int		b0x40000000 : 1;
    unsigned int		b0x80000000 : 1;

    CVector				m_vecLinearVelocity; // 68
    CVector				m_vecAngularVelocity; // 80
    CVector				m_vecCollisionLinearVelocity; // 92
    CVector				m_vecCollisionAngularVelocity; // 104
    CVector				m_vForce;							// 0x74
    CVector				m_vTorque;							// 0x80
    float				fMass;								// 0x8C
    float				fTurnMass;							// 0x90
    float				m_fVelocityFrequency;					// 0x94
    float				m_fAirResistance;						// 0x98
    float				m_fElasticity;						// 0x9C
    float				m_fBuoyancyConstant;					// 0xA0

    CVector				vecCenterOfMass;					// 0xA4
    DWORD				dwUnk1;								// 0xB0
    void* unkCPtrNodeDoubleLink;				// 0xB4
    BYTE				byUnk : 8;								// 0xB8
    BYTE				byCollisionRecords : 8;					// 0xB9
    BYTE				byUnk2 : 8;								// 0xBA (Baracus)
    BYTE				byUnk3 : 8;								// 0xBB

    float				pad4[6];								// 0xBC

    float				fDistanceTravelled;					// 0xD4
    float				fDamageImpulseMagnitude;				// 0xD8
    CEntity* damageEntity;						// 0xDC
    BYTE				pad2[28];								// 0xE0
    CEntity* pAttachedEntity;					// 0xFC
    CVector				m_vAttachedPosition;				// 0x100
    CVector				m_vAttachedRotation;				// 0x10C
    BYTE				pad3[20];								// 0x118
    float				fLighting;							// 0x12C col lighting? CPhysical::GetLightingFromCol
    float				fLighting_2;							// 0x130 added to col lighting in CPhysical::GetTotalLighting
    class CRealTimeShadow* m_pShadow;							// 0x134

public:
    inline class CRealTimeShadow* GetRealTimeShadow()
    {
        return m_pShadow;
    }
    inline CVector& GetLinearVelocity()
    {
        return m_vecLinearVelocity;
    }

    inline void						SetRealTimeShadow(class CRealTimeShadow* pShadow)
    {
        m_pShadow = pShadow;
    }
};

export struct CObjectInfo
{
    float  m_fMass;
    float  m_fTurnMass;
    float  m_fAirResistance;
    float  m_fElasticity;
    float  m_fBuoyancyConstant;
    float  m_fUprootLimit;
    float  m_fColDamageMultiplier;
    BYTE   m_bColDamageEffect;
    BYTE   m_bSpecialColResponseCase;
    BYTE   m_bCameraAvoidObject;
    BYTE   m_bCausesExplosion;
    BYTE   m_bFxType;
    BYTE   field_21;
    BYTE   field_22;
    BYTE   field_23;
    RwV3D  m_vFxOffset;
    void* m_pFxSystem;              // CFxSystem
    int    field_34;
    RwV3D  m_vBreakVelocity;
    float  m_fBreakVelocityRand;
    float  m_fSmashMultiplier;
    DWORD  m_dwSparksOnImpact;
};

export struct CObject : public CPhysical
{
    int          field_138;
    char         m_bObjectType;
    char         field_13D;
    char         gap_13E[2];
    int          m_dwObjectFlags;
    char         m_bColDamageEffect;
    char         field_145;
    char         field_146;
    char         field_147;
    char         lastWeaponDamage;
    char         m_bColBrightness;
    char         m_bCarPartModelId;
    char         gap_14B[1];
    BYTE         m_bColorId[4];
    int          field_150;
    float        m_fHealth;
    float        field_158;
    int          m_fScale;
    CObjectInfo* m_pObjectInfo;
    int          field_164;
    WORD         field_168;
    WORD         m_wPaintjobTxd;
    RwTexture* m_pPaintjobTex;
    int          field_170;
    int          field_174;
    float        field_178;
};

export namespace CWeather
{
    GameRef<float> Rain;
    GameRef<float> Foggyness;
    GameRef<float> UnderWaterness;
}

export RwTexture** gpCoronaTexture;

export GameRef<CScene> Scene;

export GameRef<RwGlobals*> RwEngineInstance;

export void RwRenderStateSet(RwRenderState nState, void* pParam)
{
    RwEngineInstance->dOpenDevice.fpRenderStateSet(nState, pParam);
}

export void RwRenderStateGet(RwRenderState nState, void* pParam)
{
    RwEngineInstance->dOpenDevice.fpRenderStateGet(nState, pParam);
}

export namespace CRenderer
{
    GameRef<float> ms_lodDistScale;
}

export namespace CShadows
{
    RwTexture** gpShadowExplosionTex = nullptr;
    bool(__cdecl* StoreStaticShadow)(unsigned int id, unsigned char type, RwTexture* particle, CVector* pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = nullptr;
}

export GameRef<int> gGameState([]() -> int*
{
    auto pattern = hook::pattern("89 35 ? ? ? ? B8 ? ? ? ? 0F A2");
    if (!pattern.empty())
        return *pattern.get_first<int*>(2);
    return nullptr;
});

export void (__fastcall* CreateRwObject)(CEntity* entity, void* edx) = nullptr;

export namespace CStreaming
{
    void (__cdecl* RequestModel)(signed int dwModelId, int flags) = nullptr;
    void (__cdecl* LoadAllRequestedModels)(char bOnlyPriorityRequests) = nullptr;
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