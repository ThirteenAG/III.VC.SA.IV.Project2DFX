#include "stdafx.h"
#include "CLODLightManager.h"
#include "Hooking.Patterns.h"
#include <ranges>

using namespace injector;

#define NewLimitExponent 14

char* CLODLightManager::IV::CurrentTimeHours;
char* CLODLightManager::IV::CurrentTimeMinutes;
int(__cdecl *CLODLightManager::IV::DrawCorona)(float x, float y, float z, float radius, unsigned int unk, float unk2, unsigned char r, unsigned char g, unsigned char b);
int(__cdecl *CLODLightManager::IV::DrawCorona2)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
int(__cdecl *CLODLightManager::IV::DrawCorona3)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14);
void(__stdcall *CLODLightManager::IV::GetRootCam)(int *camera);
void(__stdcall *CLODLightManager::IV::GetGameCam)(int *camera);
bool(__cdecl *CLODLightManager::IV::CamIsSphereVisible)(int camera, float pX, float pY, float pZ, float radius);
void(__cdecl *CLODLightManager::IV::GetCamPos)(int camera, float *pX, float *pY, float *pZ);
int* CTimer::m_snTimeInMillisecondsPauseMode = nullptr;
float* CTimer::ms_fTimeStep = nullptr;
float fCamHeight;
std::map<unsigned int, CRGBA> FestiveLights;
extern bool bIsIVEFLC;

int CCoronasRegisterFestiveCoronaForEntity(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14)
{
    auto it = FestiveLights.find(id);
    if (it != FestiveLights.end())
    {
        return CLODLightManager::IV::DrawCorona2(id, it->second.r, it->second.g, it->second.b, a5, pos, radius, a8, a9, a10, a11, a12, a13, a14);
    }
    else
    {
        FestiveLights[id] = CRGBA(random(0, 255), random(0, 255), random(0, 255), 0);
        return CLODLightManager::IV::DrawCorona2(id, r, g, b, a5, pos, radius, a8, a9, a10, a11, a12, a13, a14);
    }
}

void CLODLightManager::IV::Init()
{
    bIsIVEFLC = true;
    CIniReader iniReader("");
    bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
    fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
    bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
    fCoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
    bool DisableDefaultLodLights = iniReader.ReadInteger("LodLights", "DisableDefaultLodLights", 0) == 1;
    int32_t DisableCoronasWaterReflection = iniReader.ReadInteger("LodLights", "DisableCoronasWaterReflection", 0);
    fGenericObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "LamppostsDrawDistance", 0.0f);
    bFestiveLights = iniReader.ReadInteger("Misc", "FestiveLights", 1) != 0;
    bFestiveLightsAlways = iniReader.ReadInteger("Misc", "FestiveLightsAlways", 0) != 0;

    struct LoadObjectInstanceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *(uintptr_t*)(regs.ebp + 0x8);
            regs.eax = (regs.esp + 0x1C);

            PossiblyAddThisEntity((WplInstance*)regs.esi);
        }
    };

    struct ParseIdeObjsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto xmmZero = *(float*)(regs.esp + 0x1C);
            if (xmmZero == 50.0f || xmmZero == 100.0f || xmmZero == 150.0f || xmmZero == 80.0f) //lampposts draw distances
            {
                xmmZero = fGenericObjectsDrawDistance;
            }
            _asm movss   xmm0, xmmZero
        }
    };

    if (bRenderLodLights)
    {
        GetMemoryAddresses();
        IncreaseCoronaLimit();
        LoadDatFile();
        RegisterCustomCoronas();

        auto pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? ? 74 05 E8 ? ? ? ? 6A 05"); //+
        injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), RegisterLODLights, true);
        pattern = hook::pattern("8B 75 08 8D 44 24 1C 50 FF 76 1C C6 44 24"); //+
        injector::MakeInline<LoadObjectInstanceHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(7));
        //pattern = hook::pattern(""); //causes crash
        //injector::MakeInline<ParseIdeObjsHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6)); //0x008D2311 - 0x8D2317
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

    if (bFestiveLights)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm *date = std::localtime(&now_c);
        if (bFestiveLightsAlways || (date->tm_mon == 0 && date->tm_mday <= 1) || (date->tm_mon == 11 && date->tm_mday >= 31))
        {
            DrawCorona3 = &CCoronasRegisterFestiveCoronaForEntity;
            pattern = hook::pattern("6A 00 51 E8 ? ? ? ? 83 C4 3C E9");
            injector::MakeCALL(pattern.get_first(3), CCoronasRegisterFestiveCoronaForEntity, true);
            pattern = hook::pattern("51 52 E8 ? ? ? ? 83 C4 3C EB 03");
            injector::MakeCALL(pattern.get_first(2), CCoronasRegisterFestiveCoronaForEntity, true);
        }
    }
}

