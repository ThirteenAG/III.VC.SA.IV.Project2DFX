#define NOMINMAX
#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include "safetyhook.hpp"
#include "FileWatch.hpp"
#include "asi_settings/asi_settings.hpp"

import CoronaLimit;
import WplInstance;
import Game;

// The settings of the plugin (ASI Settings, external/FusionFixUtils/asi_settings): described in
// settings/IVLodLights.json, embedded with its translations (source/IVLodLights/Settings.rc), stored in IVLodLights.cfg
// next to the plugin. Fusion Fix's menu and overlay and FusionFixHub change them through the AsiSettings_GetApi export.
asi_settings::Plugin& Settings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = []()
    {
        if (!plugin.Load())
            OutputDebugStringA(("IVLodLights: settings unavailable, " + plugin.Error() + "\n").c_str());
        else
            asi_settings::LiveLink::Start();
        return plugin.Loaded();
    }();
    (void)loaded;
    return plugin;
}

ASI_SETTINGS_EXPORT(Settings())

// The values in effect, which follow every change while the game runs
struct
{
    const int* enabled;
    const float* radiusMultiplier;
    const float* alphaMultiplier;
    const int* increaseRadiusWithDistance;
    const int* disableDefaultLodLights;
    const float* drawDistance;
} Values;

void BindSettings()
{
    auto& s = Settings();
    Values.enabled = &s.Int("LODLIGHTS.RenderLodLights");
    Values.radiusMultiplier = &s.Float("LODLIGHTS.CoronaRadiusMultiplier");
    Values.alphaMultiplier = &s.Float("LODLIGHTS.CoronaAlphaMultiplier");
    Values.increaseRadiusWithDistance = &s.Int("LODLIGHTS.SlightlyIncreaseRadiusWithDistance");
    Values.disableDefaultLodLights = &s.Int("LODLIGHTS.DisableDefaultLodLights");
    Values.drawDistance = &s.Float("LODLIGHTS.DrawDistance");

    // A hand-edited settings file is read again while the game runs; the plugin's own writes change nothing
    if (s.Loaded() && std::filesystem::exists(s.CfgPath()))
    {
        static filewatch::FileWatch<std::wstring> watch(s.CfgPath().wstring(), [](const std::wstring&, const filewatch::Event change)
        {
            if (change == filewatch::Event::modified || change == filewatch::Event::added || change == filewatch::Event::renamed_new)
                Settings().Reload();
        });
    }
}

struct LightColour
{
    uint8_t r, g, b, a;
};

enum BlinkTypes
{
    DEFAULT,
    RANDOM_FLASHING,
    T_1S_ON_1S_OFF,
    T_2S_ON_2S_OFF,
    T_3S_ON_3S_OFF,
    T_4S_ON_4S_OFF,
    T_5S_ON_5S_OFF,
    T_6S_ON_4S_OFF
};

struct CLamppostInfo
{
    CVector vecPos;
    LightColour colour;
    float fCustomSizeMult;
    int nCoronaShowMode;
    int nNoDistance;
    int nDrawSearchlight;
    float fHeading;
    float fObjectDrawDistance;
};

std::multimap<unsigned int, CLamppostInfo> FileContent;
std::vector<CLamppostInfo> m_Lampposts;
std::atomic<bool> m_bCatchLamppostsNow = false;

