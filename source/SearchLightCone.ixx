module;
#define NOMINMAX
#include <stdafx.h>

export module SearchLightCone;

import ComVars;
import LamppostInfo;
import Entity;
import Sprite;
import Camera;
import Clock;
import Game;
import Misc;

using RwV3D = RwV3d;

export void __cdecl Pre_SearchLightCone()
{
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATESRCBLEND, (void*)2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATESHADEMODE, (void*)2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)7);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
}

export void __cdecl Post_SearchLightCone()
{
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATECULLMODE, (void*)2);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)5);
    RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)2);
}

export void __cdecl SearchLightCone(RwV3D StartPoint, RwV3D EndPoint, float TargetRadius, float baseRadius, const CRGBA& color, float slAlpha, bool bAlphaGradient = true)
{
    static short TempBufferRenderIndexList[4096];
    static RxObjSpace3dVertex TempVertexBuffer[500];

    static RwRGBAReal visArray[130];
    static RwRGBAReal intensityArray[130];

    int          TempBufferIndicesStored = 0;
    unsigned int TempBufferVerticesStored = 0;

    RwV3D a = { EndPoint.x - StartPoint.x, EndPoint.y - StartPoint.y, EndPoint.z - StartPoint.z };
    a.Normalise();

    EndPoint.x += a.x * 3.0f;
    EndPoint.y += a.y * 3.0f;
    EndPoint.z += a.z * 3.0f;

    const RwV3D farPoint = {
        StartPoint.x + a.x * 100.0f,
        StartPoint.y + a.y * 100.0f,
        StartPoint.z + a.z * 100.0f
    };

    const RwV3D camPos = TheCamera->GetCoords();
    RwV3D camDir = { camPos.x - StartPoint.x, camPos.y - StartPoint.y, camPos.z - StartPoint.z };
    camDir.Normalise();

    float maxCosSqr = 0.0f;

    TempBufferIndicesStored = 0;
    TempBufferVerticesStored = 0;

    constexpr int NUM_STEPS = 64;
    const float angleStep = (2.0f * 3.1415926535f) / NUM_STEPS;

    for (int step = 0; step <= NUM_STEPS; ++step)
    {
        RwV3D b = { 0.0f, 0.0f, 1.0f };
        RwV3D out, perp2;
        out = a.Cross(b);
        out.Normalise();
        perp2 = out.Cross(a);
        perp2.Normalise();

        const float angle = static_cast<float>(step) * angleStep;
        const float sinVal = sinf(angle);
        const float cosVal = cosf(angle);

        const float baseX = StartPoint.x + (out.x * sinVal + perp2.x * cosVal) * baseRadius;
        const float baseY = StartPoint.y + (out.y * sinVal + perp2.y * cosVal) * baseRadius;
        const float baseZ = StartPoint.z + (out.z * sinVal + perp2.z * cosVal) * baseRadius;

        const float targetX = farPoint.x + (out.x * sinVal + perp2.x * cosVal) * TargetRadius;
        const float targetY = farPoint.y + (out.y * sinVal + perp2.y * cosVal) * TargetRadius;
        const float targetZ = farPoint.z + (out.z * sinVal + perp2.z * cosVal) * TargetRadius;

        const float t = (baseZ - EndPoint.z) / (baseZ - targetZ);
        float endX = baseX + (targetX - baseX) * t;
        float endY = baseY + (targetY - baseY) * t;
        float endZ = baseZ + (targetZ - baseZ) * t;

        float dx = endX - baseX, dy = endY - baseY, dz = endZ - baseZ;
        float lenSqr = dx * dx + dy * dy + dz * dz;
        if (lenSqr > 10000.0f)
        {
            float scale = 100.0f / std::sqrt(lenSqr);
            endX = baseX + dx * scale;
            endY = baseY + dy * scale;
            endZ = baseZ + dz * scale;
        }

        unsigned int idx = TempBufferVerticesStored;

        TempVertexBuffer[idx].objVertex = { baseX, baseY, baseZ };
        TempVertexBuffer[idx + 1].objVertex = { endX, endY, endZ };

        RwV3D ray = { baseX - StartPoint.x, baseY - StartPoint.y, baseZ - StartPoint.z };
        ray.Normalise();
        float dot = ray.x * camDir.x + ray.y * camDir.y + ray.z * camDir.z;
        if (dot < 0.0f) dot = -dot;
        float cosSqr = dot * dot;
        if (cosSqr > maxCosSqr) maxCosSqr = cosSqr;

        visArray[idx].red = cosSqr;
        visArray[idx].green = cosSqr;

        // Store gradient factor: 1.0 at top (base), fade out BEFORE reaching bottom
        float gradientTop = 1.0f;  // Top vertex: full alpha

        // Calculate how far down this vertex is (0.0 = top, 1.0 = bottom)
        float verticalProgress = (baseZ - endZ) / (StartPoint.z - EndPoint.z);
        if (verticalProgress < 0.0f) verticalProgress = 0.0f;
        if (verticalProgress > 1.0f) verticalProgress = 1.0f;

        // Fade out to reach 0 at 50% down the cone
        float gradientBottom = bAlphaGradient ? std::max(0.0f, 1.0f - (verticalProgress * 2.0f)) : 1.0f;

        intensityArray[idx].red = slAlpha * 0.15000001f + 0.1f;
        intensityArray[idx].green = gradientTop;  // Store gradient for top vertex

        intensityArray[idx + 1].red = slAlpha * 0.15000001f + 0.1f;
        intensityArray[idx + 1].green = gradientBottom; // Store gradient for bottom vertex - fades to 0 at 50%

        if (step != NUM_STEPS)
        {
            int i = TempBufferIndicesStored;
            TempBufferRenderIndexList[i++] = static_cast<short>(idx);
            TempBufferRenderIndexList[i++] = static_cast<short>(idx + 3);
            TempBufferRenderIndexList[i++] = static_cast<short>(idx + 1);

            if (baseRadius > 0.0f)
            {
                TempBufferRenderIndexList[i++] = static_cast<short>(idx);
                TempBufferRenderIndexList[i++] = static_cast<short>(idx + 2);
                TempBufferRenderIndexList[i++] = static_cast<short>(idx + 3);
            }
            TempBufferIndicesStored = i;
        }

        TempBufferVerticesStored += 2;
    }

    const unsigned int numVerts = TempBufferVerticesStored;
    if (numVerts > 0)
    {
        const float globalScale = slAlpha / (maxCosSqr > 0.0f ? maxCosSqr : 1.0f);

        for (unsigned int i = 0; i < numVerts; ++i)
        {
            // Apply gradient multiplier stored in intensityArray[i].green
            float gradientFactor = intensityArray[i].green;
            float intensity = visArray[i].red * intensityArray[i].red * globalScale * gradientFactor;
            if (intensity > 1.0f) intensity = 1.0f;

            unsigned char a = static_cast<unsigned char>(255.0f * intensity);
            unsigned char r = static_cast<unsigned char>(color.r * intensity);
            unsigned char g = static_cast<unsigned char>(color.g * intensity);
            unsigned char b = static_cast<unsigned char>(color.b * intensity);

            TempVertexBuffer[i].color = b | (g << 8) | (r << 16) | (a << 24);
        }
    }

    if (TempBufferIndicesStored > 0 && RwIm3DTransform(TempVertexBuffer, numVerts, nullptr, 0x18u))
    {
        RwIm3DRenderIndexedPrimitive(3, TempBufferRenderIndexList, TempBufferIndicesStored);
        RwIm3DEnd();
    }
}

