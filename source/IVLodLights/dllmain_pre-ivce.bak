#include "stdafx.h"
#include "CLODLightManager.h"
#include "Hooking.Patterns.h"

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
int& CTimer::m_snTimeInMillisecondsPauseMode = *(int*)0xBADDEAD;
extern bool bIsIVEFLC;
float fCamHeight;
std::map<unsigned int, CRGBA> FestiveLights;

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
    bFestiveLightsAlways = iniReader.ReadInteger("Misc", "bFestiveLightsAlways", 0) != 0;

    struct LoadObjectInstanceHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uintptr_t*)(regs.ebp + 0x8);
            regs.ecx = *(uintptr_t*)(regs.ebx + 0x1C);

            PossiblyAddThisEntity((WplInstance*)regs.ebx);
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

        auto pattern = hook::pattern("E8 ? ? ? ? 83 3D ? ? ? ? 00 74 05 E8 ? ? ? ? 6A 05");
        injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), RegisterLODLights, true); //0x00402D6C
        pattern = hook::pattern("8B 5D 08 8B 4B 1C 8D 44 24 14 50 51");
        injector::MakeInline<LoadObjectInstanceHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6)); //0x008D63A1 - 0x008D63A7
        //pattern = hook::pattern("F3 0F 10 44 24 1C 0D 00 0C"); //causes crash
        //injector::MakeInline<ParseIdeObjsHook>(pattern.get(0).get<uintptr_t>(0), pattern.get(0).get<uintptr_t>(6)); //0x008D2311 - 0x8D2317
    }

    if (DisableDefaultLodLights)
    {
        auto pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC B4 00 00 00 F3 0F 10 45 08");
        injector::WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(0), 0xC3, true); //0x00903300
    }

    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 80 3D ? ? ? ?  00 74 ? 6A 00 6A 0C");
    static auto jmp = pattern.get(0).get<uintptr_t>(8);
    if (DisableCoronasWaterReflection == 1)
    {
        injector::MakeNOP(pattern.get(0).get<uintptr_t>(0), 5, true); //0xB54373
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
            }; injector::MakeInline<ReflectionsHook>(pattern.get(0).get<uintptr_t>(-7), pattern.get(0).get<uintptr_t>(-1));
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
    auto pattern = hook::pattern("01 15 ? ? ? ? 8D 0C 81 89 0D"); //820F75
    CLODLightManager::IV::CurrentTimeHours = *pattern.get(0).get<char*>(2); //0x11D5300
    CLODLightManager::IV::CurrentTimeMinutes = *pattern.get(0).get<char*>(11); //0x11D52FC
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 20 D9 05 ? ? ? ? F3 0F 10 45 08 6A 00");
    CLODLightManager::IV::DrawCorona = (int(__cdecl *)(float, float, float, float, unsigned int, float, unsigned char, unsigned char, unsigned char))(pattern.get(0).get<uintptr_t>(0)); //0xA6E240
    pattern = hook::pattern("A1 ? ? ? ? 56 8D 70 01 81 FE");
    CLODLightManager::IV::DrawCorona2 = (int(__cdecl *)(int id, char r, char g, char b, float a5, CVector* pos, float radius, float a8, float a9, int a10, float a11, char a12, char a13, int a14))(pattern.get(0).get<uintptr_t>(0)); //0x7E1970
    CLODLightManager::IV::DrawCorona3 = CLODLightManager::IV::DrawCorona2;
    pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00");
    CLODLightManager::IV::GetRootCam = (void(__stdcall *)(int *camera))(pattern.get(0).get<uintptr_t>(0)); //0xB006C0
    pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B 4C 24 04 89 01 C2 04 00");
    CLODLightManager::IV::GetGameCam = (void(__stdcall *)(int *camera))(pattern.get(0).get<uintptr_t>(0)); //0xB006E0
    pattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 10 F3 0F 10 45 0C D9 45 18 51");
    CLODLightManager::IV::CamIsSphereVisible = (bool(__cdecl *)(int camera, float pX, float pY, float pZ, float radius))(pattern.get(0).get<uintptr_t>(0)); //0xBB9340
    pattern = hook::pattern("55 8B EC 83 E4 F0 8B 4D 08 83 EC 10 8D 04 24 50 51 B9 ? ? ? ? E8 ? ? ? ? F3 ? ? ? ? 8B 55 0C");
    CLODLightManager::IV::GetCamPos = (void(__cdecl *)(int camera, float *pX, float *pY, float *pZ))(pattern.get(0).get<uintptr_t>(0)); //0xBB8510
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

    uintptr_t range_start = (uintptr_t)hook::get_pattern("BE ? ? ? ? 83 7E 18 00"); //0x7E0F95
    uintptr_t range_end = (uintptr_t)hook::get_pattern("5E C3 83 05 ? ? ? ? ? 5E C3"); //0x7E1AAB

    uintptr_t dword_temp = (uintptr_t)*hook::pattern("D9 98 ? ? ? ? F3 0F 10 44 24 20 F3 0F 11 88").get(0).get<uint32_t*>(2); //0x68C350

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

        GoThroughPatterns("D9 98", 2);
        GoThroughPatterns("F3 0F 11 80", 4);
        GoThroughPatterns("F3 0F 11 88", 4);
        GoThroughPatterns("83 88 ? ? ? ? 10", 2);
        GoThroughPatterns("83 88 ? ? ? ? 06", 2);
        GoThroughPatterns("BE", 1);
        GoThroughPatterns("89 88", 2);
        GoThroughPatterns("88 90", 2);
        GoThroughPatterns("88 88", 2);
        GoThroughPatterns("8B 90", 2);
    }


    range_start = (uintptr_t)hook::get_pattern("D9 46 20 F3 0F 10 54 24 ? 0F B6 56 30"); //0x7E10AB
    range_end = (uintptr_t)hook::get_pattern("F3 0F 59 C3 F3 0F 5C F0 F3 0F 59 CB"); //0x7E1671

    dword_temp = (uintptr_t)*hook::pattern("F3 0F 11 90 ? ? ? ? F3 0F 10 54 24 24 F3 0F").get(0).get<uint32_t*>(4);

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

        GoThroughPatterns("0F 28 89", 3);
        GoThroughPatterns("88 90", 2);
        GoThroughPatterns("88 88", 2);
        GoThroughPatterns("F3 0F 11 90", 4);
        GoThroughPatterns("F3 0F 11 80", 4);
        GoThroughPatterns("F3 0F 10 81", 4);
        GoThroughPatterns("F3 0F 11 88", 4);
        GoThroughPatterns("F3 0F 10 A1", 4);
        GoThroughPatterns("F3 0F 10 99", 4);
        GoThroughPatterns("D9 98", 2);
        GoThroughPatterns("80 B9 ? ? ? ? 00", 2);
        GoThroughPatterns("0F B6 B1", 3);
        GoThroughPatterns("0F B6 81", 3);
        GoThroughPatterns("0F B6 91", 3);
    }

    if (counter1 != 24 || counter2 != 18)
        MessageBox(0, "IV.Project2DFX", "Project2DFX is not fully compatible with this version of the game", 0);


    auto pattern = hook::pattern("C1 E0 ? 03 C3 C1 E0");
    WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true); //aslr_ptr(0x7E109F + 0x2)
    pattern = hook::pattern("C1 E1 ? 03 CF C1 E1");
    WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true); //aslr_ptr(0x7E149A + 0x2)
    pattern = hook::pattern("C1 E1 ? 03 C8 C1 E1");
    WriteMemory<uint8_t>(pattern.get(0).get<uintptr_t>(2), NewLimitExponent, true); //aslr_ptr(0x7E130E + 0x2)

    pattern = hook::pattern("81 FE ? ? ? ? 0F 8D ? ? ? ? 8B 4C 24 08 8A 54 24 0C");
    WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(2), nCoronasLimit, true);      //aslr_ptr(0x7E1979 + 0x2)
    pattern = hook::pattern("81 FF ? ? ? ? 0F 8D ? ? ? ? 8B 44 24 1C D9 46 20");
    WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(2), nCoronasLimit, true);      //aslr_ptr(0x7E1072 + 0x2)
    pattern = hook::pattern("3D ? ? ? ? 72 ? 89 0D ? ? ? ? C3");
    WriteMemory<uint32_t>(pattern.get(0).get<uintptr_t>(1), nCoronasLimit * 64, true); //aslr_ptr(0x7E1189 + 0x1)
}


