module;

#define NOMINMAX
#include <stdafx.h>
#include <unordered_map>

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

export class CRegisteredCorona
{
public:
    CVector        Coordinates;            // Where is it exactly.
    uint32_t       Identifier;             // Should be unique for each corona. Address or something (0 = empty)
    RwTexture* pTex;                   // Pointer to the actual texture to be rendered
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

public:
    CRegisteredCorona()
        : Identifier(0), pEntityAttachedTo(nullptr)
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
    static inline std::unordered_map<unsigned int, CLODLightsLinkedListNode*> UsedMap;
    static inline CLODLightsLinkedListNode FreeList, UsedList;
    static inline std::vector<CLODLightsLinkedListNode> aLinkedList;
    static inline std::vector<CRegisteredCorona> aCoronas;
    static inline uint8_t CurrentFrameStamp = 1;
    static inline CLODLightsLinkedListNode* pFarthestNode = nullptr;
    static inline float fFarthestDistSq = 0.0f;
public:
    static int& bChangeBrightnessImmediately;
    static float& ScreenMult;

public:
    static void RegisterCorona(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, unsigned char flareType, unsigned char reflectionType, unsigned char LOSCheck, unsigned char unused, float normalAngle, bool bNeonFade, float PullTowardsCam, bool bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool bWhiteCore)
    {
        UNREFERENCED_PARAMETER(unused);
        UNREFERENCED_PARAMETER(bFadeIntensity);
        UNREFERENCED_PARAMETER(pAttachTo);

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
                UsedMap.erase(it);
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
                    UsedMap.erase(evictId);
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
            UsedMap.emplace(nID, pNewEntry);

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
        pSuitableSlot->pTex = pTex;
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
    }

    static void TouchCorona(unsigned int nID)
    {
        auto it = UsedMap.find(nID);
        if (it != UsedMap.end())
            it->second->GetFrom()->RegisteredThisFrame = CurrentFrameStamp;
    }

