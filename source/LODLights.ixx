module;

#define NOMINMAX
#include <stdafx.h>
#include <unordered_map>
#include <map>
#include "DistantLightLogic.hpp"
#include <d3d9.h>
#include "LightVolumesShader.hpp"

export module LODLights;

import ComVars;
import Entity;
import Sprite;
import Camera;
import Timer;
import Clock;
import Misc;
import Game;
import LamppostInfo;
import Timecycle;
import PointLights;

#include "LightVolumes.hpp"

// Native phase readers include cheats and VC storm/SA riot flashing.
// Only two calls per frame; object classification is cached at map registration.
export namespace CTrafficLights
{
    uint8_t (__cdecl* LightForCars1)() = nullptr;
    uint8_t (__cdecl* LightForCars2)() = nullptr;
    uint8_t (__cdecl* LightForCars1_Visual)() = nullptr;
    uint8_t (__cdecl* LightForCars2_Visual)() = nullptr;
    int (__cdecl* FindTrafficLightType)(CEntity*) = nullptr;
    unsigned TimeDivisor = 1;

    uint8_t Type(CEntity* entity, const CVector& forward)
    {
        return FindTrafficLightType ? static_cast<uint8_t>(FindTrafficLightType(entity)) : DistantLightLogic::TrafficGroup(forward.x, forward.y);
    }
    uint8_t Phase(unsigned group)
    {
        auto visual = group == 1 ? LightForCars1_Visual : LightForCars2_Visual;
        auto logic = group == 1 ? LightForCars1 : LightForCars2;
        if (visual) return visual();
        if (logic) return logic();
        return DistantLightLogic::TrafficPhase(CTimer::m_snTimeInMilliseconds, group, TimeDivisor);
    }
    bool StopForCars(unsigned group)
    {
        auto logic = group == 1 ? LightForCars1 : LightForCars2;
        // Native driving phases let traffic through flashing storm/riot lights.
        if (logic) return logic() != 0;
        auto phase = Phase(group);
        return phase == 1 || phase == 2;
    }
}

// How the game draws the lights it owns, set by each plugin. Near a lamp the game draws its own corona and light pool;
// ours take over where the game's fade out and look like the game's there.
export namespace GameLights
{
    GameRef<float> FOV;
    // Timecycle sprite brightness and size to the units the game's light formulas use
    float fSpriteBrightnessScale = 1.0f;
    float fSpriteSizeScale = 1.0f;
    // Lamp corona pulled toward the camera
    float fLampPull = 1.5f;
    // Traffic light corona range
    float fTrafficRange = 50.0f;
    // Lamp light pool range (the game's 40 m times ShadowDistance) and its colour factor
    float fPoolRange = 40.0f;
    float fPoolColour = 1.0f / 256.0f;
    // Lamps follow the day/night balance (SA) instead of switching on at 19:00
    bool bDayNightBalance = false;

    // 0..1 brightness of the game's lamps at this time
    float LampFade()
    {
        const int hours = CClock::ms_nGameClockHours;
        if (!bDayNightBalance)
            return hours > 18 || hours < 7 ? 1.0f : 0.0f;
        const float m = hours * 60.0f + CClock::ms_nGameClockMinutes;
        if (m < 360.0f) return 1.0f;
        if (m < 420.0f) return (420.0f - m) / 60.0f;
        if (m < 1200.0f) return 0.0f;
        if (m >= 1260.0f) return 1.0f;
        return 1.0f - (1260.0f - m) / 60.0f;
    }
}

export class CRegisteredCorona
{
public:
    CVector        Coordinates;            // Where is it exactly.
    uint32_t       Identifier;             // Should be unique for each corona. Address or something (0 = empty)
    uint8_t        nTexType;               // Corona sprite index into gpCoronaTexture[], resolved at render time
    float          Size;                   // How big is this fellow
    float          NormalAngle;            // Is corona normal (if relevant) facing the camera?
    float          Range;                  // How far away is this guy still visible
    float          PullTowardsCam;         // How far away is the z value pulled towards camera.
    float          HeightAboveGround;      // Stired so that we don't have to do a ProcessVerticalLine every frame
    // The following fields are used for trails behind coronas (glowy lights)
    float          FadeSpeed;              // The speed the corona fades in and out ##SA##
    uint8_t        Red, Green, Blue;       // Rendering colour.
    uint8_t        Intensity;              // 255 = full
    uint8_t        FadedIntensity;         // Intensity that lags behind the given intenisty and fades out if the LOS is blocked
    uint8_t        RegisteredThisFrame;    // Has this guy been registered by game code this frame
    uint8_t        FlareType;              // What type of flare to render
    uint8_t        ReflectionType;         // What type of reflection during wet weather
    uint8_t        LOSCheck : 1;           // Do we check the LOS or do we render at the right Z value
    uint8_t        OffScreen : 1;          // Set by the rendering code to be used by the update code
    uint8_t        JustCreated;            // If this guy has been created this frame we won't delete it (It hasn't had the time to get its OffScreen cleared) ##SA removed from packed byte ##
    uint8_t        NeonFade : 1;           // Does the guy fade out when closer to cam
    uint8_t        OnlyFromBelow : 1;      // This corona is only visible if the camera is below it. ##SA##
    uint8_t        bHasValidHeightAboveGround : 1;
    uint8_t        WhiteCore : 1;          // This corona rendered with a small white core.
    uint8_t        bIsAttachedToEntity : 1;
    CEntity* pEntityAttachedTo;
    CoronaPredicate pPredicate = nullptr;
    // The game's own corona of the same light: its range and how far it has faded in
    float HandoverRange = 0.0f;
    float HandoverGame = 0.0f;

public:
    CRegisteredCorona()
        : Identifier(0), nTexType(0), pEntityAttachedTo(nullptr)
    {
    }

    void Update()
    {
        if (!RegisteredThisFrame)
        {
            Intensity = 0;
        }
        if (!Intensity && !JustCreated)
        {
            Identifier = 0;
        }
        JustCreated = 0;
        RegisteredThisFrame = 0;
    }
};