void CLODLightManager::IV::RegisterCustomCoronas()
{
    unsigned short      nModelID = 65534;

    auto    itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f));
}

WplInstance* CLODLightManager::IV::PossiblyAddThisEntity(WplInstance* pInstance)
{
    if (m_bCatchLamppostsNow && IsModelALamppost(pInstance->ModelNameHash))
        RegisterLamppost(pInstance);

    return pInstance;
}

void CLODLightManager::IV::RegisterLamppost(WplInstance* pObj)
{
    DWORD               nModelID = pObj->ModelNameHash;
    CMatrix             dummyMatrix;

    float qw = pObj->RotationW;
    float qx = pObj->RotationX;
    float qy = pObj->RotationY;
    float qz = pObj->RotationZ;

    float n = 1.0f / sqrt(qx*qx + qy * qy + qz * qz + qw * qw);
    qx *= n;
    qy *= n;
    qz *= n;
    qw *= n;

    dummyMatrix.matrix.right.x = 1.0f - 2.0f*qy*qy - 2.0f*qz*qz;
    dummyMatrix.matrix.right.y = 2.0f*qx*qy - 2.0f*qz*qw;
    dummyMatrix.matrix.right.z = 2.0f*qx*qz + 2.0f*qy*qw;

    dummyMatrix.matrix.up.x = 2.0f*qx*qy + 2.0f*qz*qw;
    dummyMatrix.matrix.up.y = 1.0f - 2.0f*qx*qx - 2.0f*qz*qz;
    dummyMatrix.matrix.up.z = 2.0f*qy*qz - 2.0f*qx*qw;

    dummyMatrix.matrix.at.x = 2.0f*qx*qz - 2.0f*qy*qw;
    dummyMatrix.matrix.at.y = 2.0f*qy*qz + 2.0f*qx*qw;
    dummyMatrix.matrix.at.z = 1.0f - 2.0f*qx*qx - 2.0f*qy*qy;

    dummyMatrix.matrix.pos.x = pObj->PositionX;
    dummyMatrix.matrix.pos.y = pObj->PositionY;
    dummyMatrix.matrix.pos.z = pObj->PositionZ;

    if (GetDistance((RwV3d*)&CVector(pObj->PositionX, pObj->PositionY, pObj->PositionZ), (RwV3d*)&CVector(-278.37f, -1377.48f, 90.98f)) <= 300.0f)
        return;

    auto    itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(dummyMatrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, atan2(dummyMatrix.GetUp()->y, -dummyMatrix.GetUp()->x)));
}