// The lights of the models (IVLodLights.dat next to the plugin), or of the Vice City conversion's map
// (IVLodLights.ViceCity.dat) when its text is loaded
void LoadDatFile()
{
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&LoadDatFile), &module);
    wchar_t buffer[MAX_PATH * 4] = {};
    GetModuleFileNameW(module, buffer, DWORD(std::size(buffer)));
    std::filesystem::path path(buffer);
    path.replace_extension(HasViceCityStrings() ? L".ViceCity.dat" : L".dat");

    std::ifstream file(path);
    if (!file)
        return;

    unsigned int nModel = 0xFFFFFFFF;
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#')
            continue;
        line.erase(0, start);

        if (line[0] == '%')
        {
            nModel = line != "%additional_coronas" ? HashStringLowercase(line.c_str() + 1) : 0xFFFFFFFF;
            continue;
        }

        CLamppostInfo info{};
        unsigned int nRed = 0, nGreen = 0, nBlue = 0, nAlpha = 0;
        float fOffsetX = 0.0f, fOffsetY = 0.0f, fOffsetZ = 0.0f;
        info.fCustomSizeMult = 1.0f;
        if (sscanf(line.c_str(), "%3u %3u %3u %3u %f %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &info.fCustomSizeMult, &info.fObjectDrawDistance, &info.nCoronaShowMode, &info.nNoDistance, &info.nDrawSearchlight) != 12)
            sscanf(line.c_str(), "%3u %3u %3u %3u %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &info.fCustomSizeMult, &info.nCoronaShowMode, &info.nNoDistance, &info.nDrawSearchlight);
        info.vecPos = CVector(fOffsetX, fOffsetY, fOffsetZ);
        info.colour = { uint8_t(nRed), uint8_t(nGreen), uint8_t(nBlue), uint8_t(nAlpha) };
        FileContent.emplace(nModel, info);
    }

    // Lights that belong to no model are at their place in the world
    auto range = FileContent.equal_range(0xFFFFFFFF);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto info = it->second;
        info.fObjectDrawDistance = 0.0f;
        m_Lampposts.push_back(info);
    }

    m_bCatchLamppostsNow = true;
}

void RegisterLamppost(WplInstance* pObj, std::pair<std::multimap<unsigned int, CLamppostInfo>::iterator, std::multimap<unsigned int, CLamppostInfo>::iterator> range)
{
    // Objects around this point get no distant lights
    {
        float dx = pObj->PositionX - -278.37f;
        float dy = pObj->PositionY - -1377.48f;
        float dz = pObj->PositionZ - 90.98f;
        if ((dx * dx + dy * dy + dz * dz) <= (300.0f * 300.0f))
            return;
    }

    float qw = pObj->RotationW;
    float qx = pObj->RotationX;
    float qy = pObj->RotationY;
    float qz = pObj->RotationZ;

    float n = 1.0f / sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    qx *= n;
    qy *= n;
    qz *= n;
    qw *= n;

    // The object's axes from its rotation: the light's offset x, y, z go along right, up and at
    CVector right(1.0f - 2.0f * qy * qy - 2.0f * qz * qz, 2.0f * qx * qy - 2.0f * qz * qw, 2.0f * qx * qz + 2.0f * qy * qw);
    CVector up(2.0f * qx * qy + 2.0f * qz * qw, 1.0f - 2.0f * qx * qx - 2.0f * qz * qz, 2.0f * qy * qz - 2.0f * qx * qw);
    CVector at(2.0f * qx * qz - 2.0f * qy * qw, 2.0f * qy * qz + 2.0f * qx * qw, 1.0f - 2.0f * qx * qx - 2.0f * qy * qy);
    CVector pos(pObj->PositionX, pObj->PositionY, pObj->PositionZ);

    float heading = atan2(up.y, -up.x);
    for (auto it = range.first; it != range.second; ++it)
    {
        auto info = it->second;
        auto& offset = it->second.vecPos;
        info.vecPos = pos + right * offset.x + up * offset.y + at * offset.z;
        info.fHeading = heading;
        m_Lampposts.push_back(info);
    }
}

void PossiblyAddThisEntity(WplInstance* pInstance)
{
    static std::once_flag flag;
    std::call_once(flag, LoadDatFile);

    if (m_bCatchLamppostsNow)
    {
        auto range = FileContent.equal_range(pInstance->ModelNameHash);
        if (range.first != range.second)
            RegisterLamppost(pInstance, range);
    }
}

// How much of a blinking light shows now: on/off times of its mode, with soft edges; every light of a mode blinks
// together
float BlinkVisibility(int mode)
{
    uint32_t on = 0, off = 0;
    switch (mode)
    {
        case RANDOM_FLASHING: on = 500; off = 500; break;
        case T_1S_ON_1S_OFF: on = 1000; off = 1000; break;
        case T_2S_ON_2S_OFF: on = 2000; off = 2000; break;
        case T_3S_ON_3S_OFF: on = 3000; off = 3000; break;
        case T_4S_ON_4S_OFF: on = 4000; off = 4000; break;
        case T_5S_ON_5S_OFF: on = 5000; off = 5000; break;
        case T_6S_ON_4S_OFF: on = 6000; off = 4000; break;
        default: return 1.0f;
    }

    uint32_t phase = *CTimer::m_snTimeInMilliseconds % (on + off);
    if (phase >= on)
        return 0.0f;
    float edge = std::min(500.0f, on * 0.5f);
    float fade = std::clamp(std::min(float(phase), float(on - phase)) / edge, 0.0f, 1.0f);
    return fade * fade * (3.0f - 2.0f * fade);
}