    static void RegisterCorona(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, int coronaType, unsigned char flareType, bool enableReflection, bool checkObstacles, int unused, float normalAngle, bool longDistance, float nearClip, unsigned char bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool reflectionDelay)
    {
        RegisterCorona(nID, pAttachTo, R, G, B, A, Position, Size, Range, gpCoronaTexture[coronaType], flareType, enableReflection, checkObstacles, unused, normalAngle, longDistance, nearClip, bFadeIntensity, FadeSpeed, bOnlyFromBelow, reflectionDelay);
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
                    UsedMap.erase(nIndex);
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
            UsedMap.clear();
            UsedMap.reserve(numCoronas);

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

    static void RenderBuffered()
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
        bool bLastZTestEnable = true;

        void* oldZWrite = nullptr;
        void* oldVertexAlpha = nullptr;
        void* oldSrcBlend = nullptr;
        void* oldDstBlend = nullptr;
        void* oldZTest = nullptr;
        void* oldRaster = nullptr;

        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &oldZWrite);
        RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &oldVertexAlpha);
        RwRenderStateGet(rwRENDERSTATESRCBLEND, &oldSrcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, &oldDstBlend);
        RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &oldZTest);
        RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &oldRaster);

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);

        for (auto pNode = UsedList.First(); pNode && pNode != &UsedList; pNode = pNode->GetNextNode())
        {
            auto& corona = *pNode->GetFrom();
            if (!corona.Identifier || corona.Intensity == 0)
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
            const float fadeFactor = vecTransformedCoords.z > halfRange ? 1.0f - (vecTransformedCoords.z - halfRange) / halfRange : 1.0f;
            const short fadeIntensity = static_cast<short>(corona.Intensity * fadeFactor);

            if (!corona.pTex)
                continue;

            const bool zTestEnable = !corona.LOSCheck;
            if (bLastZTestEnable != zTestEnable)
            {
                bLastZTestEnable = zTestEnable;
                CSprite::FlushSpriteBuffer();
                RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)bLastZTestEnable);
            }

            const RwRaster* pRaster = RwTextureGetRaster(corona.pTex);
            if (pLastRaster != pRaster)
            {
                pLastRaster = const_cast<RwRaster*>(pRaster);
                CSprite::FlushSpriteBuffer();
                RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pLastRaster);
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

            CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect(
                vecTransformedCoords.x, vecTransformedCoords.y, vecTransformedCoords.z,
                renderHeight, renderHeight * fColourFogMult,
                static_cast<uint8_t>(static_cast<float>(corona.Red) / fColourFogMult),
                static_cast<uint8_t>(static_cast<float>(corona.Green) / fColourFogMult),
                static_cast<uint8_t>(static_cast<float>(corona.Blue) / fColourFogMult),
                fadeIntensity, invFarClip * 20.0f, 0.0, 0xFF);
        }

        CSprite::FlushSpriteBuffer();

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, oldRaster);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, oldZTest);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, oldDstBlend);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, oldSrcBlend);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, oldVertexAlpha);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, oldZWrite);
    }

    static void RegisterLODLights()
    {
        if (!(CClock::GetIsTimeInRange(20, 7) && CGame::currArea == 0))
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
        unsigned int curMin = CClock::ms_nGameClockMinutes;

        fCoronaFarClip = autoFarClip ? CTimeCycle::m_fCurrentFarClip : fCoronaFarClip;

        // Use fixed reference for size calculations to prevent size changes with far clip
        const float REFERENCE_FAR_CLIP = 1000.0f;

        // Time-based alpha
        if (nTime >= 20 * 60)
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

        for (auto it = m_Lampposts.cbegin(); it != m_Lampposts.cend(); ++it)
        {
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

            if (fDistSqr > fVeryFarDistSq)
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
            float fNormalizedAlpha = (bAlpha / 255.0f) * (it->colour.a / 255.0f) * fAlphaMultiplier;

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
                    fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false
                );

                if (it->nNoDistance > 1)
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

                    if (bRenderStaticShadowsForLODs)
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
                    // Per-light blinking using unique seed
                    uint32_t seed = reinterpret_cast<uint32_t>(&*it);
                    float timeMs = static_cast<float>(CTimer::m_snTimeInMillisecondsPauseMode);
                    float blinking = 1.0f;

                    switch (it->nCoronaShowMode)
                    {
                        case BlinkTypes::DEFAULT:
                            blinking = 1.0f;  // Always on
                            break;
                        case BlinkTypes::RANDOM_FLASHING:
                        {
                            // All RANDOM_FLASHING lights blink together, randomly
                            static float nextToggleTime = 0.0f;
                            static bool isOn = true;

                            if (timeMs >= nextToggleTime)
                            {
                                isOn = !isOn;
                                // Random interval between 500ms and 1500ms
                                float randomInterval = 500.0f + (rand() % 1000);
                                nextToggleTime = timeMs + randomInterval;
                            }

                            blinking = isOn ? 1.0f : 0.0f;
                            break;
                        }
                        case BlinkTypes::T_1S_ON_1S_OFF:
                        {
                            float period = 2000.0f;  // 1s on + 1s off = 2s total
                            float phase = (seed % 1000) / 1000.0f * period;
                            float t = fmodf(timeMs + phase, period);
                            blinking = (t < 1000.0f) ? 1.0f : 0.0f;
                            break;
                        }
                        case BlinkTypes::T_2S_ON_2S_OFF:
                        {
                            float period = 4000.0f;  // 2s on + 2s off = 4s total
                            float phase = (seed % 1000) / 1000.0f * period;
                            float t = fmodf(timeMs + phase, period);
                            blinking = (t < 2000.0f) ? 1.0f : 0.0f;
                            break;
                        }
                        case BlinkTypes::T_3S_ON_3S_OFF:
                        {
                            float period = 6000.0f;  // 3s on + 3s off = 6s total
                            float phase = (seed % 1000) / 1000.0f * period;
                            float t = fmodf(timeMs + phase, period);
                            blinking = (t < 3000.0f) ? 1.0f : 0.0f;
                            break;
                        }
                        case BlinkTypes::T_4S_ON_4S_OFF:
                        {
                            float period = 8000.0f;  // 4s on + 4s off = 8s total
                            float phase = (seed % 1000) / 1000.0f * period;
                            float t = fmodf(timeMs + phase, period);
                            blinking = (t < 4000.0f) ? 1.0f : 0.0f;
                            break;
                        }
                        case BlinkTypes::T_5S_ON_5S_OFF:
                        {
                            float period = 10000.0f;  // 5s on + 5s off = 10s total
                            float phase = (seed % 1000) / 1000.0f * period;
                            float t = fmodf(timeMs + phase, period);
                            blinking = (t < 5000.0f) ? 1.0f : 0.0f;
                            break;
                        }
                        case BlinkTypes::T_6S_ON_4S_OFF:
                        {
                            float period = 10000.0f;  // 6s on + 4s off = 10s total
                            float phase = (seed % 1000) / 1000.0f * period;
                            float t = fmodf(timeMs + phase, period);
                            blinking = (t < 6000.0f) ? 1.0f : 0.0f;
                            break;
                        }
                        default:
                            blinking = 1.0f;
                            break;
                    }

                    RegisterLampCorona(blinking * fNormalizedAlpha);
                }
            }
            else  // Traffic lights
            {
                // Rotate local position by object heading to get world-facing direction
                float cosHeading = cos(it->fHeading);
                float sinHeading = sin(it->fHeading);

                float worldX = it->vecLocalPos.x * cosHeading - it->vecLocalPos.y * sinHeading;
                float worldY = it->vecLocalPos.x * sinHeading + it->vecLocalPos.y * cosHeading;

                // Determine if this bulb set faces N/S or E/W
                bool isFacingEastWest = fabs(worldX) > fabs(worldY);

                // Color detection
                bool isYellow = (it->colour.r >= 250 && it->colour.g >= 100 && it->colour.b <= 150);
                bool isRed = (it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0);
                bool isGreen = (it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0);

                // Timing (simplified - can hook CTrafficLights for real timing)
                uint8_t curMin = CClock::ms_nGameClockMinutes;
                bool isYellowTime = (curMin % 10 == 9);
                bool isRedTime = (curMin % 20 < 9);
                bool isGreenTime = !isYellowTime && !isRedTime;

                // E/W gets OPPOSITE phase from N/S
                if (isFacingEastWest)
                {
                    bool temp = isRedTime;
                    isRedTime = isGreenTime;
                    isGreenTime = temp;
                }

                bool shouldDraw = (isYellow && isYellowTime) ||
                    (isRed && isRedTime) ||
                    (isGreen && isGreenTime);

                if (shouldDraw)
                    RegisterLampCorona(fNormalizedAlpha);
            }
        }

        Update();
    }
};