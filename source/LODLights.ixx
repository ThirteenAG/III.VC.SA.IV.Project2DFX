module;

#define NOMINMAX
#include <stdafx.h>
#include <unordered_map>
#include "DistantLightLogic.hpp"

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

// Native phase readers include cheats and VC storm/SA riot flashing.
// Only two calls per frame; object classification is cached at map registration.
export namespace CTrafficLights
{
    uint8_t (__cdecl* LightForCars1)() = nullptr;
    uint8_t (__cdecl* LightForCars2)() = nullptr;
    uint8_t (__cdecl* LightForCars1_Visual)() = nullptr;
    uint8_t (__cdecl* LightForCars2_Visual)() = nullptr;
    int (__cdecl* FindTrafficLightType)(CEntity*) = nullptr;
    uint32_t* GameTime = nullptr;
    unsigned TimeDivisor = 1;

    uint8_t Type(CEntity* entity, const CVector& forward)
    {
        return FindTrafficLightType ? static_cast<uint8_t>(FindTrafficLightType(entity)) : DistantLightLogic::TrafficGroup(forward.x, forward.y);
    }
    uint8_t Phase(unsigned group, uint32_t fallbackTime)
    {
        auto visual = group == 1 ? LightForCars1_Visual : LightForCars2_Visual;
        auto logic = group == 1 ? LightForCars1 : LightForCars2;
        if (visual) return visual();
        if (logic) return logic();
        return DistantLightLogic::TrafficPhase(GameTime ? *GameTime : fallbackTime, group, TimeDivisor);
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
    static void RegisterCoronaInternal(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, int coronaType, unsigned char flareType, unsigned char reflectionType, unsigned char LOSCheck, unsigned char unused, float normalAngle, bool bNeonFade, float PullTowardsCam, bool bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool bWhiteCore, CoronaPredicate pPredicate = nullptr)
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

    static void RegisterCorona(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, int coronaType, unsigned char flareType, bool enableReflection, bool checkObstacles, int unused, float normalAngle, bool longDistance, float nearClip, unsigned char bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool reflectionDelay, CoronaPredicate pPredicate = nullptr)
    {
        RegisterCoronaInternal(nID, pAttachTo, R, G, B, A, Position, Size, Range, coronaType, flareType, enableReflection, checkObstacles, unused, normalAngle, longDistance, nearClip, bFadeIntensity, FadeSpeed, bOnlyFromBelow, reflectionDelay, pPredicate);
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
            const float fadeFactor = !modelLamp && vecTransformedCoords.z > halfRange ? 1.0f - (vecTransformedCoords.z - halfRange) / halfRange : 1.0f;
            const short fadeIntensity = static_cast<short>(corona.Intensity * fadeFactor);

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


    static void RegisterLODLights()
    {
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
        const uint8_t trafficPhases[] = { CTrafficLights::Phase(1, timeMs), CTrafficLights::Phase(2, timeMs) };

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

        const CVector* pCamPos = &TheCamera->GetCoords();
        const float fScale = 1.0f;
        const float fCoronaFarClipSq = fCoronaFarClip * fCoronaFarClip;
        const float fVeryFarDistSq = 260.0f * 260.0f;
        const uint8_t frameSlice4 = CurrentFrameStamp & 3;

        if (TrafficLampData != m_Lampposts.data() || TrafficLampCount != m_Lampposts.size())
        {
            TrafficLampIndices.clear();
            for (size_t i = 0; i < m_Lampposts.size(); ++i)
                if (m_Lampposts[i].fCustomSizeMult == .45f) TrafficLampIndices.push_back(i);
            TrafficLampData = m_Lampposts.data();
            TrafficLampCount = m_Lampposts.size();
        }
        const size_t lampCount = night ? m_Lampposts.size() : TrafficLampIndices.size();
        for (size_t lampIndex = 0; lampIndex < lampCount; ++lampIndex)
        {
            auto it = m_Lampposts.cbegin() + (night ? lampIndex : TrafficLampIndices[lampIndex]);
            const bool trafficLight = it->fCustomSizeMult == .45f;
            if (it->vecPos.z < -15.0f || it->vecPos.z > 1030.0f)
                continue;

            unsigned int coronaId = reinterpret_cast<unsigned int>(&*it);
            float dx = pCamPos->x - it->vecPos.x;
            float dy = pCamPos->y - it->vecPos.y;
            float dz = pCamPos->z - it->vecPos.z;
            float fDistSqr = dx * dx + dy * dy + dz * dz;

            float fEffectiveDrawDistance = it->fObjectDrawDistance * fScale;
            float fEffectiveCoronaDist = fEffectiveDrawDistance - 30.0f;
            float fEffectiveCoronaDistSq = fEffectiveCoronaDist * fEffectiveCoronaDist;

            // Early exit with squared distance
            if (!it->nNoDistance &&
                (fDistSqr <= fEffectiveCoronaDistSq || fDistSqr >= fCoronaFarClipSq))
                continue;

            if (fDistSqr > fVeryFarDistSq && !trafficLight && !it->nCoronaShowMode)
            {
                if (((coronaId >> 5) & 3u) != frameSlice4)
                {
                    TouchCorona(coronaId);
                    continue;
                }
            }

            float distance = std::sqrt(fDistSqr);
            float fRadius;

            if (it->nNoDistance)
                fRadius = 1.75f;
            else
                fRadius = SolveEqSys(fEffectiveCoronaDist, 0.0f, fEffectiveDrawDistance, 1.75f, distance);

            if (bSlightlyIncreaseRadiusWithDistance)
                fRadius *= std::min(SolveEqSys(fEffectiveCoronaDist, 1.0f, REFERENCE_FAR_CLIP, 4.0f, distance), 4.0f);

            // Alpha transition with proper fade-in and fade-out
            float fAlphaMultiplier = 1.0f;

            if (!it->nNoDistance)
            {
                if (distance < fEffectiveDrawDistance)
                {
                    // Fade IN: 0.0 at fEffectiveCoronaDist → 1.0 at fEffectiveDrawDistance
                    fAlphaMultiplier = std::clamp((distance - fEffectiveCoronaDist) / 30.0f, 0.0f, 1.0f);
                }
                else if (distance > fCoronaFarClip - 100.0f)
                {
                    // Fade OUT: 1.0 at (farClip - 100) → 0.0 at farClip
                    fAlphaMultiplier = std::clamp((fCoronaFarClip - distance) / 100.0f, 0.0f, 1.0f);
                }
            }

            {
                // Distance from corona fade-in start zone
                const float d = distance - fEffectiveCoronaDist;

                float alphaDistMult = fCoronaAlphaNearMinMult + std::clamp(d / fCoronaAlphaReachOneAt, 0.0f, 1.0f) * (1.0f - fCoronaAlphaNearMinMult);

                if (d > fCoronaAlphaBoostStartAt)
                {
                    const float t = std::clamp((d - fCoronaAlphaBoostStartAt) / 900.0f, 0.0f, 1.0f);
                    alphaDistMult = 1.0f + t * (fCoronaAlphaFarBoostMax - 1.0f);
                }

                fAlphaMultiplier *= alphaDistMult;
            }

            // Calculate normalized alpha
            float fNormalizedAlpha = ((trafficLight ? 255 : bAlpha) / 255.0f) * (it->colour.a / 255.0f) * fAlphaMultiplier;

            // Helper for registration
            auto RegisterLampCorona = [&](float normalizedAlpha)  // 0.0 to 1.0
            {
                // Scale alpha inversely with radius size
                // Larger coronas get proportionally dimmer to prevent overwhelming brightness
                float radiusAlphaScale = 1.0f;

                // Calculate the final radius that will be used
                float finalRadius = fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier;

                // Apply inverse scaling: larger radius = lower alpha
                // Example: radius 1.0 = 100% alpha, radius 2.0 = 70% alpha, radius 3.0 = 50% alpha
                if (finalRadius > 1.0f)
                {
                    radiusAlphaScale = FastRadiusFalloff(finalRadius);
                }

                unsigned char alpha = static_cast<unsigned char>(std::clamp(normalizedAlpha * radiusAlphaScale * 255.0f, 0.0f, 255.0f));
                RegisterCorona(
                    reinterpret_cast<unsigned int>(&*it), nullptr,
                    it->colour.r, it->colour.g, it->colour.b, alpha,
                    it->vecPos,
                    finalRadius,
                    fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false,
                    it->pPredicate
                );

                if (it->nNoDistance > 1 && (!it->pPredicate || it->pPredicate()))
                {
                    constexpr float MAX_POINTLIGHT_DIST = 22.0f;

                    if (distance <= MAX_POINTLIGHT_DIST)
                    {
                        // Calculate light radius based on corona size
                        float lightRadius = it->fCustomSizeMult * 10.0f;
                        if (it->fObjectDrawDistance < 20.0f)
                            lightRadius = it->fObjectDrawDistance;

                        // Normalize color intensity (0.0 to 1.0 range)
                        float intensity = normalizedAlpha * radiusAlphaScale;
                        float red = (it->colour.r / 255.0f) * intensity;
                        float green = (it->colour.g / 255.0f) * intensity;
                        float blue = (it->colour.b / 255.0f) * intensity;

                        // 0 - lod light, 1 - no distance, 2 and above - point light type
                        auto type = PointLightType(it->nNoDistance - 2);

                        // Direction pointing downward for lamppost effect
                        CPointLights::AddLight(
                            type,
                            it->vecPos.x, it->vecPos.y, it->vecPos.z,
                            0.0f, 0.0f, -1.0f,  // Direction (down)
                            lightRadius,
                            red, green, blue,
                            0,  // Fog type
                            0,  // Generate extra shadows
                            nullptr  // Entity affected
                        );
                    }
                }
            };

            if (it->fCustomSizeMult != 0.45f)
            {
                if (!it->nCoronaShowMode)
                {
                    RegisterLampCorona(fNormalizedAlpha);

                    if (bRenderStaticShadowsForLODs && (!it->pPredicate || it->pPredicate()))
                        CShadows::StoreStaticShadow(
                            reinterpret_cast<unsigned int>(&*it), SSHADT_INTENSIVE,
                            *CShadows::gpShadowExplosionTex, (CVector*)&it->vecPos,
                            8.0f, 0.0f, 0.0f, -8.0f, bAlpha,
                            it->colour.r / 3, it->colour.g / 3, it->colour.b / 3,
                            15.0f, 1.0f, fCoronaFarClip, false, 0.0f
                        );
                }
                else
                {
                    float blinking = 1.0f;
                    if (it->nCoronaShowMode == BlinkTypes::RANDOM_FLASHING)
                        blinking = DistantLightLogic::Pulse(timeMs, 500, 500);
                    else if (it->nCoronaShowMode >= BlinkTypes::T_1S_ON_1S_OFF && it->nCoronaShowMode <= BlinkTypes::T_6S_ON_4S_OFF)
                    {
                        uint32_t on = (it->nCoronaShowMode - BlinkTypes::T_1S_ON_1S_OFF + 1) * 1000;
                        uint32_t off = it->nCoronaShowMode == BlinkTypes::T_6S_ON_4S_OFF ? 4000 : on;
                        blinking = DistantLightLogic::Pulse(timeMs, on, off);
                    }

                    RegisterLampCorona(blinking * fNormalizedAlpha);
                }
            }
            else  // Traffic lights
            {
                // All bulb sets on one entity follow its native traffic group.
                // Bulb offsets describe geometry, not a different signal phase.
                const uint8_t phase = trafficPhases[it->nTrafficLightType == 1 ? 0 : 1];
                const bool shouldDraw = phase < 3 && phase == it->nTrafficLightState;
                if (shouldDraw)
                    RegisterLampCorona(fNormalizedAlpha);
            }
        }

        Update();
    }
};