// The lights show from 19:00 to 8:00 of the rendered time; the game's own distant lights are hidden only then
bool ProjectLightsDrawn()
{
    if (!*Values.enabled || m_Lampposts.empty())
        return false;
    int hour = RenderedHours();
    return !(hour < 19 && hour > 7);
}

// The timecycle's distant corona brightness and size at the rendered time, blended between its time slots and the
// old and new weather, so they change smoothly instead of in steps
void DistantCoronaParams(float& brightness, float& size)
{
    float time = RenderedHours() + RenderedMinutes() / 60.0f;
    int slot = int(NUM_HOURS) - 1;
    for (int i = 0; i < int(NUM_HOURS); ++i)
    {
        if (time >= TimecycleSlotHours[i])
            slot = i;
    }
    int next = (slot + 1) % NUM_HOURS;
    float start = float(TimecycleSlotHours[slot]);
    float end = next ? float(TimecycleSlotHours[next]) : 24.0f;
    float t = std::clamp((time - start) / (end - start), 0.0f, 1.0f);

    auto weather = [](int32_t* type)
    {
        return type ? std::clamp(*type, 0, int32_t(NUM_WEATHERS) - 1) : 0;
    };
    int oldWeather = weather(CWeather::OldWeatherType);
    int newWeather = CWeather::NewWeatherType ? weather(CWeather::NewWeatherType) : oldWeather;
    float w = CWeather::InterpolationValue ? std::clamp(*CWeather::InterpolationValue, 0.0f, 1.0f) : 0.0f;

    auto at = [&](auto member)
    {
        auto& p = mTimeCycle->mParams;
        float a = std::lerp(p[slot][oldWeather].*member, p[next][oldWeather].*member, t);
        float b = std::lerp(p[slot][newWeather].*member, p[next][newWeather].*member, t);
        return std::lerp(a, b, w);
    };
    brightness = at(&TimeCycleParams::mDistantCoronaBrightness) / 10.0f;
    size = at(&TimeCycleParams::mDistantCoronaSize);
}

