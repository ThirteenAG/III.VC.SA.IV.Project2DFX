#include "stdafx.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <ranges>

import CoronaLimit;
import LamppostInfo;
import WplInstance;
import TimecycleIV;
import Timer;
import FileMgr;

std::multimap<unsigned int, CLamppostInfo> FileContentMMap;

bool bRenderLodLights;
float fCoronaRadiusMultiplier;
bool bSlightlyIncreaseRadiusWithDistance;
float fCoronaFarClip;

char* CurrentTimeHours;
char* CurrentTimeMinutes;
int(__cdecl* DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
int(__cdecl* DrawCorona2)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
void(__stdcall* GetRootCam)(int* camera);
void(__stdcall* GetGameCam)(int* camera);
bool(__cdecl* CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
void(__cdecl* GetCamPos)(int camera, float* pX, float* pY, float* pZ);
float fCoronaAlphaMultiplier = 1.0f;
float fCamHeight;

unsigned int hashStringLowercaseFromSeed(const char* str, unsigned int seed)
{
    auto hash = seed;
    auto currentChar = str;

    if (*str == '"')
        currentChar = str + 1;

    while (*currentChar)
    {
        char character = *currentChar;

        if (*str == '"' && character == '"')
            break;

        ++currentChar;

        if ((uint8_t)(character - 'A') <= 25)
        {
            character += 32; // Convert uppercase to lowercase
        }
        else if (character == '\\')
        {
            character = '/';
        }

        hash = (1025 * (hash + character) >> 6) ^ 1025 * (hash + character);
    }

    return 32769 * (9 * hash ^ (9 * hash >> 11));
}

void GetMemoryAddresses()
{
    auto pattern = hook::pattern("A3 ? ? ? ? 8B 44 24 0C A3 ? ? ? ? 8B 44 24 10 A3 ? ? ? ? A1 ? ? ? ? A3 ? ? ? ? C3");
    CurrentTimeHours = *pattern.get(0).get<char*>(1);
    CurrentTimeMinutes = *pattern.get(0).get<char*>(10);
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 4D ? F3 0F 10 45");
    DrawCorona = (int(__cdecl*)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("8B 15 ? ? ? ? 56 8D 72 01");
    DrawCorona2 = (int(__cdecl*)(int id, char r, char g, char b, float a5, CVector * pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(pattern.get(0).get<uintptr_t>(0)); //0x7E1970
    pattern = hook::pattern("FF 35 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00");
    GetRootCam = (void(__stdcall*)(int* camera))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00");
    GetGameCam = (void(__stdcall*)(int* camera))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 45 ? F3 0F 11 04 24 F3 0F 10 45 ? F3 0F 11 44 24 ? F3 0F 10 45 ? 51 F3 0F 11 44 24 ? F3 0F 10 45 ? F3 0F 11 04 24");
    CamIsSphereVisible = (bool(__cdecl*)(int camera, float pX, float pY, float pZ, float radius))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 8D 04 24 50 FF 75 08");
    GetCamPos = (void(__cdecl*)(int camera, float* pX, float* pY, float* pZ))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8B 43 20 53");
    CTimer::ms_fTimeStep.SetAddress(*pattern.get_first<float*>(4));
    pattern = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? EB 3A");
    CTimer::m_snTimeInMillisecondsPauseMode.SetAddress(*pattern.get_first<unsigned int*>(1)); //m_snTimeInMilliseconds
    pattern = hook::pattern("A1 ? ? ? ? 83 C4 08 8B CF");
    CWeather::CurrentWeather = *pattern.get_first<CWeather::eWeatherType*>(1);
    pattern = hook::pattern("05 ? ? ? ? 50 8D 4C 24 60");
    mTimeCycle = *pattern.get_first<Timecycle*>(1);
}

void RegisterCustomCoronas()
{
    constexpr unsigned int nModelID = 0xFFFFFFFF;

    auto foundElements = FileContentMMap | std::views::filter([&nModelID](auto& v)
    {
        return v.first == nModelID;
    });

    for (auto& it : foundElements)
    {
        m_Lampposts.push_back(CLamppostInfo(it.second.vecLocalPos, { 0.0f, 0.0f, 0.0f }, it.second.colour, it.second.fCustomSizeMult, it.second.nCoronaShowMode, it.second.nNoDistance, it.second.nDrawSearchlight, 0.0f));
    }
}

void RegisterLamppost(WplInstance* pObj)
{
    DWORD nModelID = pObj->ModelNameHash;
    CMatrix             dummyMatrix;

    float qw = pObj->RotationW;
    float qx = pObj->RotationX;
    float qy = pObj->RotationY;
    float qz = pObj->RotationZ;

    float n = 1.0f / sqrt(qx * qx + qy * qy + qz * qz + qw * qw);
    qx *= n;
    qy *= n;
    qz *= n;
    qw *= n;

    dummyMatrix.rx = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
    dummyMatrix.ry = 2.0f * qx * qy - 2.0f * qz * qw;
    dummyMatrix.rz = 2.0f * qx * qz + 2.0f * qy * qw;

    dummyMatrix.ux = 2.0f * qx * qy + 2.0f * qz * qw;
    dummyMatrix.uy = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
    dummyMatrix.uz = 2.0f * qy * qz - 2.0f * qx * qw;

    dummyMatrix.fx = 2.0f * qx * qz - 2.0f * qy * qw;
    dummyMatrix.fy = 2.0f * qy * qz + 2.0f * qx * qw;
    dummyMatrix.fz = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;

    dummyMatrix.px = pObj->PositionX;
    dummyMatrix.py = pObj->PositionY;
    dummyMatrix.pz = pObj->PositionZ;

    {
        float dx = pObj->PositionX - -278.37f;
        float dy = pObj->PositionY - -1377.48f;
        float dz = pObj->PositionZ - 90.98f;
        if ((dx * dx + dy * dy + dz * dz) <= (300.0f * 300.0f))
            return;
    }

    auto foundElements = FileContentMMap | std::views::filter([&nModelID](auto& v)
    {
        return v.first == nModelID;
    });

    float heading = atan2(dummyMatrix.GetUp().y, -dummyMatrix.GetUp().x);
    for (auto& it : foundElements)
    {
        m_Lampposts.push_back(CLamppostInfo(dummyMatrix * it.second.vecLocalPos, it.second.vecLocalPos, it.second.colour, it.second.fCustomSizeMult, it.second.nCoronaShowMode, it.second.nNoDistance, it.second.nDrawSearchlight, heading, it.second.fObjectDrawDistance));
    }
}

WplInstance* PossiblyAddThisEntity(WplInstance* pInstance)
{
    if (m_bCatchLamppostsNow && FileContentMMap.contains(pInstance->ModelNameHash))
        RegisterLamppost(pInstance);

    return pInstance;
}

void LoadDatFileIV()
{
    CIniReader iniReader("");
    auto DataFilePath = iniReader.GetIniPath();
    DataFilePath.replace_extension(".dat");

    if (FILE* hFile = CFileMgr::OpenFile(DataFilePath.string().c_str(), "r"))
    {
        unsigned int nModelIV = 0xFFFFFFFF;

        while (const char* pLine = CFileMgr::LoadLine(hFile))
        {
            if (pLine[0] && pLine[0] != '#')
            {
                if (pLine[0] == '%')
                {
                    if (strcmp(pLine, "%additional_coronas") != 0)
                        nModelIV = hashStringLowercaseFromSeed((char*)(pLine + 1), 0);
                    else
                        nModelIV = 0xFFFFFFFF;
                }
                else
                {
                    float			fOffsetX, fOffsetY, fOffsetZ;
                    unsigned int	nRed, nGreen, nBlue, nAlpha;
                    float			fCustomSize = 1.0f;
                    float			fDrawDistance = 0.0f;
                    int				nNoDistance = 0;
                    int				nDrawSearchlight = 0;
                    int				nCoronaShowMode = 0;
                    if (sscanf(pLine, "%3d %3d %3d %3d %f %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &fDrawDistance, &nCoronaShowMode, &nNoDistance, &nDrawSearchlight) != 12)
                        sscanf(pLine, "%3d %3d %3d %3d %f %f %f %f %2d %1d %1d", &nRed, &nGreen, &nBlue, &nAlpha, &fOffsetX, &fOffsetY, &fOffsetZ, &fCustomSize, &nCoronaShowMode, &nNoDistance, &nDrawSearchlight);
                    FileContentMMap.insert(std::make_pair(nModelIV, CLamppostInfo(CVector(0.0f, 0.0f, 0.0f), CVector(fOffsetX, fOffsetY, fOffsetZ), CRGBA(static_cast<unsigned char>(nRed), static_cast<unsigned char>(nGreen), static_cast<unsigned char>(nBlue), static_cast<unsigned char>(nAlpha)), fCustomSize, nCoronaShowMode, nNoDistance, nDrawSearchlight, 0.0f, fDrawDistance)));
                }
            }
        }

        m_bCatchLamppostsNow = true;
        CFileMgr::CloseFile(hFile);
    }
    else
    {
        bRenderLodLights = 0;
    }
}

void RegisterLODLights()
{
    static auto SolveEqSys = [](float a, float b, float c, float d, float value) -> float
    {
        float determinant = a - c;
        float x = (b - d) / determinant;
        float y = (a * d - b * c) / determinant;
        return min(x * value + y, d);
    };

    if (m_bCatchLamppostsNow)
        m_bCatchLamppostsNow = false;

    static auto ff = GetModuleHandleW(L"GTAIV.EFLC.FusionFix.asi");
    if (ff)
    {
        static auto GetDistantLightsPrefValue = (int32_t(*)())GetProcAddress(ff, "GetDistantLightsPrefValue");
        if (GetDistantLightsPrefValue)
        {
            if (!GetDistantLightsPrefValue())
                return;
        }
    }

    int currentHour = *CurrentTimeHours;
    if (currentHour < 19 && currentHour > 7)
        return;

    // Calculate alpha based on time
    unsigned int nTime = currentHour * 60 + *CurrentTimeMinutes;
    unsigned int curMin = *CurrentTimeMinutes;
    unsigned char bAlpha = 0;

    if (nTime >= 19 * 60)
        bAlpha = static_cast<unsigned char>(SolveEqSys(19 * 60.0f, 30.0f, 24 * 60.0f, 255.0f, static_cast<float>(nTime)));
    else if (nTime < 3 * 60)
        bAlpha = 255;
    else
        bAlpha = static_cast<unsigned char>(SolveEqSys(8 * 60.0f, 30.0f, 3 * 60.0f, 255.0f, static_cast<float>(nTime)));

    // Get time cycle parameters
    auto timeIndex = Timecycle::GameTimeToTimecycTimeIndex(currentHour);
    auto& timeCycleParams = mTimeCycle->mParams[timeIndex][*CWeather::CurrentWeather];
    float fDistantCoronaBrightness = timeCycleParams.mDistantCoronaBrightness / 10.0f;
    float fDistantCoronaSize = timeCycleParams.mDistantCoronaSize;

    // Get camera info once
    int currentCamera;
    GetRootCam(&currentCamera);
    CVector camPos;
    GetCamPos(currentCamera, &camPos.x, &camPos.y, &camPos.z);

    for (const auto& lamppost : m_Lampposts)
    {
        // Check height bounds
        if (lamppost.vecPos.z < -15.0f || lamppost.vecPos.z > 1030.0f)
            continue;

        // Calculate distance
        float dx = camPos.x - lamppost.vecPos.x;
        float dy = camPos.y - lamppost.vecPos.y;
        float dz = camPos.z - lamppost.vecPos.z;
        float fDistSqr = dx * dx + dy * dy + dz * dz;
        float distance = std::sqrt(fDistSqr);

        float fCoronaDist = lamppost.fObjectDrawDistance - 30.0f;

        // Check if within corona range
        if (!lamppost.nNoDistance &&
            (fDistSqr <= fCoronaDist * fCoronaDist || fDistSqr >= fCoronaFarClip * fCoronaFarClip))
            continue;

        // Calculate radius
        float fRadius = lamppost.nNoDistance ? 3.5f :
            SolveEqSys(fCoronaDist, 0.0f, lamppost.fObjectDrawDistance, 3.5f, distance);

        if (bSlightlyIncreaseRadiusWithDistance)
            fRadius *= min(SolveEqSys(fCoronaDist, 1.0f, 9000.0, 4.0f, distance), 3.0f);

        float fAlphaDistMult = 110.0f - SolveEqSys(fCoronaDist / 4.0f, 10.0f, lamppost.fObjectDrawDistance * 4.0f, 100.0f, distance);

        // Calculate base alpha
        float baseAlpha = fCoronaAlphaMultiplier * ((bAlpha * (lamppost.colour.a / 255.0f)) / fAlphaDistMult);

        if (!lamppost.nNoDistance)
        {
            // Fade in alpha as camera moves away, similar to how radius is handled
            float alphaFade = SolveEqSys(fCoronaDist, 0.0f, lamppost.fObjectDrawDistance, 1.0f, distance);
            baseAlpha *= alphaFade;
        }

        if (lamppost.fCustomSizeMult != 0.45f)
        {
            // Regular lamppost
            float finalAlpha = baseAlpha * fDistantCoronaBrightness;
            float finalSize = fRadius * lamppost.fCustomSizeMult * fCoronaRadiusMultiplier * fDistantCoronaSize * 1270.5f;

            if (lamppost.nCoronaShowMode)
            {
                // Blinking light
                static float blinking = 1.0f;
                if (IsBlinkingNeeded(lamppost.nCoronaShowMode))
                    blinking -= *CTimer::ms_fTimeStep / 1000.0f;
                else
                    blinking += *CTimer::ms_fTimeStep / 1000.0f;

                blinking = std::clamp(blinking, 0.0f, 1.0f);
                finalAlpha *= blinking;
                finalSize = fRadius * lamppost.fCustomSizeMult * fCoronaRadiusMultiplier * 1270.5f;
            }

            DrawCorona2(reinterpret_cast<unsigned int>(&lamppost),
                lamppost.colour.r, lamppost.colour.g, lamppost.colour.b,
                finalAlpha,
                const_cast<CVector*>(&lamppost.vecPos),
                finalSize,
                0.0, 0.0, 0, 0.0, 0, 0, 0);
        }
        else
        {
            fRadius *= 1.3f;
            float finalSize = fRadius * lamppost.fCustomSizeMult * fCoronaRadiusMultiplier * 1270.5f;

            // Color detection
            bool isYellow = (lamppost.colour.r >= 250 && lamppost.colour.g >= 100 && lamppost.colour.b <= 100);
            bool isRed = (lamppost.colour.r >= 250 && lamppost.colour.g < 100 && lamppost.colour.b == 0);
            bool isGreen = (lamppost.colour.r == 0 && lamppost.colour.g >= 100 && lamppost.colour.b == 0);
            bool isGreenAlt = (lamppost.colour.r == 0 && lamppost.colour.g >= 250 && lamppost.colour.b == 0);

            // Time periods
            bool isYellowTime = (curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59);
            bool isRedTime = ((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49));
            bool isGreenTime = ((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59));

            // Determine orientation based on heading
            float heading = lamppost.fHeading;
            bool isFacingEastWest = (heading >= -5.0f * M_PI / 6.0f && heading <= -M_PI / 6.0f) ||
                (heading >= M_PI / 6.0f && heading <= 5.0f * M_PI / 6.0f);

            // Swap phases for E/W traffic lights
            bool redPhase = isRedTime;
            bool greenPhase = isGreenTime;
            if (isFacingEastWest)
            {
                std::swap(redPhase, greenPhase);
            }

            bool shouldDraw = (isYellow && isYellowTime) ||
                (isRed && redPhase) ||
                ((isGreen || isGreenAlt) && greenPhase);

            if (shouldDraw)
            {
                DrawCorona2(reinterpret_cast<unsigned int>(&lamppost),
                    lamppost.colour.r, lamppost.colour.g, lamppost.colour.b,
                    baseAlpha,
                    const_cast<CVector*>(&lamppost.vecPos),
                    finalSize,
                    0.0, 0.0, 0, 0.0, 0, 0, 0);
            }
        }
    }
}

void Init()
{
    CIniReader iniReader("");
    bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
    fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
    fCoronaAlphaMultiplier = iniReader.ReadFloat("LodLights", "CoronaAlphaMultiplier", 1.0f);
    bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
    fCoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
    bool DisableDefaultLodLights = iniReader.ReadInteger("LodLights", "DisableDefaultLodLights", 1) != 0;
    int32_t DisableCoronasWaterReflection = iniReader.ReadInteger("LodLights", "DisableCoronasWaterReflection", 0);

    struct LoadObjectInstanceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *(uintptr_t*)(regs.ebp + 0x8);
            regs.eax = (regs.esp + 0x1C);

            PossiblyAddThisEntity((WplInstance*)regs.esi);
        }
    };

    if (bRenderLodLights)
    {
        GetMemoryAddresses();
        IncreaseCoronaLimit();
        LoadDatFileIV();
        RegisterCustomCoronas();

        auto pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? ? 74 05 E8 ? ? ? ? 6A 05"); //+
        injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), RegisterLODLights, true);
        pattern = hook::pattern("8B 75 08 8D 44 24 1C 50 FF 76 1C C6 44 24"); //+
        injector::MakeInline<LoadObjectInstanceHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7));
    }

    if (DisableDefaultLodLights)
    {
        auto pattern = hook::pattern("83 F8 08 0F 8C ? ? ? ? 83 3D");
        injector::WriteMemory<uint8_t>(pattern.get_first(2), 0, true);
    }

    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 80 3D ? ? ? ? ? 74 32 6A 00 6A 0C");
    static auto jmp = pattern.get(0).get<uintptr_t>(8);
    if (DisableCoronasWaterReflection == 1)
    {
        injector::MakeNOP(pattern.get(0).get<uintptr_t>(0), 5, true);
    }
    else
    {
        if (DisableCoronasWaterReflection == 2)
        {
            struct ReflectionsHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    regs.edx = *(uintptr_t*)(regs.esi + 0x938);
                    if (fCamHeight < 100.0f)
                    {
                        *(uintptr_t*)(regs.esp - 4) = (uintptr_t)jmp;
                    }
                }
            }; injector::MakeInline<ReflectionsHook>(pattern.get(0).get<uintptr_t>(-6), pattern.get(0).get<uintptr_t>(0));
            injector::WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(-1), 0x52, true); // push edx
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("F3 0F 10 44 24 ? F3 0F 59 05 ? ? ? ? EB ? E8"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
