module;

#include <FileWatch.hpp>
#include <filesystem>
#include "IniReader.h"

export module ComVars;

export
{
    int numCoronas = 25000;
    bool bRenderLodLights;
    float fCoronaRadiusMultiplier;
    bool bSlightlyIncreaseRadiusWithDistance;
    float fCoronaFarClip;
    bool autoFarClip;
    bool bRenderStaticShadowsForLODs;
    bool bIncreasePedsCarsShadowsDrawDistance;
    float fStaticShadowsIntensity, fStaticShadowsDrawDistance;
    float fTrafficLightsShadowsIntensity, fTrafficLightsShadowsDrawDistance;
    bool bRenderSearchlightEffects;
    bool bRenderOnlyDuringFoggyWeather;
    bool bRenderHeliSearchlights;
    int nSmoothEffect;
    float fSearchlightEffectVisibilityFactor;
    bool bAdaptiveDrawDistanceEnabled;
    float fNewFarClip, fMaxPossibleDrawDistance;
    float fMaxDrawDistanceForNormalObjects, fTimedObjectsDrawDistance, fNeonsDrawDistance, fLODObjectsDrawDistance;
    float fGenericObjectsDrawDistance, fAllNormalObjectsDrawDistance, fVegetationDrawDistance;
    bool bLoadAllBinaryIPLs, bPreloadLODs;
    float fDrawDistance;
    bool bRandomExplosionEffects, bReplaceSmokeTrailWithBulletTrail;
    float fStaticSunSize;

    float fFarClipMultiplier = 1.0f;
    float fFarClipStaticMultiplier;
    float fFarClipHeightFactor;
    float fFarClipMinMultiplier;
    float fFarClipMaxMultiplier;
    unsigned int nFarClipTargetFPS;

    float fCoronaAlphaNearMinMult = 0.50f;
    float fCoronaAlphaReachOneAt = 350.0f;
    float fCoronaAlphaBoostStartAt = 700.0f;
    float fCoronaAlphaFarBoostMax = 4.0f;

    int nNumDistantCarImpostors = 2000;
    float fDistantCarsRadiusMultiplier = 1.0f;

    void ReadIniSettings()
    {
        CIniReader iniReader("");

        // LodLights section
        bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
        numCoronas = iniReader.ReadInteger("LodLights", "MaxNumberOfLodLights", 25000);
        fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
        bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
        if (iniReader.ReadString("LodLights", "CoronaFarClip", "auto") != "auto")
            fCoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
        else
            autoFarClip = true;
        fCoronaAlphaNearMinMult = iniReader.ReadFloat("LodLights", "CoronaAlphaNearMinMult", 0.50f);
        fCoronaAlphaReachOneAt = iniReader.ReadFloat("LodLights", "CoronaAlphaReachOneAt", 350.0f);
        fCoronaAlphaBoostStartAt = iniReader.ReadFloat("LodLights", "CoronaAlphaBoostStartAt", 700.0f);
        fCoronaAlphaFarBoostMax = iniReader.ReadFloat("LodLights", "CoronaAlphaFarBoostMax", 4.0f);
        nNumDistantCarImpostors = iniReader.ReadInteger("LodLights", "MaxNumberOfDistantCars", 2000);
        fDistantCarsRadiusMultiplier = iniReader.ReadFloat("LodLights", "DistantCarsRadiusMultiplier", 1.0f);

        // StaticShadows section
        bRenderStaticShadowsForLODs = iniReader.ReadInteger("StaticShadows", "RenderStaticShadowsForLODs", 0) != 0;
        bIncreasePedsCarsShadowsDrawDistance = iniReader.ReadInteger("StaticShadows", "IncreaseCarsShadowsDrawDistance", 0) != 0;
        fStaticShadowsIntensity = iniReader.ReadFloat("StaticShadows", "StaticShadowsIntensity", 0.0f);
        fStaticShadowsIntensity *= 0.00390625f;
        fStaticShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "StaticShadowsDrawDistance", 0.0f);
        fTrafficLightsShadowsIntensity = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsIntensity", 0.0f);
        fTrafficLightsShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsDrawDistance", 0.0f);

        // SearchLights section
        bRenderSearchlightEffects = iniReader.ReadInteger("SearchLights", "RenderSearchlightEffects", 1) != 0;
        bRenderOnlyDuringFoggyWeather = iniReader.ReadInteger("SearchLights", "RenderOnlyDuringFoggyWeather", 0) != 0;
        bRenderHeliSearchlights = iniReader.ReadInteger("SearchLights", "RenderHeliSearchlights", 1) != 0;
        nSmoothEffect = iniReader.ReadInteger("SearchLights", "SmoothEffect", 1);
        fSearchlightEffectVisibilityFactor = iniReader.ReadFloat("SearchLights", "SearchlightEffectVisibilityFactor", 0.4f);

        // FarClip section
        fFarClipStaticMultiplier = iniReader.ReadFloat("FarClip", "FarClipMultiplier", 1.0f);
        fFarClipHeightFactor = iniReader.ReadFloat("FarClip", "HeightFactor", 0.05f);
        fFarClipMinMultiplier = iniReader.ReadFloat("FarClip", "MinMultiplier", 1.0f);
        fFarClipMaxMultiplier = iniReader.ReadFloat("FarClip", "MaxMultiplier", 30.0f);
        nFarClipTargetFPS = iniReader.ReadInteger("FarClip", "TargetFPS", 60);
        fStaticSunSize = iniReader.ReadFloat("FarClip", "StaticSunSize", 20.0f);

        // IDETweaker section
        fMaxDrawDistanceForNormalObjects = iniReader.ReadFloat("IDETweaker", "MaxDrawDistanceForNormalObjects", 300.0f);
        fTimedObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "TimedObjectsDrawDistance", 0.0f);
        fNeonsDrawDistance = iniReader.ReadFloat("IDETweaker", "NeonsDrawDistance", 0.0f);
        fLODObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "LODObjectsDrawDistance", 0.0f);
        fGenericObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "GenericObjectsDrawDistance", 0.0f);
        fAllNormalObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "AllNormalObjectsDrawDistance", 0.0f);
        fVegetationDrawDistance = iniReader.ReadFloat("IDETweaker", "VegetationDrawDistance", 0.0f);
        fDrawDistance = iniReader.ReadFloat("IDETweaker", "DrawDistance", 0.0f);
        bPreloadLODs = iniReader.ReadInteger("IDETweaker", "PreloadLODs", 0) != 0;

        // Misc section
        bRandomExplosionEffects = iniReader.ReadInteger("Misc", "RandomExplosionEffects", 0) != 0;
        bReplaceSmokeTrailWithBulletTrail = iniReader.ReadInteger("Misc", "ReplaceSmokeTrailWithBulletTrail", 0) != 0;

        auto path = iniReader.GetIniPath().wstring();
        static std::once_flag watchFlag;
        std::call_once(watchFlag, [path]()
        {
            std::thread([path]()
            {
                if (std::filesystem::exists(path))
                {
                    static filewatch::FileWatch<std::wstring> watch(path, [](const std::wstring& path, const filewatch::Event change_type)
                    {
                        if (change_type == filewatch::Event::modified)
                        {
                            ReadIniSettings();
                        }
                    });
                }
            }).detach();
        });
    }
}