void RegisterLODLights()
{
    static auto SolveEqSys = [](float a, float b, float c, float d, float value) -> float
    {
        float determinant = a - c;
        float x = (b - d) / determinant;
        float y = (a * d - b * c) / determinant;
        return std::min(x * value + y, d);
    };

    // Map loading is over once the first frame is drawn
    m_bCatchLamppostsNow = false;

    if (!ProjectLightsDrawn())
        return;

    int currentHour = RenderedHours();
    int curMin = RenderedMinutes();
    unsigned int nTime = currentHour * 60 + curMin;
    unsigned char bAlpha = 0;

    if (nTime >= 19 * 60)
        bAlpha = static_cast<unsigned char>(SolveEqSys(19 * 60.0f, 30.0f, 24 * 60.0f, 255.0f, static_cast<float>(nTime)));
    else if (nTime < 3 * 60)
        bAlpha = 255;
    else
        bAlpha = static_cast<unsigned char>(SolveEqSys(8 * 60.0f, 30.0f, 3 * 60.0f, 255.0f, static_cast<float>(nTime)));

    float fDistantCoronaBrightness, fDistantCoronaSize;
    DistantCoronaParams(fDistantCoronaBrightness, fDistantCoronaSize);

    // The camera only changes while scripts run; the last one is kept while the game is paused
    static CVector camPos;
    static float fCameraFarClip = 0.0f;
    float x, y, z, clip;
    if (Natives::GetRenderedCamera(x, y, z, clip))
    {
        camPos = CVector(x, y, z);
        fCameraFarClip = clip;
    }
    float fCoronaFarClip = *Values.drawDistance > 0.0f ? *Values.drawDistance : fCameraFarClip;
    float fRadiusMultiplier = *Values.radiusMultiplier;
    float fAlphaMultiplier = *Values.alphaMultiplier;
    bool bIncreaseRadius = *Values.increaseRadiusWithDistance != 0;

    for (const auto& lamppost : m_Lampposts)
    {
        if (lamppost.vecPos.z < -15.0f || lamppost.vecPos.z > 1030.0f)
            continue;

        float dx = camPos.x - lamppost.vecPos.x;
        float dy = camPos.y - lamppost.vecPos.y;
        float dz = camPos.z - lamppost.vecPos.z;
        float fDistSqr = dx * dx + dy * dy + dz * dz;

        float fCoronaDist = lamppost.fObjectDrawDistance - 30.0f;

        // Between the object's own lights and the far clip
        if (!lamppost.nNoDistance && (fDistSqr <= fCoronaDist * fCoronaDist || fDistSqr >= fCoronaFarClip * fCoronaFarClip))
            continue;

        float distance = std::sqrt(fDistSqr);

        float fRadius = lamppost.nNoDistance ? 3.5f : SolveEqSys(fCoronaDist, 0.0f, lamppost.fObjectDrawDistance, 3.5f, distance);

        if (bIncreaseRadius)
            fRadius *= std::min(SolveEqSys(fCoronaDist, 1.0f, 9000.0f, 4.0f, distance), 3.0f);

        float fAlphaDistMult = 110.0f - SolveEqSys(fCoronaDist / 4.0f, 10.0f, lamppost.fObjectDrawDistance * 4.0f, 100.0f, distance);

        float baseAlpha = fAlphaMultiplier * ((bAlpha * (lamppost.colour.a / 255.0f)) / fAlphaDistMult);

        // Fade in as the camera moves away, as the radius does
        if (!lamppost.nNoDistance)
            baseAlpha *= SolveEqSys(fCoronaDist, 0.0f, lamppost.fObjectDrawDistance, 1.0f, distance);

        if (lamppost.fCustomSizeMult != 0.45f)
        {
            float finalAlpha = baseAlpha * fDistantCoronaBrightness;
            float finalSize = fRadius * lamppost.fCustomSizeMult * fRadiusMultiplier * fDistantCoronaSize * 1270.5f;

            if (lamppost.nCoronaShowMode)
            {
                float visibility = BlinkVisibility(lamppost.nCoronaShowMode);
                if (visibility <= 0.0f)
                    continue;
                finalAlpha *= visibility;
                finalSize = fRadius * lamppost.fCustomSizeMult * fRadiusMultiplier * 1270.5f;
            }

            DrawCorona(reinterpret_cast<int>(&lamppost), lamppost.colour.r, lamppost.colour.g, lamppost.colour.b, finalAlpha,
                const_cast<CVector*>(&lamppost.vecPos), finalSize, 0.0f, 0.0f, 0, 0.0f, 0, 0, 0);
        }
        else
        {
            // Traffic lights
            fRadius *= 1.3f;
            float finalSize = fRadius * lamppost.fCustomSizeMult * fRadiusMultiplier * 1270.5f;

            bool isYellow = (lamppost.colour.r >= 250 && lamppost.colour.g >= 100 && lamppost.colour.b <= 100);
            bool isRed = (lamppost.colour.r >= 250 && lamppost.colour.g < 100 && lamppost.colour.b == 0);
            bool isGreen = (lamppost.colour.r == 0 && lamppost.colour.g >= 100 && lamppost.colour.b == 0);

            bool isYellowTime = (curMin % 10 == 9);
            bool isRedTime = ((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49));
            bool isGreenTime = ((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59));

            // East-west lights have the opposite phase
            float heading = lamppost.fHeading;
            bool isFacingEastWest = (heading >= -5.0f * float(M_PI) / 6.0f && heading <= -float(M_PI) / 6.0f) ||
                (heading >= float(M_PI) / 6.0f && heading <= 5.0f * float(M_PI) / 6.0f);

            bool redPhase = isRedTime;
            bool greenPhase = isGreenTime;
            if (isFacingEastWest)
                std::swap(redPhase, greenPhase);

            if ((isYellow && isYellowTime) || (isRed && redPhase) || (isGreen && greenPhase))
            {
                DrawCorona(reinterpret_cast<int>(&lamppost), lamppost.colour.r, lamppost.colour.g, lamppost.colour.b, baseAlpha,
                    const_cast<CVector*>(&lamppost.vecPos), finalSize, 0.0f, 0.0f, 0, 0.0f, 0, 0, 0);
            }
        }
    }
}