export class CLODLightsLinkedListNode
{
private:
    CLODLightsLinkedListNode* pNext;
    CLODLightsLinkedListNode* pPrev;
    CRegisteredCorona* pEntry;

private:
    inline void Remove()
    {
        pNext->pPrev = pPrev; pPrev->pNext = pNext; pNext = nullptr;
    }

public:
    inline void Init()
    {
        pNext = pPrev = this;
    }
    inline void Add(CLODLightsLinkedListNode* pHead)
    {
        if (pNext) Remove();
        pNext = pHead->pNext; pPrev = pHead; pHead->pNext->pPrev = this; pHead->pNext = this;
    }
    inline void SetEntry(CRegisteredCorona* pEnt)
    {
        pEntry = pEnt;
    }
    inline CRegisteredCorona* GetFrom()
    {
        return pEntry;
    }
    inline CLODLightsLinkedListNode* GetNextNode()
    {
        return pNext;
    }
    inline CLODLightsLinkedListNode* GetPrevNode()
    {
        return pPrev;
    }

    inline CLODLightsLinkedListNode* First()
    {
        return pNext == this ? nullptr : pNext;
    }
};

export class CLODLights
{
private:
    using CoronaMap = std::unordered_map<unsigned int, CLODLightsLinkedListNode*>;
    static inline CoronaMap UsedMap;
    static inline std::vector<CoronaMap::node_type> FreeMapNodes;

    static void RecycleMapEntry(CoronaMap::iterator it)
    {
        if (it != UsedMap.end())
            FreeMapNodes.push_back(UsedMap.extract(it));
    }
    static void RecycleMapEntry(unsigned int id) { RecycleMapEntry(UsedMap.find(id)); }
    static void InsertMapEntry(unsigned int id, CLODLightsLinkedListNode* entry)
    {
        auto node = std::move(FreeMapNodes.back());
        FreeMapNodes.pop_back();
        node.key() = id;
        node.mapped() = entry;
        UsedMap.insert(std::move(node));
    }
    static void InitMap(size_t capacity)
    {
        UsedMap.clear();
        FreeMapNodes.clear();
        UsedMap.reserve(capacity);
        FreeMapNodes.reserve(capacity);
        // Retain actual map nodes: some standard memory pools return empty
        // chunks to the heap, which would reintroduce per-frame allocations.
        for (size_t i = 0; i < capacity; ++i)
            UsedMap.emplace(static_cast<unsigned int>(i), nullptr);
        while (!UsedMap.empty())
            RecycleMapEntry(UsedMap.begin());
    }
    static inline CLODLightsLinkedListNode FreeList, UsedList;
    static inline std::vector<CLODLightsLinkedListNode> aLinkedList;
    static inline std::vector<CRegisteredCorona> aCoronas;
    static inline uint8_t CurrentFrameStamp = 1;
    static inline std::vector<size_t> TrafficLampIndices;
    static inline const CLamppostInfo* TrafficLampData = nullptr;
    static inline size_t TrafficLampCount = size_t(-1);
    static inline CLODLightsLinkedListNode* pFarthestNode = nullptr;
    static inline float fFarthestDistSq = 0.0f;

    struct RenderBatch
    {
        RwRaster* m_pRaster;
        std::vector<RwIm2DVertex> m_aVertices;

        RenderBatch(RwRaster* raster) : m_pRaster(raster) { m_aVertices.reserve(6 * 1024); }
        RenderBatch() : RenderBatch(nullptr) {}

        void Clear()
        {
            m_aVertices.clear();
            m_pRaster = nullptr;
        }

        void AddOneXLUSpriteToBuffer_Rotate_Aspect(float x, float y, float z, float w, float h, uint8_t r, uint8_t g, uint8_t b, int16_t intens, float recipz, float rotation, uint8_t a)
        {
            float c = rotation == 0.0f ? 1.0f : cos(rotation);
            float s = rotation == 0.0f ? 0.0f : sin(rotation);

            float xs[4], ys[4], us[4], vs[4];
            int i;

            if (z < 3.0f)
            {
                if (z < 1.5f)
                    return;
                int f = (z - 1.5f) / 1.5f * 255;
                r = f * r >> 8;
                g = f * g >> 8;
                b = f * b >> 8;
                intens = f * intens >> 8;
            }

            xs[0] = x + w * (-c - s); ys[0] = y + h * (-c + s); us[0] = 0.0f; vs[0] = 0.0f; // TL
            xs[1] = x + w * (+c - s); ys[1] = y + h * (-c - s); us[1] = 1.0f; vs[1] = 0.0f; // TR
            xs[2] = x + w * (-c + s); ys[2] = y + h * (+c + s); us[2] = 0.0f; vs[2] = 1.0f; // BL
            xs[3] = x + w * (+c + s); ys[3] = y + h * (+c - s); us[3] = 1.0f; vs[3] = 1.0f; // BR

            if (xs[0] < 0.0f && xs[1] < 0.0f && xs[2] < 0.0f && xs[3] < 0.0f)
                return;
            if (ys[0] < 0.0f && ys[1] < 0.0f && ys[2] < 0.0f && ys[3] < 0.0f)
                return;
            if (xs[0] > RsGlobal->width && xs[1] > RsGlobal->width &&
                xs[2] > RsGlobal->width && xs[3] > RsGlobal->width)
                return;
            if (ys[0] > RsGlobal->height && ys[1] > RsGlobal->height &&
                ys[2] > RsGlobal->height && ys[3] > RsGlobal->height)
                return;

            float screenz = *CSprite::m_f2DNearScreenZ +
                (z - *CDraw::ms_fNearClipZ) * (*CSprite::m_f2DFarScreenZ - *CSprite::m_f2DNearScreenZ) * *CDraw::ms_fFarClipZ /
                ((*CDraw::ms_fFarClipZ - *CDraw::ms_fNearClipZ) * z);

            uint8_t cr = r * intens >> 8;
            uint8_t cg = g * intens >> 8;
            uint8_t cb = b * intens >> 8;

            static constexpr int order[6] = { 0, 1, 2, 1, 3, 2 };

            // Bound each draw and retain its storage even at the corona limit.
            // All batches use additive blending with depth writes disabled.
            if (m_aVertices.size() == 6 * 1024)
                Render();
            const size_t base = m_aVertices.size();
            m_aVertices.resize(base + 6);

            for (i = 0; i < 6; i++)
            {
                auto& vert = m_aVertices[base + i];
                vert.x = xs[order[i]];
                vert.y = ys[order[i]];
                vert.z = screenz;
                vert.rhw = recipz;
                vert.r = cr;
                vert.g = cg;
                vert.b = cb;
                vert.a = a;
                vert.u = us[order[i]];
                vert.v = vs[order[i]];
            }
        }