void CLODLightManager::IV::GetMemoryAddresses()
{
    auto pattern = hook::pattern("A3 ? ? ? ? 8B 44 24 0C A3 ? ? ? ? 8B 44 24 10 A3 ? ? ? ? A1 ? ? ? ? A3 ? ? ? ? C3"); //+
    CLODLightManager::IV::CurrentTimeHours = *pattern.get(0).get<char*>(1);
    CLODLightManager::IV::CurrentTimeMinutes = *pattern.get(0).get<char*>(10);
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 4D ? F3 0F 10 45"); //+
    CLODLightManager::IV::DrawCorona = (int(__cdecl *)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("8B 15 ? ? ? ? 56 8D 72 01"); //+
    CLODLightManager::IV::DrawCorona2 = (int(__cdecl *)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(pattern.get(0).get<uintptr_t>(0)); //0x7E1970
    CLODLightManager::IV::DrawCorona3 = CLODLightManager::IV::DrawCorona2;
    pattern = hook::pattern("FF 35 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00"); //+
    CLODLightManager::IV::GetRootCam = (void(__stdcall *)(int *camera))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00"); //+
    CLODLightManager::IV::GetGameCam = (void(__stdcall *)(int *camera))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 45 ? F3 0F 11 04 24 F3 0F 10 45 ? F3 0F 11 44 24 ? F3 0F 10 45 ? 51 F3 0F 11 44 24 ? F3 0F 10 45 ? F3 0F 11 04 24");
    CLODLightManager::IV::CamIsSphereVisible = (bool(__cdecl *)(int camera, float pX, float pY, float pZ, float radius))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 8D 04 24 50 FF 75 08"); //+
    CLODLightManager::IV::GetCamPos = (void(__cdecl *)(int camera, float *pX, float *pY, float *pZ))(pattern.get(0).get<uintptr_t>(0));
    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 59 05 ? ? ? ? 8B 43 20 53");
    CTimer::ms_fTimeStep = *pattern.get_first<float*>(4);
    pattern = hook::pattern("A1 ? ? ? ? A3 ? ? ? ? EB 3A");
    CTimer::m_snTimeInMillisecondsPauseMode = *pattern.get_first<int32_t*>(1); //m_snTimeInMilliseconds
}

void CLODLightManager::IV::IncreaseCoronaLimit()
{
    auto nCoronasLimit = static_cast<uint32_t>(3 * pow(2.0, NewLimitExponent)); // 49152, default 3 * pow(2, 8) = 768

    static std::vector<uint32_t> aCoronas;
    static std::vector<uint32_t> aCoronas2;
    aCoronas.resize(nCoronasLimit * 0x3C * 4);
    aCoronas2.resize(nCoronasLimit * 0x3C * 4);

    int32_t counter1 = 0;
    int32_t counter2 = 0;

    uintptr_t range_start = (uintptr_t)hook::get_pattern("33 C0 C7 80 ? ? ? ? ? ? ? ? 83 C0 40 3D ? ? ? ? 72 EC C7 05 ? ? ? ? ? ? ? ? C3"); //+
    uintptr_t range_end = (uintptr_t)hook::get_pattern("5E C3 FF 05 ? ? ? ? 5E C3"); //+

    uintptr_t dword_temp = (uintptr_t)*hook::pattern("89 82 ? ? ? ? F3 0F 11 82 ? ? ? ? F3 0F 10 44 24 ? F3 0F 11 8A ? ? ? ? 8B 41 0C 0F B6 4C 24 ? 89 82").get(0).get<uint32_t*>(2); //+

    for (size_t i = dword_temp; i <= (dword_temp + 0x3C); i++)
    {
        auto GoThroughPatterns = [&](const char* pattern_str, int32_t pos) -> void
        {
            auto patternl = hook::range_pattern(range_start, range_end, pattern_str);
            for (size_t j = 0; j < patternl.size(); j++)
            {
                if (*patternl.get(j).get<uintptr_t>(pos) == i)
                {
                    AdjustPointer(patternl.get(j).get<uint32_t>(pos), &aCoronas[0], dword_temp, dword_temp + 0x3C);
                    counter1++;
                }
            }
        };

        GoThroughPatterns("83 8A", 2);
        GoThroughPatterns("88 82", 2);
        GoThroughPatterns("89 82", 2);
        GoThroughPatterns("89 82", 2);
        GoThroughPatterns("89 8A", 2);
        GoThroughPatterns("8B 82", 2);
        GoThroughPatterns("BE", 1);
        GoThroughPatterns("C7 80", 2);
        GoThroughPatterns("C7 82", 2);
        GoThroughPatterns("F3 0F 11 82", 4);
        GoThroughPatterns("F3 0F 11 8A", 4);
    }


    range_start = (uintptr_t)hook::get_pattern("8B 44 24 1C F3 0F 10 4C 24 ? F3 0F 10 46 ? F3 0F 59 05 ? ? ? ? 8D 0C 40"); //+
    range_end = (uintptr_t)hook::get_pattern("0F 29 85 ? ? ? ? 0F 28 9D ? ? ? ? F3 0F 5C C2 F3 0F 11 5C 24 ? 0F 28 9D ? ? ? ? F3 0F 11 5C 24 ? 0F 28 D0"); //+

    dword_temp = (uintptr_t)*hook::pattern("F3 0F 11 89 ? ? ? ? F3 0F 10 4C 24 ? F3 0F 11 89").get(0).get<uint32_t*>(4);

    for (size_t i = dword_temp; i <= (dword_temp + 0x1B); i++)
    {
        auto GoThroughPatterns = [&](const char* pattern_str, int32_t pos) -> void
        {
            auto patternl = hook::range_pattern(range_start, range_end, pattern_str);
            for (size_t j = 0; j < patternl.size(); j++)
            {
                if (*patternl.get(j).get<uintptr_t>(pos) == i)
                {
                    AdjustPointer(patternl.get(j).get<uint32_t>(pos), &aCoronas2[0], dword_temp, dword_temp + 0x1B);
                    counter2++;
                }
            }
        };

        GoThroughPatterns("0F 28 81", 3);
        GoThroughPatterns("0F B6 81", 3);
        GoThroughPatterns("0F B6 B1", 3);
        GoThroughPatterns("80 B8", 2);
        GoThroughPatterns("88 81", 2);
        GoThroughPatterns("88 91", 2);
        GoThroughPatterns("89 81", 2);
        GoThroughPatterns("BE", 1);
        GoThroughPatterns("F3 0F 10 81", 4);
        GoThroughPatterns("F3 0F 10 89", 4);
        GoThroughPatterns("F3 0F 11 81", 4);
        GoThroughPatterns("F3 0F 11 89", 4);
        GoThroughPatterns("F3 0F 11 91", 4);
    }

    if (counter1 != 24 || counter2 != 18)
        MessageBox(0, L"IV.Project2DFX", L"Project2DFX is not fully compatible with this version of the game", 0);

    auto p = hook::pattern("BF FF 02 00 00"); //+
    AdjustPointer(p.get_first(-4), &aCoronas[0], dword_temp, dword_temp + 0x3C);
    p = hook::pattern("BF FF 05 00 00"); //+
    AdjustPointer(p.get_first(-4), &aCoronas2[0], dword_temp, dword_temp + 0x1B);

    auto pattern = hook::pattern("C1 E1 ? 03 4C 24 18 C1");
    WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true);
    pattern = hook::pattern("C1 E1 ? 03 CF C1 E1");
    WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true);
    pattern = hook::pattern("C1 E0 ? 03 C2 C1 E0 05 80 B8");
    WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true);

    pattern = hook::pattern("81 FE ? ? ? ? 0F 8D ? ? ? ? 8B 44 24 08 8B 4C 24 1C F3 0F 10 44 24 ? C1 E2 06");
    WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(2), nCoronasLimit, true);
    pattern = hook::pattern("3D ? ? ? ? 0F 8D ? ? ? ? 8B 44 24 1C F3 0F 10 4C 24");
    WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(1), nCoronasLimit, true);
    pattern = hook::pattern("3D ? ? ? ? 72 EC C7 05 ? ? ? ? ? ? ? ? C3");
    WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(1), nCoronasLimit * 64, true);
}