void Init()
{
    BindSettings();

    if (!FindGameAddresses())
    {
        OutputDebugStringW(L"IVLodLights: this version of the game isn't supported\n");
        return;
    }

    RaiseCoronaLimit();

    // Once per frame, in the game's update loop (a call of an empty function)
    auto pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? ? 74 05 E8 ? ? ? ? 6A 05");
    if (pattern.empty())
        return;
    injector::MakeCALL(pattern.get_first(0), RegisterLODLights, true);

    // The objects of the map as they are loaded
    pattern = hook::pattern("8B 75 08 8D 44 24 1C 50 FF 76 1C C6 44 24");
    if (!pattern.empty())
    {
        struct LoadObjectInstanceHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = *(uintptr_t*)(regs.ebp + 0x8);
                regs.eax = (regs.esp + 0x1C);
                PossiblyAddThisEntity((WplInstance*)regs.esi);
            }
        }; injector::MakeInline<LoadObjectInstanceHook>(pattern.get_first(0), pattern.get_first(7));
    }
    else
    {
        pattern = hook::pattern("8B 5D 08 8B 4B 1C 8D 44 24 14 50 51");
        if (pattern.empty())
            return;
        struct LoadObjectInstanceHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ebx = *(uintptr_t*)(regs.ebp + 0x8);
                regs.ecx = *(uintptr_t*)(regs.ebx + 0x1C);
                PossiblyAddThisEntity((WplInstance*)regs.ebx);
            }
        }; injector::MakeInline<LoadObjectInstanceHook>(pattern.get_first(0), pattern.get_first(6));
    }

    // Reflections of the coronas in water at their full size while the lights are on
    static SafetyHookMid WaterMultiplierHook = {};
    pattern = hook::pattern("F3 0F 59 45 ? F3 0F 11 45 ? 0F 28 85");
    if (!pattern.empty())
    {
        WaterMultiplierHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (*Values.enabled)
                regs.xmm0.f32[0] = 1.0f;
        });
    }
    else
    {
        pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 59 C4 F3 0F 11 45");
        if (!pattern.empty())
        {
            static float* fReflectionMultiplier = *pattern.get_first<float*>(4);
            injector::MakeNOP(pattern.get_first(0), 12, true);
            WaterMultiplierHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                regs.xmm0.f32[0] = *Values.enabled ? 1.0f : *fReflectionMultiplier;
                regs.xmm0.f32[0] *= regs.xmm4.f32[0];
            });
        }
    }

    // The game's own distant lights (its first 8 kinds) are left out while these are drawn
    pattern = hook::pattern("83 F8 08 0F 8C ? ? ? ? 83 3D");
    if (!pattern.empty())
    {
        static uintptr_t loopStart = (uintptr_t)pattern.get_first(9) + *pattern.get_first<int32_t>(5);
        struct DisableDefaultLodLights
        {
            void operator()(injector::reg_pack& regs)
            {
                if (regs.eax < 8 && !(*Values.disableDefaultLodLights && ProjectLightsDrawn()))
                    *(uintptr_t*)(regs.esp - 4) = loopStart;
            }
        }; injector::MakeInline<DisableDefaultLodLights>(pattern.get_first(0), pattern.get_first(9));
    }
    else
    {
        pattern = hook::pattern("0F 8C ? ? ? ? 83 3D ? ? ? ? ? 75 ? F3 0F 10 05");
        if (!pattern.empty())
        {
            static uintptr_t loopStart = (uintptr_t)pattern.get_first(6) + *pattern.get_first<int32_t>(2);
            struct DisableDefaultLodLights
            {
                void operator()(injector::reg_pack& regs)
                {
                    if (regs.ebx < 8 && !(*Values.disableDefaultLodLights && ProjectLightsDrawn()))
                        *(uintptr_t*)(regs.esp - 4) = loopStart;
                }
            }; injector::MakeInline<DisableDefaultLodLights>(pattern.get_first(0), pattern.get_first(6));
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        // The code of older versions can be read only once their executable is unpacked: a thread waits for it. The
        // update loop's call is looked for, since other plugins change the code usually looked for
        #pragma warning(suppress: 4996)
        CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? ? 74 05 E8 ? ? ? ? 6A 05"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent())
            InitializeASI();
    }
    return TRUE;
}