        void Render()
        {
            if (m_aVertices.empty() || m_pRaster == nullptr)
                return;

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, m_pRaster);
            RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, m_aVertices.data(), m_aVertices.size());

            m_aVertices.clear();
        }
    };

    static inline std::vector<RenderBatch> m_RenderBatches{};

public:
    static int& bChangeBrightnessImmediately;
    static float& ScreenMult;

public:
    static void RegisterCoronaInternal(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, int coronaType, unsigned char flareType, unsigned char reflectionType, unsigned char LOSCheck, unsigned char unused, float normalAngle, bool bNeonFade, float PullTowardsCam, bool bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool bWhiteCore, CoronaPredicate pPredicate = nullptr, float handoverRange = 0.0f, float handoverGame = 0.0f)
    {
        UNREFERENCED_PARAMETER(unused);
        UNREFERENCED_PARAMETER(bFadeIntensity);
        UNREFERENCED_PARAMETER(pAttachTo);

        // The game destroys and recreates gpCoronaTexture[] when a game is
        // restarted (CCoronas::Shutdown/Init), so only the type index is kept
        // here; the texture itself is resolved every frame in RenderBuffered.
        if (coronaType < 0 || coronaType >= 9)
            return;

        const CVector* pCamPos = GetCamPos();
        const float dx = pCamPos->x - Position.x;
        const float dy = pCamPos->y - Position.y;
        const float rangeSq = Range * Range;
        const float dist2DSq = dx * dx + dy * dy;

        if (rangeSq < dist2DSq)
            return;

        if (bNeonFade)
        {
            const float distSq = dist2DSq + (pCamPos->z - Position.z) * (pCamPos->z - Position.z);
            const float fDistFromCam = std::sqrt(distSq);

            if (fDistFromCam < 35.0f)
                return;
            if (fDistFromCam < 50.0f)
                A = static_cast<unsigned char>(A * ((fDistFromCam - 35.0f) * (2.0f / 3.0f)));
        }

        CRegisteredCorona* pSuitableSlot = nullptr;
        auto it = UsedMap.find(nID);

        if (it != UsedMap.end())
        {
            pSuitableSlot = it->second->GetFrom();

            if (pSuitableSlot->Intensity == 0 && A == 0)
            {
                pSuitableSlot->Identifier = 0;
                it->second->Add(&FreeList);
                RecycleMapEntry(it);
                return;
            }
        }
        else
        {
            if (!A)
                return;

            auto pNewEntry = FreeList.First();
            if (!pNewEntry)
            {
                // Validate cached farthest node
                if (pFarthestNode)
                {
                    auto* pCorona = pFarthestNode->GetFrom();
                    if (!pCorona->Identifier)
                    {
                        pFarthestNode = nullptr;
                        fFarthestDistSq = 0.0f;
                    }
                    else
                    {
                        const float cdx = pCamPos->x - pCorona->Coordinates.x;
                        const float cdy = pCamPos->y - pCorona->Coordinates.y;
                        fFarthestDistSq = cdx * cdx + cdy * cdy;
                    }
                }

                // Rebuild cache if missing
                if (!pFarthestNode)
                {
                    for (auto pNode = UsedList.First(); pNode && pNode != &UsedList; pNode = pNode->GetNextNode())
                    {
                        auto* pCorona = pNode->GetFrom();
                        if (!pCorona->Identifier)
                            continue;
                        const float cdx = pCamPos->x - pCorona->Coordinates.x;
                        const float cdy = pCamPos->y - pCorona->Coordinates.y;
                        const float dSq = cdx * cdx + cdy * cdy;
                        if (dSq > fFarthestDistSq)
                        {
                            fFarthestDistSq = dSq;
                            pFarthestNode = pNode;
                        }
                    }
                }

                // Only evict if cached farthest is farther than incoming
                if (pFarthestNode && fFarthestDistSq > dist2DSq)
                {
                    const unsigned int evictId = pFarthestNode->GetFrom()->Identifier;
                    pFarthestNode->GetFrom()->Identifier = 0;
                    RecycleMapEntry(evictId);
                    pFarthestNode->Add(&FreeList);
                    pFarthestNode = nullptr;  // Invalidate cache after eviction
                    fFarthestDistSq = 0.0f;
                    pNewEntry = FreeList.First();
                }

                if (!pNewEntry)
                    return;
            }

            pSuitableSlot = pNewEntry->GetFrom();
            pNewEntry->Add(&UsedList);
            InsertMapEntry(nID, pNewEntry);

            pSuitableSlot->FadedIntensity = A;
            pSuitableSlot->OffScreen = true;
            pSuitableSlot->JustCreated = true;
            pSuitableSlot->Identifier = nID;
        }

        pSuitableSlot->Red = R;
        pSuitableSlot->Green = G;
        pSuitableSlot->Blue = B;
        pSuitableSlot->Intensity = A;
        pSuitableSlot->Coordinates = Position;
        pSuitableSlot->Size = Size;
        pSuitableSlot->NormalAngle = normalAngle;
        pSuitableSlot->Range = Range;
        pSuitableSlot->nTexType = static_cast<uint8_t>(coronaType);
        pSuitableSlot->FlareType = flareType;
        pSuitableSlot->ReflectionType = reflectionType;
        pSuitableSlot->LOSCheck = LOSCheck;
        pSuitableSlot->RegisteredThisFrame = CurrentFrameStamp;
        pSuitableSlot->PullTowardsCam = PullTowardsCam;
        pSuitableSlot->FadeSpeed = FadeSpeed;

        pSuitableSlot->NeonFade = bNeonFade;
        pSuitableSlot->OnlyFromBelow = bOnlyFromBelow;
        pSuitableSlot->WhiteCore = bWhiteCore;

        pSuitableSlot->bIsAttachedToEntity = false;
        pSuitableSlot->pEntityAttachedTo = nullptr;

        pSuitableSlot->pPredicate = pPredicate;
        pSuitableSlot->HandoverRange = handoverRange;
        pSuitableSlot->HandoverGame = handoverGame;
    }

    // Explicit removal must work regardless of camera position/range. Registering
    // a zero-radius corona at the origin is rejected by the distance check.
    static void UnregisterCorona(unsigned int nID)
    {
        auto it = UsedMap.find(nID);
        if (it == UsedMap.end()) return;
        auto node = it->second;
        if (pFarthestNode == node)
        {
            pFarthestNode = nullptr;
            fFarthestDistSq = 0.0f;
        }
        node->GetFrom()->Identifier = 0;
        node->GetFrom()->Intensity = 0;
        node->Add(&FreeList);
        RecycleMapEntry(it);
    }

    static void TouchCorona(unsigned int nID)
    {
        auto it = UsedMap.find(nID);
        if (it != UsedMap.end())
            it->second->GetFrom()->RegisteredThisFrame = CurrentFrameStamp;
    }

    static void RegisterCorona(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, int coronaType, unsigned char flareType, bool enableReflection, bool checkObstacles, int unused, float normalAngle, bool longDistance, float nearClip, unsigned char bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool reflectionDelay, CoronaPredicate pPredicate = nullptr, float handoverRange = 0.0f, float handoverGame = 0.0f)
    {
        RegisterCoronaInternal(nID, pAttachTo, R, G, B, A, Position, Size, Range, coronaType, flareType, enableReflection, checkObstacles, unused, normalAngle, longDistance, nearClip, bFadeIntensity, FadeSpeed, bOnlyFromBelow, reflectionDelay, pPredicate, handoverRange, handoverGame);
    }

    static void Update()
    {
        auto pNode = UsedList.First();
        if (pNode)
        {
            while (pNode != &UsedList)
            {
                unsigned int nIndex = pNode->GetFrom()->Identifier;
                auto pNext = pNode->GetNextNode();

                pNode->GetFrom()->Update();

                // Did it become invalid?
                if (!pNode->GetFrom()->Identifier)
                {
                    // Remove from used list
                    pNode->Add(&FreeList);
                    RecycleMapEntry(nIndex);
                }

                pNode = pNext;
            }
        }
    }

    static void Init(int numCoronas)
    {
        if (aCoronas.size() != numCoronas)
        {
            aLinkedList.resize(numCoronas);
            aCoronas.resize(numCoronas);
            InitMap(numCoronas);
            TrafficLampIndices.reserve(numCoronas);
            if (m_RenderBatches.empty())
            {
                m_RenderBatches.reserve(9);
                for (int i = 0; i < 9; ++i)
                    m_RenderBatches.emplace_back();
            }

            // Initialise the lists
            FreeList.Init();
            UsedList.Init();
            pFarthestNode = nullptr;
            fFarthestDistSq = 0.0f;

            for (size_t i = 0; i < aLinkedList.size(); i++)
            {
                aLinkedList[i].Add(&FreeList);
                aLinkedList[i].SetEntry(&aCoronas[i]);
            }
        }
    }

    static void Shutdown()
    {
        PoolCache.clear();
        PoolAxes.clear();
        PoolAxisData = nullptr;
        aPools.clear();
        TrafficLampData = nullptr;
        TrafficLampCount = size_t(-1);
        TrafficLampIndices.clear();
        // Drop every registered corona. The game runs CCoronas::Shutdown
        // (which destroys gpCoronaTexture[]) shortly after this when a game
        // is restarted, so nothing stale may be left behind to render.
        while (!UsedMap.empty())
            RecycleMapEntry(UsedMap.begin());
        FreeList.Init();
        UsedList.Init();
        pFarthestNode = nullptr;
        fFarthestDistSq = 0.0f;

        for (size_t i = 0; i < aLinkedList.size(); i++)
        {
            aLinkedList[i].Init();
            aLinkedList[i].Add(&FreeList);
            aLinkedList[i].SetEntry(&aCoronas[i]);
        }

        for (auto& corona : aCoronas)
        {
            corona.Identifier = 0;
            corona.Intensity = 0;
            corona.JustCreated = 0;
            corona.RegisteredThisFrame = 0;
        }
    }

    static void RenderBuffered(bool bOnlyModelLamps = false)
    {
        const int nWidth = Scene->m_pRwCamera->frameBuffer->width;
        const int nHeight = Scene->m_pRwCamera->frameBuffer->height;
        const float farPlane = Scene->m_pRwCamera->farPlane;
        const CVector* pCamPos = GetCamPos();
        const float fogyness = CWeather::Foggyness;
        const float screenWidth = static_cast<float>(nWidth);
        const float screenHeight = static_cast<float>(nHeight);

        const RwMatrix& viewMatrix = Scene->m_pRwCamera->viewMatrix;
        const float vmRightX = viewMatrix.right.x, vmRightY = viewMatrix.right.y, vmRightZ = viewMatrix.right.z;
        const float vmUpX = viewMatrix.up.x, vmUpY = viewMatrix.up.y, vmUpZ = viewMatrix.up.z;
        const float vmAtX = viewMatrix.at.x, vmAtY = viewMatrix.at.y, vmAtZ = viewMatrix.at.z;
        const float vmPosX = viewMatrix.pos.x, vmPosY = viewMatrix.pos.y, vmPosZ = viewMatrix.pos.z;

        RwRaster* pLastRaster = nullptr;
        RenderBatch* pCurRenderBatch = nullptr;
        bool bLastZTestEnable = true;

        void* oldZWrite = nullptr;
        void* oldVertexAlpha = nullptr;
        void* oldSrcBlend = nullptr;
        void* oldDstBlend = nullptr;
        void* oldZTest = nullptr;
        void* oldCullMode = nullptr;
        void* oldAlphaTestFunc = nullptr;
        void* oldAlphaTestRef = nullptr;
        void* oldTextureRaster = nullptr;
        void* oldFogEnable = nullptr;

        RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &oldTextureRaster);
        RwRenderStateGet(rwRENDERSTATEFOGENABLE, &oldFogEnable);
        if (bOnlyModelLamps) RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);

        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &oldZWrite);
        RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &oldVertexAlpha);
        RwRenderStateGet(rwRENDERSTATESRCBLEND, &oldSrcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, &oldDstBlend);
        RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &oldZTest);
        RwRenderStateGet(rwRENDERSTATECULLMODE, &oldCullMode);
        RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTION, &oldAlphaTestFunc);
        RwRenderStateGet(rwRENDERSTATEALPHATESTFUNCTIONREF, &oldAlphaTestRef);

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1); // rwCULLMODECULLNONE
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)7); // rwALPHATESTFUNCTIONGREATEREQUAL
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);

        for (auto& batch : m_RenderBatches)
            batch.Clear();


        auto SelectBatch = [](RwRaster* pRaster) -> RenderBatch*
        {
            for (auto& batch : m_RenderBatches)
                if (batch.m_pRaster == pRaster)
                    return &batch;

            for (auto& batch : m_RenderBatches)
                if (batch.m_pRaster == nullptr)
                {
                    batch.m_pRaster = pRaster;
                    return &batch;
                }

            return &m_RenderBatches.emplace_back(pRaster);
        };

        for (auto pNode = UsedList.First(); pNode && pNode != &UsedList; pNode = pNode->GetNextNode())
        {
            auto& corona = *pNode->GetFrom();
            if (!corona.Identifier || corona.Intensity == 0)
                continue;

            // Model lamps draw beside their meshes, before subsequent clouds/fog.
            const bool modelLamp = (corona.Identifier & 0xFFFF0000u) == 0x7C000000u;
            if (modelLamp != bOnlyModelLamps)
                continue;

            if (corona.pPredicate && !corona.pPredicate())
                continue;

            const float worldX = corona.Coordinates.x;
            const float worldY = corona.Coordinates.y;
            const float worldZ = corona.Coordinates.z;

            const float viewX = worldX * vmRightX + worldY * vmUpX + worldZ * vmAtX + vmPosX;
            const float viewY = worldX * vmRightY + worldY * vmUpY + worldZ * vmAtY + vmPosY;
            const float viewZ = worldX * vmRightZ + worldY * vmUpZ + worldZ * vmAtZ + vmPosZ;

            if (viewZ <= 1.0f)
            {
                corona.OffScreen = true;
                continue;
            }

            const float invViewZ = 1.0f / viewZ;
            RwV3d vecTransformedCoords{ viewX * screenWidth * invViewZ, viewY * screenHeight * invViewZ, viewZ };
            float fComputedHeight = screenHeight * invViewZ;

            corona.OffScreen = !(vecTransformedCoords.x >= 0.0f && vecTransformedCoords.x <= screenWidth &&
                vecTransformedCoords.y >= 0.0f && vecTransformedCoords.y <= screenHeight);

            if (vecTransformedCoords.z > corona.Range)
                continue;

            const float invFarClip = 1.0f / vecTransformedCoords.z;
            const float halfRange = corona.Range * 0.5f;
            // 0x7C00xxxx is reserved for lamps attached to distant vehicle meshes.
            // Their opacity already includes the model's distance/envelope/fog fade.
            float fadeFactor = !modelLamp && vecTransformedCoords.z > halfRange ? 1.0f - (vecTransformedCoords.z - halfRange) / halfRange : 1.0f;
            // The game's corona of this light fades over its range's second half of view depth; ours fills the rest
            if (corona.HandoverGame > 0.0f && corona.HandoverRange > 0.0f)
            {
                const float gameHalf = corona.HandoverRange * 0.5f;
                const float gameFade = vecTransformedCoords.z < gameHalf ? 1.0f : (std::max)(0.0f, 1.0f - (vecTransformedCoords.z - gameHalf) / gameHalf);
                fadeFactor *= 1.0f - corona.HandoverGame * gameFade;
            }
            const short fadeIntensity = static_cast<short>(corona.Intensity * fadeFactor);
            if (fadeIntensity <= 0)
                continue;

            RwTexture* pTex = corona.nTexType < 9 ? gpCoronaTexture[corona.nTexType] : nullptr;
            if (!pTex)
                continue;

            RwRaster* pRaster = RwTextureGetRaster(pTex);
            if (pLastRaster != pRaster)
            {
                pLastRaster = pRaster;
                pCurRenderBatch = SelectBatch(pRaster);
            }

            const float fColourFogMult = std::min(40.0f, vecTransformedCoords.z) * fogyness * 0.025f + 1.0f;
            if (corona.Identifier == 1)
                vecTransformedCoords.z = farPlane * 0.95f;

            float renderHeight = corona.Size * fComputedHeight;

            if (renderHeight < 0.35f)
                continue;

            if (corona.PullTowardsCam != 0.0f)
            {
                const float dx = worldX - pCamPos->x;
                const float dy = worldY - pCamPos->y;
                const float dz = worldZ - pCamPos->z;
                const float lenSq = dx * dx + dy * dy + dz * dz;

                if (lenSq > 0.0f)
                {
                    const float invLen = 1.0f / std::sqrt(lenSq);
                    const float dirX = dx * invLen;
                    const float dirY = dy * invLen;
                    const float dirZ = dz * invLen;

                    const float pullViewX = dirX * vmRightX + dirY * vmUpX + dirZ * vmAtX;
                    const float pullViewY = dirX * vmRightY + dirY * vmUpY + dirZ * vmAtY;
                    const float pullViewZ = dirX * vmRightZ + dirY * vmUpZ + dirZ * vmAtZ;

                    const float pulledViewX = viewX - pullViewX * corona.PullTowardsCam;
                    const float pulledViewY = viewY - pullViewY * corona.PullTowardsCam;
                    const float pulledViewZ = viewZ - pullViewZ * corona.PullTowardsCam;

                    if (pulledViewZ <= 1.0f)
                        continue;

                    const float invPulledViewZ = 1.0f / pulledViewZ;
                    vecTransformedCoords.x = pulledViewX * screenWidth * invPulledViewZ;
                    vecTransformedCoords.y = pulledViewY * screenHeight * invPulledViewZ;
                    vecTransformedCoords.z = pulledViewZ;

                    fComputedHeight = screenHeight * invPulledViewZ;
                    renderHeight = corona.Size * fComputedHeight;
                    if (renderHeight < 0.35f)
                        continue;
                }
            }

            pCurRenderBatch->AddOneXLUSpriteToBuffer_Rotate_Aspect(
                vecTransformedCoords.x, vecTransformedCoords.y, vecTransformedCoords.z,
                renderHeight, renderHeight * fColourFogMult,
                static_cast<uint8_t>(static_cast<float>(corona.Red) / fColourFogMult),
                static_cast<uint8_t>(static_cast<float>(corona.Green) / fColourFogMult),
                static_cast<uint8_t>(static_cast<float>(corona.Blue) / fColourFogMult),
                fadeIntensity, invFarClip * 20.0f, 0.0, 0xFF);
        }

        for (auto& batch : m_RenderBatches)
        {
            batch.Render();
        }

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, oldTextureRaster);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, oldFogEnable);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, oldZTest);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, oldDstBlend);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, oldSrcBlend);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, oldVertexAlpha);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, oldZWrite);
        RwRenderStateSet(rwRENDERSTATECULLMODE, oldCullMode);
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, oldAlphaTestFunc);
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, oldAlphaTestRef);
    }


    struct LightPool
    {
        const CLamppostInfo* lamp;
        float distance, r, g, b;
        CVector axis;
    };
    static inline std::vector<LightPool> aPools;
    static inline std::map<std::string, RwTexture*> PoolCache;

    static inline std::vector<CVector> PoolAxes;
    static inline const CLamppostInfo* PoolAxisData{ nullptr };

    static void RefreshPoolAxes()
    {
        if (PoolAxisData == m_Lampposts.data() && PoolAxes.size() == m_Lampposts.size())
            return;
        PoolAxisData = m_Lampposts.data();
        PoolAxes.assign(m_Lampposts.size(), {});
        // Match nearby rows of the same lamp, keeping growth narrower across the street.
        std::map<std::pair<int, int>, std::vector<size_t>> cells;
        auto Eligible = [](const CLamppostInfo& lamp)
        {
            return lamp.nEntity >= 0 && !lamp.nCoronaShowMode && lamp.nTrafficLightType == 2 &&
                lamp.pool.size > 0.0f && lamp.pool.intensity > 0.0f;
        };
        for (size_t i = 0; i < m_Lampposts.size(); ++i)
        {
            const auto& lamp{ m_Lampposts[i] };
            if (Eligible(lamp))
                cells[{ static_cast<int>(std::floor(lamp.vecPos.x / 80.0f)),
                    static_cast<int>(std::floor(lamp.vecPos.y / 80.0f)) }].push_back(i);
        }
        for (size_t i = 0; i < m_Lampposts.size(); ++i)
        {
            const auto& lamp{ m_Lampposts[i] };
            if (!Eligible(lamp))
                continue;
            const int x{ static_cast<int>(std::floor(lamp.vecPos.x / 80.0f)) };
            const int y{ static_cast<int>(std::floor(lamp.vecPos.y / 80.0f)) };
            float nearest{ 80.0f * 80.0f };
            for (int cy = y - 1; cy <= y + 1; ++cy)
                for (int cx = x - 1; cx <= x + 1; ++cx)
                {
                    const auto cell{ cells.find({ cx, cy }) };
                    if (cell == cells.end())
                        continue;
                    for (const auto j : cell->second)
                    {
                        const auto& other{ m_Lampposts[j] };
                        if (other.nEntity == lamp.nEntity || other.nPoolModel != lamp.nPoolModel ||
                            std::abs(other.vecPos.z - lamp.vecPos.z) > 6.0f ||
                            std::abs(std::cos(other.fHeading - lamp.fHeading)) < 0.95f)
                            continue;
                        const float dx{ other.vecPos.x - lamp.vecPos.x };
                        const float dy{ other.vecPos.y - lamp.vecPos.y };
                        const float distance{ dx * dx + dy * dy };
                        if (distance < 20.0f * 20.0f || distance >= nearest)
                            continue;
                        nearest = distance;
                        const float inverse{ 1.0f / std::sqrt(distance) };
                        PoolAxes[i] = { dx * inverse, dy * inverse, 0.0f };
                    }
                }
        }
    }

    static void RefreshPools()
    {
        for (auto& lamp : m_Lampposts)
        {
            if (lamp.pool.texture || lamp.poolTexture.empty() || lamp.nEntity < 0 || lamp.nCoronaShowMode || lamp.nTrafficLightType != 2)
                continue;
            auto [it, inserted]{ PoolCache.try_emplace(lamp.poolTexture, nullptr) };
            if (inserted)
            {
                it->second = CModelInfo::FindPoolTexture(lamp.poolTexture.c_str());
                LampEntities::RetainPoolTexture(it->second);
            }
            lamp.pool.texture = it->second;
        }
    }