void CLODLightManager::IV::RegisterLODLights()
{
    if (m_bCatchLamppostsNow)
        m_bCatchLamppostsNow = false;

    if (*CurrentTimeHours > 19 || *CurrentTimeHours < 7)
    {
        unsigned char   bAlpha = 0;
        float           fRadius = 0.0f;
        unsigned int    nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
        unsigned int    curMin = *CurrentTimeMinutes;

        if (nTime >= 19 * 60)
            bAlpha = static_cast<unsigned char>((3.0f / 4.0f)*nTime - 825.0f); // http://goo.gl/O03RpE {(19*60)a + y = 30,  (24*60)a + y = 255}
        else if (nTime < 3 * 60)
            bAlpha = 255;
        else
            bAlpha = static_cast<unsigned char>((-15.0f / 16.0f)*nTime + 424.0f); // http://goo.gl/M8Dev9 {(7*60)a + y = 30,  (3*60)a + y = 255}

        for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
        {
            static int currentCamera;
            GetRootCam(&currentCamera);
            if ((it->vecPos.z >= -15.0f) && (it->vecPos.z <= 1030.0f) /*&& CamIsSphereVisible(currentCamera, it->vecPos.x, it->vecPos.y, it->vecPos.z, 3.0f)*/)
            {
                CVector CamPos = CVector();
                GetCamPos(currentCamera, &CamPos.x, &CamPos.y, &CamPos.z);
                CVector*    pCamPos = &CamPos;
                float       fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);
                fCamHeight = CamPos.z;

                if ((fDistSqr > 250.0f*250.0f && fDistSqr < fCoronaFarClip*fCoronaFarClip) || it->nNoDistance)
                {
                    if (it->nNoDistance)
                        fRadius = 3.5f;
                    else
                        fRadius = (fDistSqr < 300.0f*300.0f) ? (0.07f)*sqrt(fDistSqr) - 17.5f : 3.5f; // http://goo.gl/vhAZSx

                    if (bSlightlyIncreaseRadiusWithDistance)
                        fRadius *= min((0.00136364f)*sqrt(fDistSqr) + 0.590909f, 3.0f); // http://goo.gl/3kDpnC {(300)a + y = 1.0,  (2500)a + y = 4}

                    if (it->fCustomSizeMult != 0.45f)
                    {
                        if (!it->nCoronaShowMode)
                        {
                            //DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
                            DrawCorona3(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                        }
                        //else
                        //{
                        //  static float blinking = 1.0f;
                        //  if (IsBlinkingNeeded(it->nCoronaShowMode))
                        //      blinking -= CTimer::ms_fTimeStep / 1000.0f;
                        //  else
                        //      blinking += CTimer::ms_fTimeStep / 1000.0f;
                        //
                        //  (blinking > 1.0f) ? blinking = 1.0f : (blinking < 0.0f) ? blinking = 0.0f : 0.0f;
                        //
                        //  CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, blinking * (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                        //}
                    }
                    else
                    {
                        fRadius *= 1.3f;
                        if ((it->colour.r >= 250 && it->colour.g >= 100 && it->colour.b <= 100) && ((curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59))) //yellow
                        {
                            //DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
                            DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                        }
                        else
                        {
                            if ((abs(it->fHeading) >= (3.1415f / 6.0f) && abs(it->fHeading) <= (5.0f * 3.1415f / 6.0f)))
                            {
                                if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    //DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
                                    DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                }
                                else
                                {
                                    if ((it->colour.r == 0 && it->colour.g >= 100 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        //DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
                                        DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                    }
                                }
                            }
                            else
                            {
                                if ((it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    //DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
                                    DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
                                }
                                else
                                {
                                    if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        //DrawCorona(it->vecPos.x, it->vecPos.y, it->vecPos.z, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 127.5f, 0, 0.0f, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.r, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.g, ((bAlpha * (it->colour.a / 255.0f)) / 500.0f) * it->colour.b);
                                        DrawCorona2(reinterpret_cast<unsigned int>(&*it), it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)) / 10.0f, (CVector*)&it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier) * 1270.5f, 0.0, 0.0, 0, 0.0, 0, 0, 0);
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

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        CLODLightManager::IV::Init();
    }
    return TRUE;
}