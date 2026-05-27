module;

#define NOMINMAX
#include <stdafx.h>

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
    static inline std::map<unsigned int, CLODLightsLinkedListNode*> UsedMap;
    static inline CLODLightsLinkedListNode FreeList, UsedList;
    static inline std::vector<CLODLightsLinkedListNode> aLinkedList;
    static inline std::vector<CRegisteredCorona> aCoronas;
public:
    static int& bChangeBrightnessImmediately;
    static float& ScreenMult;

public:
    static void RegisterCorona(unsigned int nID, CEntity* pAttachTo, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, unsigned char flareType, unsigned char reflectionType, unsigned char LOSCheck, unsigned char unused, float normalAngle, bool bNeonFade, float PullTowardsCam, bool bFadeIntensity, float FadeSpeed, bool bOnlyFromBelow, bool bWhiteCore)
    {
        UNREFERENCED_PARAMETER(unused);

        CVector		vecPosToCheck = Position;
        CVector* pCamPos = GetCamPos();
        if (Range * Range >= (pCamPos->x - vecPosToCheck.x) * (pCamPos->x - vecPosToCheck.x) + (pCamPos->y - vecPosToCheck.y) * (pCamPos->y - vecPosToCheck.y))
        {
            if (bNeonFade)
            {
                float		fDistFromCam = CVector(*pCamPos - vecPosToCheck).Magnitude();

                if (fDistFromCam < 35.0f)
                    return;
                if (fDistFromCam < 50.0f)
                    A *= static_cast<unsigned char>((fDistFromCam - 35.0f) * (2.0f / 3.0f));
            }

            // Is corona already present?
            CRegisteredCorona* pSuitableSlot;
            auto it = UsedMap.find(nID);

            if (it != UsedMap.end())
            {
                pSuitableSlot = it->second->GetFrom();

                if (pSuitableSlot->Intensity == 0 && A == 0)
                {
                    // Mark as free
                    it->second->GetFrom()->Identifier = 0;
                    it->second->Add(&FreeList);
                    UsedMap.erase(nID);
                    return;
                }
            }
            else
            {
                if (!A)
                    return;

                // Adding a new element
                auto	pNewEntry = FreeList.First();
                if (!pNewEntry)
                {
                    //LogToFile("ERROR: Not enough space for coronas!");
                    return;
                }

                pSuitableSlot = pNewEntry->GetFrom();

                // Add to used list and push this index to the map
                pNewEntry->Add(&UsedList);
                UsedMap[nID] = pNewEntry;

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
            pSuitableSlot->RegisteredThisFrame = true;
            pSuitableSlot->PullTowardsCam = PullTowardsCam;
            pSuitableSlot->FadeSpeed = FadeSpeed;

            pSuitableSlot->NeonFade = bNeonFade;
            pSuitableSlot->OnlyFromBelow = bOnlyFromBelow;
            pSuitableSlot->WhiteCore = bWhiteCore;

            pSuitableSlot->bIsAttachedToEntity = false;
            pSuitableSlot->pEntityAttachedTo = nullptr;
        }
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
                unsigned int	nIndex = pNode->GetFrom()->Identifier;
                auto			pNext = pNode->GetNextNode();

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

            // Initialise the lists
            FreeList.Init();
            UsedList.Init();

            for (size_t i = 0; i < aLinkedList.size(); i++)
            {
                aLinkedList[i].Add(&FreeList);
                aLinkedList[i].SetEntry(&aCoronas[i]);
            }
        }
    }

    static void RenderBuffered()
    {
        int nWidth = Scene->m_pRwCamera->frameBuffer->width;
        int nHeight = Scene->m_pRwCamera->frameBuffer->height;

        RwRaster* pLastRaster = nullptr;
        bool bLastZWriteRenderState = true;

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);

        for (size_t i = 0; i < aCoronas.size(); i++)
        {
            if (aCoronas[i].Identifier)
            {
                if (aCoronas[i].Intensity > 0)
                {
                    RwV3d	vecCoronaCoords, vecTransformedCoords;
                    float	fComputedWidth, fComputedHeight;

                    vecCoronaCoords.x = aCoronas[i].Coordinates.x;
                    vecCoronaCoords.y = aCoronas[i].Coordinates.y;
                    vecCoronaCoords.z = aCoronas[i].Coordinates.z;

                    if (CSprite::CalcScreenCoors(&vecCoronaCoords, &vecTransformedCoords, &fComputedWidth, &fComputedHeight, true, true))
                    {
                        aCoronas[i].OffScreen = !(vecTransformedCoords.x >= 0.0 && vecTransformedCoords.x <= nWidth &&
                            vecTransformedCoords.y >= 0.0 && vecTransformedCoords.y <= nHeight);

                        if (aCoronas[i].Intensity > 0 && vecTransformedCoords.z <= aCoronas[i].Range)
                        {
                            float fInvFarClip = 1.0f / vecTransformedCoords.z;
                            float fHalfRange = aCoronas[i].Range * 0.5f;

                            short nFadeIntensity = (short)(aCoronas[i].Intensity * (vecTransformedCoords.z > fHalfRange ? 1.0f - (vecTransformedCoords.z - fHalfRange) / fHalfRange : 1.0f));

                            if (bLastZWriteRenderState != aCoronas[i].LOSCheck == false)
                            {
                                bLastZWriteRenderState = aCoronas[i].LOSCheck == false;
                                CSprite::FlushSpriteBuffer();
                                RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)bLastZWriteRenderState);
                            }

                            if (aCoronas[i].pTex)
                            {
                                float fColourFogMult = std::min(40.0f, vecTransformedCoords.z) * CWeather::Foggyness * 0.025f + 1.0f;

                                if (aCoronas[i].Identifier == 1)	// Sun core
                                    vecTransformedCoords.z = Scene->m_pRwCamera->farPlane * 0.95f;

                                if (pLastRaster != RwTextureGetRaster(aCoronas[i].pTex))
                                {
                                    pLastRaster = RwTextureGetRaster(aCoronas[i].pTex);
                                    CSprite::FlushSpriteBuffer();

                                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pLastRaster);
                                }

                                RwV3d		vecCoronaCoordsAfterPull = vecCoronaCoords;
                                CVector		vecTempVector(vecCoronaCoordsAfterPull);
                                vecTempVector -= *GetCamPos();
                                vecTempVector.Normalise();

                                vecCoronaCoordsAfterPull.x -= (vecTempVector.x * aCoronas[i].PullTowardsCam);
                                vecCoronaCoordsAfterPull.y -= (vecTempVector.y * aCoronas[i].PullTowardsCam);
                                vecCoronaCoordsAfterPull.z -= (vecTempVector.z * aCoronas[i].PullTowardsCam);

                                if (CSprite::CalcScreenCoors(&vecCoronaCoordsAfterPull, &vecTransformedCoords, &fComputedWidth, &fComputedHeight, true, true))
                                {
                                    CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect(vecTransformedCoords.x, vecTransformedCoords.y, vecTransformedCoords.z, aCoronas[i].Size * fComputedHeight, aCoronas[i].Size * fComputedHeight * fColourFogMult, static_cast<uint8_t>(static_cast<float>(aCoronas[i].Red) / fColourFogMult), static_cast<uint8_t>(static_cast<float>(aCoronas[i].Green) / fColourFogMult), static_cast<uint8_t>(static_cast<float>(aCoronas[i].Blue) / fColourFogMult), nFadeIntensity, fInvFarClip * 20.0f, 0.0, 0xFF);
                                }
                            }
                        }
                    }
                    else
                        aCoronas[i].OffScreen = true;
                }
            }
        }

        CSprite::FlushSpriteBuffer();
    }

    static void RegisterLODLights()
    {
        if (!(CClock::GetIsTimeInRange(20, 7) && CGame::currArea == 0))
            return;

        static auto SolveEqSys = [](float a, float b, float c, float d, float value) -> float
        {
            float determinant = a - c;
            float x = (b - d) / determinant;
            float y = (a * d - b * c) / determinant;
            return std::min(x * value + y, d);
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

        for (auto it = m_Lampposts.cbegin(); it != m_Lampposts.cend(); ++it)
        {
            if (it->vecPos.z < -15.0f || it->vecPos.z > 1030.0f)
                continue;

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

            float distance = std::sqrt(fDistSqr);
            float fRadius;

            if (it->nNoDistance)
                fRadius = 1.75f;
            else
                fRadius = SolveEqSys(fEffectiveCoronaDist, 0.0f, fEffectiveDrawDistance, 1.75f, distance);

            if (bSlightlyIncreaseRadiusWithDistance)
                fRadius *= std::min(SolveEqSys(fEffectiveCoronaDist, 1.0f, REFERENCE_FAR_CLIP, 2.0f, distance), 2.0f);

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

            // Slightly decrease alpha for nearby coronas
            //if (bSlightlyDecreaseAlphaForNearbyCoro)
            {
                // Similar to radius logic: alpha multiplier ranges from 0.5 (near) to 1.0 (far)
                float alphaDistMult = std::min(0.5f + (distance - fEffectiveCoronaDist) / 1000.0f, 1.0f);
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
                    // Adjust these values to tune the effect:
                    // - Lower multiplier = more aggressive dimming
                    // - Higher clamp min = prevents coronas from getting too dim
                    radiusAlphaScale = std::clamp(1.0f / std::sqrt(finalRadius), 0.3f, 1.0f);
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
                            *(RwTexture**)0xC403F4, (CVector*)&it->vecPos,
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