public:
    static void ConfigureLightVolumes(uintptr_t deviceSlot)
    {
        LightVolumes::DeviceSlot = reinterpret_cast<void**>(deviceSlot);
    }

    static void RenderPools()
    {
        if (!bRenderStaticShadowsForLODs || aPools.empty() || !Scene->m_pRwCamera)
            return;
        LightVolumes::LastLightCount = 0;
        LightVolumes::RenderDepth(aPools, Scene->m_pRwCamera);
    }

    static void RegisterLODLights()
    {
        ++LampEntities::Frame;
        aPools.clear();

        const bool night = CClock::GetIsTimeInRange(20, 7);
        if (CGame::currArea != 0)
        {
            Update();
            return;
        }

        ++CurrentFrameStamp;
        if (!CurrentFrameStamp)
            ++CurrentFrameStamp;

        static auto SolveEqSys = [](float a, float b, float c, float d, float value) -> float
        {
            float determinant = a - c;
            float x = (b - d) / determinant;
            float y = (a * d - b * c) / determinant;
            return std::min(x * value + y, d);
        };

        static auto FastRadiusFalloff = [](float radius) -> float
        {
            return std::clamp(1.0f / (0.75f * radius + 0.25f), 0.3f, 1.0f);
        };

        unsigned char bAlpha = 0;
        unsigned int nTime = CClock::ms_nGameClockHours * 60 + CClock::ms_nGameClockMinutes;
        const uint32_t timeMs = CTimer::GetEffectsTimeInMilliseconds();
        const uint8_t trafficPhases[] = { CTrafficLights::Phase(1), CTrafficLights::Phase(2) };

        fCoronaFarClip = autoFarClip ? CTimeCycle::m_fCurrentFarClip : fCoronaFarClip;

        // Use fixed reference for size calculations to prevent size changes with far clip
        const float REFERENCE_FAR_CLIP = 1000.0f;

        // Time-based alpha
        if (!night)
            bAlpha = 0;
        else if (nTime >= 20 * 60)
            bAlpha = static_cast<unsigned char>((15.0f / 16.0f) * nTime - 1095.0f);
        else if (nTime < 3 * 60)
            bAlpha = 255;
        else
            bAlpha = static_cast<unsigned char>((-15.0f / 16.0f) * nTime + 424.0f);

        // The game's own lamps: on/off (or the SA day/night balance), sprite brightness and size, the corona's ease
        const float lampFade = GameLights::LampFade();
        const bool lampsOn = night || lampFade > 0.0f;
        const float fov = GameLights::FOV;
        const float fovScale = fov > 1.0f ? 70.0f / fov : 1.0f;
        const float spriteBrightness = (std::max)(0.0f, CTimeCycle::m_fCurrentSpriteBrightness * GameLights::fSpriteBrightnessScale);
        const float spriteSize = CTimeCycle::m_fCurrentSpriteSize * GameLights::fSpriteSizeScale;
        const float gameColourScale = GameLights::bDayNightBalance ? spriteBrightness : 1.0f;
        const float gameStep = std::clamp(CTimer::ms_fTimeStep * 15.0f / 255.0f, 0.0f, 1.0f);
        const float poolRange = GameLights::fPoolRange;
        const float poolScale = spriteBrightness * GameLights::fPoolColour * lampFade;
        const bool pools = bRenderStaticShadowsForLODs && lampFade > 0.0f && poolScale > 0.0f;

        const CVector* pCamPos = &TheCamera->GetCoords();
        const float fCoronaFarClipSq = fCoronaFarClip * fCoronaFarClip;
        const float fVeryFarDistSq = 260.0f * 260.0f;
        const uint8_t frameSlice4 = CurrentFrameStamp & 3;

        if (pools)
        {
            RefreshPools();
            RefreshPoolAxes();
        }
        LightVolumes::CompactPools = GameLights::bDayNightBalance;
        LightVolumes::Prepare();

        if (TrafficLampData != m_Lampposts.data() || TrafficLampCount != m_Lampposts.size())
        {
            TrafficLampIndices.clear();
            for (size_t i = 0; i < m_Lampposts.size(); ++i)
                if (m_Lampposts[i].fCustomSizeMult == .45f) TrafficLampIndices.push_back(i);
            TrafficLampData = m_Lampposts.data();
            TrafficLampCount = m_Lampposts.size();
        }
        const size_t lampCount = lampsOn ? m_Lampposts.size() : TrafficLampIndices.size();
        for (size_t lampIndex = 0; lampIndex < lampCount; ++lampIndex)
        {
            auto& lamp = m_Lampposts[lampsOn ? lampIndex : TrafficLampIndices[lampIndex]];
            const bool trafficLight = lamp.fCustomSizeMult == .45f;
            if (lamp.vecPos.z < -15.0f || lamp.vecPos.z > 1030.0f)
                continue;

            // The game draws this light itself while it processes the entity; its corona eases in and out
            const bool hasGame = lamp.nEntity >= 0 && !lamp.nNoDistance;
            const bool gameActive = hasGame && LampEntities::Processed(lamp.nEntity);
            if (hasGame)
                lamp.fGameFade = std::clamp(lamp.fGameFade + (gameActive ? gameStep : -gameStep), 0.0f, 1.0f);

            const unsigned int coronaId = reinterpret_cast<unsigned int>(&lamp);
            const float dx = pCamPos->x - lamp.vecPos.x;
            const float dy = pCamPos->y - lamp.vecPos.y;
            const float dz = pCamPos->z - lamp.vecPos.z;
            const float fDist2DSqr = dx * dx + dy * dy;
            const float fDistSqr = fDist2DSqr + dz * dz;

            if (!lamp.nNoDistance && fDistSqr >= fCoronaFarClipSq)
                continue;

            const float distance = std::sqrt(fDistSqr);

            // Light pool on the ground. The game's pool is full to 3/4 of its range and fades out at the range (2D);
            // ours fades in over the same band and out over the last quarter of the draw distance.
            if (pools && !trafficLight && !lamp.nCoronaShowMode && lamp.nEntity >= 0 && lamp.pool.texture && lamp.pool.size > 0.0f)
            {
                float weight = std::clamp((fCoronaFarClip - distance) / (0.25f * fCoronaFarClip), 0.0f, 1.0f);
                if (gameActive)
                {
                    const float dist2D = std::sqrt(fDist2DSqr);
                    if (dist2D < poolRange)
                        weight *= std::clamp((dist2D - 0.75f * poolRange) * 4.0f / poolRange, 0.0f, 1.0f);
                }
                if (weight > 0.004f && (!lamp.pPredicate || lamp.pPredicate()))
                {
                    const float c = 0.8f * poolScale * lamp.pool.intensity * weight;
                    const float peak{ static_cast<float>((std::max)({ lamp.pool.colour.r, lamp.pool.colour.g, lamp.pool.colour.b })) };
                    const float scale{ peak > 0.0f ? LightVolumes::Colour(peak * c, distance) / peak : 0.0f };
                    aPools.push_back({ &lamp, distance,
                        lamp.pool.colour.r * scale,
                        lamp.pool.colour.g * scale,
                        lamp.pool.colour.b * scale,
                        LightVolumes::FarWeight(distance) > 0.0f ? PoolAxes[lampIndex] : CVector{} });
                }
            }

            // Fully covered by the game's own corona (view depth is at most the distance)
            const float range = trafficLight ? GameLights::fTrafficRange : lamp.fGameCoronaRange;
            if (hasGame && lamp.fGameFade >= 1.0f && range > 0.0f && distance < range * 0.5f)
                continue;

            if (fDistSqr > fVeryFarDistSq && !trafficLight && !lamp.nCoronaShowMode && lamp.fGameFade == 0.0f)
            {
                if (((coronaId >> 5) & 3u) != frameSlice4)
                {
                    TouchCorona(coronaId);
                    TouchCorona(coronaId + 1);
                    continue;
                }
            }

            // Fade out over the last 100 m of the far clip
            float farFade = 1.0f;
            if (!lamp.nNoDistance && distance > fCoronaFarClip - 100.0f)
                farFade = std::clamp((fCoronaFarClip - distance) / 100.0f, 0.0f, 1.0f);

            const float handoverGame = hasGame ? lamp.fGameFade : 0.0f;
            auto Register = [&](unsigned int id, float r, float g, float b, float alpha, float size, int type)
            {
                const auto a = static_cast<unsigned char>(std::clamp(alpha * 255.0f, 0.0f, 255.0f));
                if (!a)
                    return;
                RegisterCorona(id, nullptr,
                    static_cast<unsigned char>(std::clamp(r, 0.0f, 255.0f)),
                    static_cast<unsigned char>(std::clamp(g, 0.0f, 255.0f)),
                    static_cast<unsigned char>(std::clamp(b, 0.0f, 255.0f)), a,
                    lamp.vecPos, size, fCoronaFarClip, type, 0, false, false, 0, 0.0f, false,
                    trafficLight ? 1.5f : GameLights::fLampPull, 0, 255.0f, false, false,
                    lamp.pPredicate, range, handoverGame);
            };

            if (trafficLight)
            {
                // All bulb sets on one entity follow its native traffic group. Like the game's: the signal's colour,
                // sprite size and brightness, no growth or boost with distance.
                const uint8_t phase = trafficPhases[lamp.nTrafficLightType == 1 ? 0 : 1];
                if (phase >= 3 || phase != lamp.nTrafficLightState)
                    continue;
                static constexpr float colours[3][3] = { { 0.0f, 255.0f, 0.0f }, { 255.0f, 128.0f, 0.0f }, { 255.0f, 0.0f, 0.0f } };
                const float brightness = 0.7f * spriteBrightness;
                Register(coronaId, colours[phase][0] * brightness, colours[phase][1] * brightness, colours[phase][2] * brightness,
                    farFade, 1.75f * spriteSize * fovScale, 0);
                continue;
            }

            float blinking = 1.0f;
            if (lamp.nCoronaShowMode == BlinkTypes::RANDOM_FLASHING)
                blinking = DistantLightLogic::Pulse(timeMs, 500, 500);
            else if (lamp.nCoronaShowMode >= BlinkTypes::T_1S_ON_1S_OFF && lamp.nCoronaShowMode <= BlinkTypes::T_6S_ON_4S_OFF)
            {
                uint32_t on = (lamp.nCoronaShowMode - BlinkTypes::T_1S_ON_1S_OFF + 1) * 1000;
                uint32_t off = lamp.nCoronaShowMode == BlinkTypes::T_6S_ON_4S_OFF ? 4000 : on;
                blinking = DistantLightLogic::Pulse(timeMs, on, off);
            }

            // From the game's look at its corona range to the distant look at twice that
            const float t = hasGame && range > 0.0f ? std::clamp((distance - range) / range, 0.0f, 1.0f) : 1.0f;
            const float from = hasGame ? range : 0.0f;

            float fRadius = 1.75f;
            if (bSlightlyIncreaseRadiusWithDistance)
                fRadius *= std::clamp(SolveEqSys(from, 1.0f, REFERENCE_FAR_CLIP, 4.0f, distance), 1.0f, 4.0f);
            const float distantSize = fRadius * lamp.fCustomSizeMult * fCoronaRadiusMultiplier;
            const float gameSize = lamp.fCustomSizeMult * fovScale;
            const float size = gameSize + (distantSize - gameSize) * t;

            // Distant look: night ramp, dimmer near and brighter far, larger coronas proportionally dimmer
            const float d = distance - from;
            float alphaDistMult = fCoronaAlphaNearMinMult + std::clamp(d / fCoronaAlphaReachOneAt, 0.0f, 1.0f) * (1.0f - fCoronaAlphaNearMinMult);
            if (d > fCoronaAlphaBoostStartAt)
            {
                const float boost = std::clamp((d - fCoronaAlphaBoostStartAt) / 900.0f, 0.0f, 1.0f);
                alphaDistMult = 1.0f + boost * (fCoronaAlphaFarBoostMax - 1.0f);
            }
            const float radiusAlphaScale = distantSize > 1.0f ? FastRadiusFalloff(distantSize) : 1.0f;
            const float distantAlpha = (bAlpha / 255.0f) * (lamp.colour.a / 255.0f) * alphaDistMult * radiusAlphaScale;

            // The game's star corona near its range, the distant one beyond
            if (t < 1.0f)
                Register(coronaId + 1, lamp.colour.r * gameColourScale, lamp.colour.g * gameColourScale, lamp.colour.b * gameColourScale,
                    lampFade * farFade * blinking * (1.0f - t), size, 0);
            if (t > 0.0f)
                Register(coronaId, lamp.colour.r, lamp.colour.g, lamp.colour.b, distantAlpha * farFade * blinking * t, size, 1);

            if (lamp.nNoDistance > 1 && (!lamp.pPredicate || lamp.pPredicate()))
            {
                constexpr float MAX_POINTLIGHT_DIST = 22.0f;

                if (distance <= MAX_POINTLIGHT_DIST)
                {
                    // Calculate light radius based on corona size
                    float lightRadius = lamp.fCustomSizeMult * 10.0f;
                    if (lamp.fObjectDrawDistance < 20.0f)
                        lightRadius = lamp.fObjectDrawDistance;

                    // Normalize color intensity (0.0 to 1.0 range)
                    float intensity = std::clamp(distantAlpha * blinking, 0.0f, 1.0f);
                    float red = (lamp.colour.r / 255.0f) * intensity;
                    float green = (lamp.colour.g / 255.0f) * intensity;
                    float blue = (lamp.colour.b / 255.0f) * intensity;

                    // 0 - lod light, 1 - no distance, 2 and above - point light type
                    auto type = PointLightType(lamp.nNoDistance - 2);

                    // Direction pointing downward for lamppost effect
                    CPointLights::AddLight(
                        type,
                        lamp.vecPos.x, lamp.vecPos.y, lamp.vecPos.z,
                        0.0f, 0.0f, -1.0f,  // Direction (down)
                        lightRadius,
                        red, green, blue,
                        0,  // Fog type
                        0,  // Generate extra shadows
                        nullptr  // Entity affected
                    );
                }
            }
        }

        Update();
    }
};
