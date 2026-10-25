module;

#include <filesystem>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <cstdint>
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

    int numCoronas = 50000;
    bool bRenderLodLights;
    float fCoronaRadiusMultiplier;
    float fCoronaRadiusBase = 1.0f; // the game's own light size, set by the plugin before the settings are read
    bool bSlightlyIncreaseRadiusWithDistance = true;
    float fCoronaFarClip = 0.0f;
    bool autoFarClip = true;
    bool bRenderStaticShadowsForLODs;
    bool bIncreasePedsCarsShadowsDrawDistance;
    float fStaticShadowsIntensity, fTrafficLightsShadowsIntensity;
    bool bRenderSearchlightEffects;
    bool bRenderOnlyDuringFoggyWeather = false;
    float fSearchlightEffectVisibilityFactor = 0.4f;
    bool bRandomExplosionEffects, bReplaceSmokeTrailWithBulletTrail;
    bool bPreloadLODs;

    // Extra outdoor camera reach in metres; 0 keeps the timecycle.
    float fViewDistance = 1000.0f;
    // Model LOD distance, independent of the weather
    float fDrawDistance = 1.0f;
    // Non-LOD objects and vegetation
    float fDetailDistance = 1.0f;
    // Static, ped and vehicle shadows (read once)
    float fShadowDistance = 1.0f;
    // 0 off, 1 lights only, 2 vehicles and boats
    int nDistantTraffic = 2;
    // Actual far clip ratio, used to preserve the sun size
    float fFarClipMultiplier = 1.0f;
    // 300 m cap of non-LOD objects, scaled by fDetailDistance
    float fMaxDrawDistanceForNormalObjects = 300.0f;

    float fCoronaAlphaNearMinMult = 0.50f;
    float fCoronaAlphaReachOneAt = 150.0f;
    float fCoronaAlphaBoostStartAt = 150.0f;
    float fCoronaAlphaFarBoostMax = 4.0f;

    int nNumDistantCarImpostors = 1000;
    float fDistantCarsRadiusMultiplier = 1.0f;
    bool bDistantMaritimeTraffic = true;
    bool bDistantCars3D = true;
    bool bDistantBoats3D = true;
    bool bDistantCars3DLights = true;
    float fDistantCarsGroundOffset = 0.0f;

    // Called after the settings are read, for values the plugin writes into the game.
    void (*OnSettingsApplied)() = nullptr;

    // The limit adjuster grows the game's lists as the draw distance needs. Without it the game keeps its own
    // limits, so the distances stay close to the game's.
    bool LimitAdjusterPresent()
    {
        static bool present = false;
        if (present)
            return true;
        HMODULE modules[1024];
        DWORD needed = 0;
        if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed))
        {
            for (DWORD i = 0; i < needed / sizeof(HMODULE) && i < 1024; i++)
            {
                if (GetProcAddress(modules[i], "OpenLA_GetVersion"))
                {
                    present = true;
                    break;
                }
            }
        }
        return present;
    }

    // Streaming memory chosen by the limit adjuster in bytes, 0 when unknown.
    uint32_t LimitAdjusterStreamingBudget()
    {
        HMODULE modules[1024];
        DWORD needed = 0;
        if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed))
        {
            for (DWORD i = 0; i < needed / sizeof(HMODULE) && i < 1024; i++)
            {
                if (auto fn = (uint32_t(*)())GetProcAddress(modules[i], "OpenLA_GetStreamingBudget"))
                    return fn();
            }
        }
        return 0;
    }

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

        // DISTANCE
        fViewDistance = std::clamp(float(Int("DISTANCE.ViewDistance", 1000)), 0.0f, 3000.0f);
        fDrawDistance = std::clamp(Float("DISTANCE.DrawDistance", 2.5f), 1.0f, 5.0f);
        fDetailDistance = std::clamp(Float("DISTANCE.DetailDistance", 2.0f), 1.0f, 4.0f);
        if (!LimitAdjusterPresent())
        {
            fViewDistance = (std::min)(fViewDistance, 500.0f);
            fDrawDistance = (std::min)(fDrawDistance, 1.5f);
            fDetailDistance = (std::min)(fDetailDistance, 1.5f);
        }
        fMaxDrawDistanceForNormalObjects = 300.0f * fDetailDistance;

        // LIGHTS
        bRenderLodLights = Bool("LIGHTS.DistantLights", true);
        bRenderStaticShadowsForLODs = Bool("LIGHTS.DistantLightShadows", true);
        bRenderSearchlightEffects = Bool("LIGHTS.Searchlights", true);
        nDistantTraffic = std::clamp(Int("LIGHTS.DistantTraffic", 2), 0, 2);
        fCoronaRadiusMultiplier = fCoronaRadiusBase * std::clamp(Float("LIGHTS.DistantLightSize", 1.0f), 0.25f, 4.0f);
        bDistantMaritimeTraffic = nDistantTraffic > 0;
        bDistantCars3D = nDistantTraffic == 2;
        bDistantBoats3D = nDistantTraffic == 2;
        bDistantCars3DLights = nDistantTraffic == 2;

        // Settings that apply at the next start, read once: the game was patched with them
        if (initial)
        {
            fShadowDistance = std::clamp(Float("DISTANCE.ShadowDistance", 4.0f), 1.0f, 8.0f);
            bIncreasePedsCarsShadowsDrawDistance = fShadowDistance >= 2.0f;
            // Brighter game light pools so the distant ones hand over to them seamlessly; 0 keeps the game's
            const bool distantPools = Bool("LIGHTS.DistantLightShadows", true);
            fStaticShadowsIntensity = distantPools ? 2.0f * 0.00390625f : 0.0f;
            fTrafficLightsShadowsIntensity = distantPools ? 0.2f : 0.0f;
            bPreloadLODs = fDrawDistance > 1.0f;
            bRandomExplosionEffects = Bool("EFFECTS.RandomExplosionEffects", true);
            bReplaceSmokeTrailWithBulletTrail = Bool("EFFECTS.ReplaceSmokeTrailWithBulletTrail", true);
            initial = false;
        }

        if (OnSettingsApplied)
            OnSettingsApplied();

        // Changes from menus, editors and hand edits of the settings file (read again by the live link's watcher) apply
        // at once
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
        });
    }
}