void CLODLightManager::IV::RegisterCustomCoronas()
{
    constexpr unsigned int nModelID = 0xFFFFFFFF;

    auto foundElements = *pFileContentMMap | std::views::filter([&nModelID](auto& v) {
        return v.first == nModelID;
    });

    for (auto& it : foundElements)
    {
        m_Lampposts.push_back(CLamppostInfo(it.second.vecPos, it.second.colour, it.second.fCustomSizeMult, it.second.nCoronaShowMode, it.second.nNoDistance, it.second.nDrawSearchlight, 0.0f));
    }
}

WplInstance* CLODLightManager::IV::PossiblyAddThisEntity(WplInstance* pInstance)
{
    if (m_bCatchLamppostsNow && pFileContentMMap->contains(pInstance->ModelNameHash))
        RegisterLamppost(pInstance);

    return pInstance;
}

void CLODLightManager::IV::RegisterLamppost(WplInstance* pObj)
{
    //DWORD nModelID = pObj->ModelNameHash;
    //RwV3d trans = { pObj->PositionX, pObj->PositionY, pObj->PositionZ };
    //RwV3d axis = { pObj->RotationW, pObj->RotationX, pObj->RotationY };
    //
    //CMatrix dummyMatrix;
    //auto angle2 = -RADTODEG(2.0f * acos(pObj->RotationZ));
    //makeRotation(&dummyMatrix, &axis, angle2, &trans);

    DWORD               nModelID = pObj->ModelNameHash;
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

    dummyMatrix.matrix.right.x = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
    dummyMatrix.matrix.right.y = 2.0f * qx * qy - 2.0f * qz * qw;
    dummyMatrix.matrix.right.z = 2.0f * qx * qz + 2.0f * qy * qw;

    dummyMatrix.matrix.up.x = 2.0f * qx * qy + 2.0f * qz * qw;
    dummyMatrix.matrix.up.y = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
    dummyMatrix.matrix.up.z = 2.0f * qy * qz - 2.0f * qx * qw;

    dummyMatrix.matrix.at.x = 2.0f * qx * qz - 2.0f * qy * qw;
    dummyMatrix.matrix.at.y = 2.0f * qy * qz + 2.0f * qx * qw;
    dummyMatrix.matrix.at.z = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;

    dummyMatrix.matrix.pos.x = pObj->PositionX;
    dummyMatrix.matrix.pos.y = pObj->PositionY;
    dummyMatrix.matrix.pos.z = pObj->PositionZ;

    {
        auto v1 = CVector(pObj->PositionX, pObj->PositionY, pObj->PositionZ);
        auto v2 = CVector(-278.37f, -1377.48f, 90.98f);
        if (GetDistance((RwV3d*)&v1, (RwV3d*)&v2) <= 300.0f)
            return;
    }

    auto foundElements = *pFileContentMMap | std::views::filter([&nModelID](auto& v) {
        return v.first == nModelID;
    });

    for (auto& it : foundElements)
    {
        m_Lampposts.push_back(CLamppostInfo(dummyMatrix * it.second.vecPos, it.second.colour, it.second.fCustomSizeMult, it.second.nCoronaShowMode, it.second.nNoDistance, it.second.nDrawSearchlight, atan2(dummyMatrix.GetUp()->y, -dummyMatrix.GetUp()->x), it.second.fObjectDrawDistance));
    }
}