export void RenderAllSearchLights()
{
    if ((bRenderOnlyDuringFoggyWeather && CWeather::Foggyness) || !bRenderOnlyDuringFoggyWeather)
    {
        if (CClock::GetIsTimeInRange(20, 7) && CGame::currArea == 0)
        {
            Pre_SearchLightCone();

            for (const auto& lamp : m_Lampposts)
            {
                if (!lamp.nDrawSearchlight)
                    continue;

                const CVector camPos = TheCamera->GetCoords();
                const float fDistSqr = (camPos - lamp.vecPos).MagnitudeSqr();
                const float fDist = std::sqrt(fDistSqr);

                // Rendering range with smooth fade zones
                constexpr float MIN_DIST = 45.0f;
                constexpr float FADE_IN_END = 65.0f;
                constexpr float FADE_OUT_START = 280.0f;
                constexpr float MAX_DIST = 300.0f;

                // Hard cutoffs at extremes
                if (fDist < MIN_DIST || fDist > MAX_DIST)
                    continue;

                float fVisibility = 1.0f;

                // Smooth fade-in when entering render range
                if (fDist < FADE_IN_END)
                {
                    float t = (fDist - MIN_DIST) / (FADE_IN_END - MIN_DIST);
                    fVisibility = t * t * (3.0f - 2.0f * t); // Smoothstep
                }
                // Smooth fade-out when leaving render range
                else if (fDist > FADE_OUT_START)
                {
                    float t = (MAX_DIST - fDist) / (MAX_DIST - FADE_OUT_START);
                    fVisibility = t * t * (3.0f - 2.0f * t); // Smoothstep
                }

                // Apply global visibility factor
                fVisibility *= fSearchlightEffectVisibilityFactor;

                if (fVisibility < 0.01f)
                    continue;

                RwV3D EndPoint = *reinterpret_cast<const RwV3D*>(&lamp.vecPos);
                EndPoint.z -= static_cast<float>(lamp.nDrawSearchlight); // object height

                const float height = lamp.vecPos.z - EndPoint.z;
                const float targetRadius = std::min(8.0f * height, 90.0f);
                const float baseRadius = lamp.fCustomSizeMult / 6.0f;

                SearchLightCone(*reinterpret_cast<const RwV3D*>(&lamp.vecPos), EndPoint, targetRadius, baseRadius, lamp.colour, fVisibility, true);
            }

            Post_SearchLightCone();
        }
    }
}
