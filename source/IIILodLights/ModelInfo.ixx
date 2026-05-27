module;
#include <cstdint>
#include "rw.h"

export module ModelInfo;

enum
{
    EFFECT_LIGHT,
    EFFECT_PARTICLE,
    EFFECT_ATTRACTOR
};

enum
{
    LIGHT_ON,
    LIGHT_ON_NIGHT,
    LIGHT_FLICKER,
    LIGHT_FLICKER_NIGHT,
    LIGHT_FLASH1,
    LIGHT_FLASH1_NIGHT,
    LIGHT_FLASH2,
    LIGHT_FLASH2_NIGHT,
    LIGHT_FLASH3,
    LIGHT_FLASH3_NIGHT,
    LIGHT_RANDOM_FLICKER,
    LIGHT_RANDOM_FLICKER_NIGHT,
    LIGHT_SPECIAL,
    LIGHT_BRIDGE_FLASH1,
    LIGHT_BRIDGE_FLASH2,
};

enum
{
    ATTRACTORTYPE_ICECREAM,
    ATTRACTORTYPE_STARE
};

enum
{
    LIGHTFLAG_LOSCHECK = 1,
    // same order as CPointLights flags, must start at 2
    LIGHTFLAG_FOG_NORMAL = 2, // can have light and fog
    LIGHTFLAG_FOG_ALWAYS = 4, // fog only
    LIGHTFLAG_FOG = (LIGHTFLAG_FOG_NORMAL | LIGHTFLAG_FOG_ALWAYS)
};

class C2dEffect
{
public:
    struct Light
    {
        float dist;
        float range; // of pointlight
        float size;
        float shadowSize;
        uint8_t lightType; // LIGHT_
        uint8_t roadReflection;
        uint8_t flareType;
        uint8_t shadowIntensity;
        uint8_t flags; // LIGHTFLAG_
        RwTexture* corona;
        RwTexture* shadow;
    };
    struct Particle
    {
        int particleType;
        CVector dir;
        float scale;
    };
    struct Attractor
    {
        CVector dir;
        int8_t type;
        uint8_t probability;
    };

    CVector pos;
    CRGBA col;
    uint8_t type;
    union
    {
        Light light;
        Particle particle;
        Attractor attractor;
    };

};

struct CColBox
{
    CVector min;
    CVector max;
    uint8_t surface;
    uint8_t piece;
};

struct CColSphere
{
    CVector center;
    float radius;
    uint8_t surface;
    uint8_t piece;
};

struct CColLine
{
    CVector p0;
    int pad0;
    CVector p1;
    int pad1;
};

struct CompressedVector
{
    float x, y, z;
};

struct CColTriangle
{
    uint16_t a;
    uint16_t b;
    uint16_t c;
    uint8_t surface;
};

struct CColTrianglePlane
{
    CVector normal;
    float dist;
    uint8_t dir;
};

struct CColModel
{
    CColSphere boundingSphere;
    CColBox boundingBox;
    int16_t numSpheres;
    int16_t numLines;
    int16_t numBoxes;
    int16_t numTriangles;
    int32_t level;
    bool ownsCollisionVolumes;
    CColSphere* spheres;
    CColLine* lines;
    CColBox* boxes;
    CompressedVector* vertices;
    CColTriangle* triangles;
    CColTrianglePlane* trianglePlanes;
};

#define MAX_MODEL_NAME (24)

export class CBaseModelInfo
{
    void* vmt;
public:
    char m_name[MAX_MODEL_NAME];
    CColModel* m_colModel;
    C2dEffect* m_twodEffects;
    int16_t m_objectId;
    uint16_t m_refCount;
    int16_t m_txdSlot;
    uint8_t m_type;
    uint8_t m_num2dEffects;
    bool m_bOwnsColModel;
};

export class CSimpleModelInfo : public CBaseModelInfo
{
public:
    void* m_atomics[3];
    float m_lodDistances[3];
    uint8_t m_numAtomics;
    uint8_t m_alpha;
    uint16_t m_firstDamaged : 2;
    uint16_t m_normalCull : 1;
    uint16_t m_isDamaged : 1;
    uint16_t m_isBigBuilding : 1;
    uint16_t m_noFade : 1;
    uint16_t m_drawLast : 1;
    uint16_t m_additive : 1;
    uint16_t m_isSubway : 1;
    uint16_t m_ignoreLight : 1;
    uint16_t m_noZwrite : 1;
};

export namespace CModelInfo
{
    CBaseModelInfo*** pp_modelInfoPtrs = nullptr;
    CBaseModelInfo* (__cdecl* GetModelInfo)(const char* name, int* id) = nullptr;
    C2dEffect* (__fastcall* Get2dEffect)(CBaseModelInfo* minfo, void* edx, int32_t i) = nullptr;
}