void CLODLightManager::IV::RegisterLODLights()
{
    static auto SolveEqSys = [](float a, float b, float c, float d, float value) -> float
    {
        float determinant = a - c;
        float x = (b - d) / determinant;
        float y = (a * d - b * c) / determinant;
        return min((x)*value + y, d);
    };

    if (m_bCatchLamppostsNow)
        m_bCatchLamppostsNow = false;

    if (*CurrentTimeHours >= 19 || *CurrentTimeHours <= 7)
    {
        unsigned char   bAlpha = 0;
        float           fRadius = 0.0f;
        unsigned int    nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
        unsigned int    curMin = *CurrentTimeMinutes;

        if (nTime >= 19 * 60)
            bAlpha = static_cast<unsigned char>(SolveEqSys((float)(19 * 60), 30.0f, (float)(24 * 60), 255.0f, (float)nTime)); // http://goo.gl/O03RpE {(19*60)a + y = 30,  (24*60)a + y = 255}
        else if (nTime < 3 * 60)
            bAlpha = 255;
        else
            bAlpha = static_cast<unsigned char>(SolveEqSys((float)(8 * 60), 30.0f, (float)(3 * 60), 255.0f, (float)nTime)); // http://goo.gl/M8Dev9 {(7*60)a + y = 30,  (3*60)a + y = 255}

        for (auto& it : m_Lampposts)
        {
            static int currentCamera;
            GetRootCam(&currentCamera);
            if ((it.vecPos.z >= -15.0f) && (it.vecPos.z <= 1030.0f) /*&& CamIsSphereVisible(currentCamera, it.vecPos.x, it.vecPos.y, it.vecPos.z, 3.0f)*/)
            {
                CVector CamPos = CVector();
                GetCamPos(currentCamera, &CamPos.x, &CamPos.y, &CamPos.z);
                CVector*    pCamPos = &CamPos;
                float       fDistSqr = (pCamPos->x - it.vecPos.x)*(pCamPos->x - it.vecPos.x) + (pCamPos->y - it.vecPos.y)*(pCamPos->y - it.vecPos.y) + (pCamPos->z - it.vecPos.z)*(pCamPos->z - it.vecPos.z);
                fCamHeight = CamPos.z;
                float fCoronaDist = it.fObjectDrawDistance - 30.0f;

                if ((fDistSqr > fCoronaDist * fCoronaDist && fDistSqr < fCoronaFarClip * fCoronaFarClip) || it.nNoDistance)
                {
                    float min_radius_distance = fCoronaDist;
                    float min_radius_value = 0.0f;
                    float max_radius_distance = it.fObjectDrawDistance;
                    float max_radius_value = 3.5f;

                    if (it.nNoDistance)
                        fRadius = max_radius_value;
                    else
                        fRadius = SolveEqSys(min_radius_distance, min_radius_value, max_radius_distance, max_radius_value, sqrt(fDistSqr)); // http://goo.gl/vhAZSx

                    if (bSlightlyIncreaseRadiusWithDistance)
                        fRadius *= min(SolveEqSys(min_radius_distance, 1.0f, fCoronaFarClip, 4.0f, sqrt(fDistSqr)), 3.0f); // http://goo.gl/3kDpnC {(300)a + y = 1.0,  (2500)a + y = 4}

                    float fAlphaDistMult = 110.0 - SolveEqSys(min_radius_distance / 4.0f, 10.0f, max_radius_distance * 4.0f, 100.0f, sqrt(fDistSqr));

                    if (it.fCustomSizeMult != 0.45f)
                    {
                        if (!it.nCoronaShowMode)
                        {
                            //DrawCorona(it.vecPos.x, it.vecPos.y, it.vecPos.z, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.r, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.g, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.b);
                            DrawCorona3(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                        }
                        else
                        {
                          static float blinking = 1.0f;
                          if (IsBlinkingNeeded(it.nCoronaShowMode))
                              blinking -= *CTimer::ms_fTimeStep / 1000.0f;
                          else
                              blinking += *CTimer::ms_fTimeStep / 1000.0f;
                        
                          (blinking > 1.0f) ? blinking = 1.0f : (blinking < 0.0f) ? blinking = 0.0f : 0.0f;
                        
                          DrawCorona3(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, blinking * (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                        }
                    }
                    else
                    {
                        fRadius *= 1.3f;
                        if ((it.colour.r >= 250 && it.colour.g >= 100 && it.colour.b <= 100) && ((curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59))) //yellow
                        {
                            //DrawCorona(it.vecPos.x, it.vecPos.y, it.vecPos.z, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.r, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.g, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.b);
                            DrawCorona2(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                        }
                        else
                        {
                            if ((abs(it.fHeading) >= (3.1415f / 6.0f) && abs(it.fHeading) <= (5.0f * 3.1415f / 6.0f)))
                            {
                                if ((it.colour.r >= 250 && it.colour.g < 100 && it.colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    //DrawCorona(it.vecPos.x, it.vecPos.y, it.vecPos.z, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.r, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.g, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.b);
                                    DrawCorona2(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                }
                                else
                                {
                                    if ((it.colour.r == 0 && it.colour.g >= 100 && it.colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        //DrawCorona(it.vecPos.x, it.vecPos.y, it.vecPos.z, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.r, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.g, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.b);
                                        DrawCorona2(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                    }
                                }
                            }
                            else
                            {
                                if ((it.colour.r == 0 && it.colour.g >= 250 && it.colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    //DrawCorona(it.vecPos.x, it.vecPos.y, it.vecPos.z, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.r, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.g, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.b);
                                    DrawCorona2(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                }
                                else
                                {
                                    if ((it.colour.r >= 250 && it.colour.g < 100 && it.colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        //DrawCorona(it.vecPos.x, it.vecPos.y, it.vecPos.z, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.r, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.g, ((bAlpha * (it.colour.a / 255.0f)) / 500.0f) * it.colour.b);
                                        DrawCorona2(reinterpret_cast<unsigned int>(&it), it.colour.r, it.colour.g, it.colour.b, (bAlpha * (it.colour.a / 255.0f)) / fAlphaDistMult, (CVector*)&it.vecPos, (fRadius * it.fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        CLODLightManager::IV::Init();
    });
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}