module;

#include <FileWatch.hpp>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <windows.h>
#include "asi_settings/asi_settings.hpp"

export module ComVars;

export
{
    struct RwIm2DVertex
    {
        float x, y, z, rhw;
        uint8_t b, g, r, a;
        float u, v;
    };

    enum RwPrimitiveType
    {
        rwPRIMTYPENAPRIMTYPE = 0,
        rwPRIMTYPELINELIST = 1,
        rwPRIMTYPEPOLYLINE = 2,
        rwPRIMTYPETRILIST = 3,
        rwPRIMTYPETRISTRIP = 4,
        rwPRIMTYPETRIFAN = 5,
        rwPRIMTYPEPOINTLIST = 6,
    };

    int (__cdecl*RwIm2DRenderIndexedPrimitive)(
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
    float fFarClipHeightFactor;
    float fFarClipMinMultiplier;
    float fFarClipMaxMultiplier;
    unsigned int nFarClipTargetFPS;

    float fCoronaAlphaNearMinMult = 0.50f;
    float fCoronaAlphaReachOneAt = 350.0f;
    float fCoronaAlphaBoostStartAt = 700.0f;
    float fCoronaAlphaFarBoostMax = 4.0f;

    int nNumDistantCarImpostors = 1000;
    float fDistantCarsRadiusMultiplier = 1.0f;
    bool bDistantMaritimeTraffic = true;
    bool bDistantCars3D = false;
    bool bDistantBoats3D = true;
    bool bDistantCars3DLights = true;
    float fDistantCarsGroundOffset = 0.0f;

    // The settings of the plugin (ASI Settings, external/FusionFixUtils/asi_settings): described in
    // settings/<plugin>.json, embedded with its translations (source/<plugin>/Settings.rc), stored in <plugin>.cfg next to
    // the plugin. Menu hosts, overlays and FusionFixHub change them through the AsiSettings_GetApi export.
    asi_settings::Plugin& Settings()
    {
        static asi_settings::Plugin plugin;
        static bool loaded = []()
        {
            if (!plugin.Load())
                OutputDebugStringA(("Project2DFX: settings unavailable, " + plugin.Error() + "\n").c_str());
            else
                asi_settings::LiveLink::Start();
            return plugin.Loaded();
        }();
        (void)loaded;
        return plugin;
    }

    // The values in effect; a setting the game's description doesn't have keeps its default here
    void ReadSettings()
    {
        static bool initial = true;
        auto& s = Settings();
        auto has = [&](const char* id) { return s.Loaded() && s.GetDescription().Find(id) != nullptr; };
        auto Int = [&](const char* id, int def) { return has(id) ? s.Int(id) : def; };
        auto Bool = [&](const char* id, bool def) { return has(id) ? s.Int(id) != 0 : def; };
        auto Float = [&](const char* id, float def) { return has(id) ? s.Float(id) : def; };

        // LODLIGHTS
        bRenderLodLights = Bool("LODLIGHTS.RenderLodLights", true);
        fCoronaRadiusMultiplier = Float("LODLIGHTS.CoronaRadiusMultiplier", 1.0f);
        bSlightlyIncreaseRadiusWithDistance = Bool("LODLIGHTS.SlightlyIncreaseRadiusWithDistance", true);
        fCoronaFarClip = Float("LODLIGHTS.CoronaFarClip", 0.0f);
        autoFarClip = fCoronaFarClip <= 0.0f;
        fCoronaAlphaNearMinMult = Float("LODLIGHTS.CoronaAlphaNearMinMult", 0.50f);
        fCoronaAlphaReachOneAt = Float("LODLIGHTS.CoronaAlphaReachOneAt", 350.0f);
        fCoronaAlphaBoostStartAt = Float("LODLIGHTS.CoronaAlphaBoostStartAt", 700.0f);
        fCoronaAlphaFarBoostMax = Float("LODLIGHTS.CoronaAlphaFarBoostMax", 4.0f);
        fDistantCarsRadiusMultiplier = Float("LODLIGHTS.DistantCarsRadiusMultiplier", 1.0f);
        bDistantMaritimeTraffic = Bool("LODLIGHTS.MaritimeTraffic", true);
        bDistantCars3D = Bool("LODLIGHTS.DistantCars3D", false);
        bDistantBoats3D = Bool("LODLIGHTS.DistantBoats3D", true);
        bDistantCars3DLights = Bool("LODLIGHTS.DistantCars3DLights", true);
        fDistantCarsGroundOffset = Float("LODLIGHTS.DistantCarsGroundOffset", 0.0f);
        if (!std::isfinite(fDistantCarsGroundOffset)) fDistantCarsGroundOffset = 0.0f;
        fDistantCarsGroundOffset = (std::clamp)(fDistantCarsGroundOffset, -2.0f, 2.0f);

        // STATICSHADOWS
        bRenderStaticShadowsForLODs = Bool("STATICSHADOWS.RenderStaticShadowsForLODs", false);

        // SEARCHLIGHTS
        bRenderSearchlightEffects = Bool("SEARCHLIGHTS.RenderSearchlightEffects", true);
        bRenderOnlyDuringFoggyWeather = Bool("SEARCHLIGHTS.RenderOnlyDuringFoggyWeather", false);
        fSearchlightEffectVisibilityFactor = Float("SEARCHLIGHTS.SearchlightEffectVisibilityFactor", 0.4f);

        // FARCLIP
        fFarClipMultiplier = Float("FARCLIP.FarClipMultiplier", 1.0f);
        fFarClipHeightFactor = Float("FARCLIP.HeightFactor", 0.05f);
        fFarClipMinMultiplier = Float("FARCLIP.MinMultiplier", 1.0f);
        fFarClipMaxMultiplier = Float("FARCLIP.MaxMultiplier", 30.0f);
        nFarClipTargetFPS = Int("FARCLIP.TargetFPS", 60);

        // Settings that apply at the next start, read once: the game was patched with them
        if (initial)
        {
            numCoronas = Int("LODLIGHTS.MaxNumberOfLodLights", 25000);
            nNumDistantCarImpostors = Int("LODLIGHTS.MaxNumberOfDistantCars", 1000);
            bIncreasePedsCarsShadowsDrawDistance = Bool("STATICSHADOWS.IncreasePedsCarsShadowsDrawDistance", false);
            fStaticShadowsIntensity = Float("STATICSHADOWS.StaticShadowsIntensity", 0.0f);
            fStaticShadowsIntensity *= 0.00390625f;
            fStaticShadowsDrawDistance = Float("STATICSHADOWS.StaticShadowsDrawDistance", 0.0f);
            fTrafficLightsShadowsIntensity = Float("STATICSHADOWS.TrafficLightsShadowsIntensity", 0.0f);
            fTrafficLightsShadowsDrawDistance = Float("STATICSHADOWS.TrafficLightsShadowsDrawDistance", 0.0f);
            fStaticSunSize = Float("FARCLIP.StaticSunSize", 20.0f);
            fMaxDrawDistanceForNormalObjects = Float("IDETWEAKER.MaxDrawDistanceForNormalObjects", 300.0f);
            fTimedObjectsDrawDistance = Float("IDETWEAKER.TimedObjectsDrawDistance", 0.0f);
            fNeonsDrawDistance = Float("IDETWEAKER.NeonsDrawDistance", 0.0f);
            fLODObjectsDrawDistance = Float("IDETWEAKER.LODObjectsDrawDistance", 0.0f);
            fGenericObjectsDrawDistance = Float("IDETWEAKER.GenericObjectsDrawDistance", 0.0f);
            fAllNormalObjectsDrawDistance = Float("IDETWEAKER.AllNormalObjectsDrawDistance", 0.0f);
            fVegetationDrawDistance = Float("IDETWEAKER.VegetationDrawDistance", 0.0f);
            fDrawDistance = Float("IDETWEAKER.DrawDistance", 0.0f);
            bPreloadLODs = Bool("IDETWEAKER.PreloadLODs", false);
            bRandomExplosionEffects = Bool("MISC.RandomExplosionEffects", false);
            bReplaceSmokeTrailWithBulletTrail = Bool("MISC.ReplaceSmokeTrailWithBulletTrail", false);
            initial = false;
        }

        // Changes from menus, editors and the settings file apply at once; a hand-edited file is read again
        static std::once_flag flag;
        std::call_once(flag, [&s]()
        {
            if (!s.Loaded())
                return;
            for (auto& setting : s.GetDescription().settings)
            {
                if (setting.apply != "restart")
                    s.OnChange(setting.id, [] { ReadSettings(); });
            }
            if (std::filesystem::exists(s.CfgPath()))
            {
                static filewatch::FileWatch<std::wstring> watch(s.CfgPath().wstring(), [](const std::wstring&, const filewatch::Event change)
                {
                    if (change == filewatch::Event::modified || change == filewatch::Event::added || change == filewatch::Event::renamed_new)
                        Settings().Reload();
                });
            }
        });
    }
}
