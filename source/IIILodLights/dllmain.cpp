#include "stdafx.h"
#include "CLODLightManager.h"
#include "CLODLights.h"
#include "CSearchlights.h"

int numCoronas;
std::vector<CEntityIII> VecEntities;
std::vector<int> ExplosionTypes = { 0,2,6,7,8,9 }; //1,3,4,5 - barrel crash
RwCamera*& Camera = *(RwCamera**)0x72676C;
int& CTimer::m_snTimeInMillisecondsPauseMode = *(int*)0x885B48;
float& CTimer::ms_fTimeStep = *(float*)0x8E2CB4;

char* CLODLightManager::III::CurrentTimeHours = (char*)0x95CDA6;
char* CLODLightManager::III::CurrentTimeMinutes = (char*)0x95CDC8;
float** CLODLightManager::III::fCurrentFarClip = (float**)0x48E5DC;
char(__cdecl *CLODLightManager::III::GetIsTimeInRange)(char hourA, char hourB) = (char(__cdecl *)(char, char)) 0x473420;
int* (__cdecl *CLODLightManager::III::GetModelInfo)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x50B860;
bool(__cdecl *const CLODLightManager::III::CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = (decltype(CLODLightManager::III::CShadowsStoreStaticShadow))0x5130A0;
float(__cdecl *CLODLightManager::III::FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject) = (float(__cdecl *)(float, float, float, BOOL *, CEntity **)) 0x4B3AE0;
RwV3D* (__cdecl *CLODLightManager::III::TransformPoint)(RwV3D *, RwMatrix *, RwV3D *) = (RwV3D *(__cdecl *)(RwV3D *, RwMatrix *, RwV3D *)) 0x4BA4D0;
bool(__thiscall *CLODLightManager::III::CCameraIsSphereVisible)(int *camera, RwV3D *origin, float radius) = (bool(__thiscall *)(int *, RwV3D *, float)) 0x0043D3B0;
void(__cdecl *CLODLightManager::III::RegisterCorona)(int coronaID, char r, char g, char b, char a, RwV3D *pos, float size, float farClip, char type, char flare, char reflection, char obstacles, char notUsed, float normalAngle) = (void(__cdecl *)(int, char, char, char, char, RwV3D *, float, float, char, char, char, char, char, float)) 0x4FA080;
void(__fastcall *CLODLightManager::III::CVectorNormalize)(RwV3D *in) = (void(__fastcall *)(RwV3D *in)) 0x4BA560;
RwV3D *(__cdecl *CLODLightManager::III::CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b) = (RwV3D *(__cdecl *)(RwV3D *out, RwV3D *a, RwV3D *b)) 0x4BA350;
int(__cdecl *CLODLightManager::III::RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags) = (int(__cdecl *)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags)) 0x5B6720;
int(__cdecl *CLODLightManager::III::RwIm3DRenderIndexedPrimitive)(int primType, short *indices, int numIndices) = (int(__cdecl *)(int primType, short *indices, int numIndices)) 0x5B6820;
int(__cdecl *CLODLightManager::III::RwIm3DEnd)() = (int(__cdecl *)()) 0x5B67F0;
int(__cdecl *CLODLightManager::III::RwRenderStateSetIII)(RwRenderState nState, void *pParam) = (int(__cdecl *)(RwRenderState nState, void *pParam)) 0x5A43C0;
void RwRenderStateSetIII(RwRenderState nState, void *pParam)
{
    CLODLightManager::III::RwRenderStateSetIII(nState, pParam);
}
void(*_RwRenderStateSet)(RwRenderState nState, void *pParam) = &RwRenderStateSetIII;


CVector* GetCamPos()
{
    return (CVector*)(0x006FACF8 + 0x770);
}

void CLODLightManager::III::Init()
{
    CIniReader iniReader("");
    bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
    numCoronas = iniReader.ReadInteger("LodLights", "MaxNumberOfLodLights", 25000);
    fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
    bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
    if (iniReader.ReadString("LodLights", "CoronaFarClip", "auto") == "auto")
        fCoronaFarClip = iniReader.ReadFloat("LodLights", "CoronaFarClip", 0.0f);
    else
        autoFarClip = true;

    bRenderStaticShadowsForLODs = iniReader.ReadInteger("StaticShadows", "RenderStaticShadowsForLODs", 0) != 0;
    bIncreasePedsCarsShadowsDrawDistance = iniReader.ReadInteger("StaticShadows", "IncreaseCarsShadowsDrawDistance", 0) != 0;
    fStaticShadowsIntensity = iniReader.ReadFloat("StaticShadows", "StaticShadowsIntensity", 0.0f);
    fStaticShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "StaticShadowsDrawDistance", 0.0f);
    fTrafficLightsShadowsIntensity = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsIntensity", 0.0f);
    fTrafficLightsShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsDrawDistance", 0.0f);

    bRenderSearchlightEffects = iniReader.ReadInteger("SearchLights", "RenderSearchlightEffects", 1) != 0;
    bRenderHeliSearchlights = iniReader.ReadInteger("SearchLights", "RenderHeliSearchlights", 1) != 0;
    fSearchlightEffectVisibilityFactor = iniReader.ReadFloat("SearchLights", "SearchlightEffectVisibilityFactor", 0.4f);
    nSmoothEffect = iniReader.ReadInteger("SearchLights", "SmoothEffect", 1);

    bEnableDrawDistanceChanger = iniReader.ReadInteger("DrawDistanceChanger", "Enable", 0) != 0;
    fMinDrawDistanceOnTheGround = iniReader.ReadFloat("DrawDistanceChanger", "MinDrawDistanceOnTheGround", 800.0f);
    fFactor1 = iniReader.ReadFloat("DrawDistanceChanger", "Factor1", 2.0f);
    fFactor2 = iniReader.ReadFloat("DrawDistanceChanger", "Factor2", 1.0f);
    fStaticSunSize = iniReader.ReadFloat("DrawDistanceChanger", "StaticSunSize", 20.0f);

    bAdaptiveDrawDistanceEnabled = iniReader.ReadInteger("AdaptiveDrawDistance", "Enable", 0) != 0;
    nMinFPSValue = iniReader.ReadInteger("AdaptiveDrawDistance", "MinFPSValue", 0);
    nMaxFPSValue = iniReader.ReadInteger("AdaptiveDrawDistance", "MaxFPSValue", 0);
    fMaxPossibleDrawDistance = iniReader.ReadFloat("AdaptiveDrawDistance", "MaxPossibleDrawDistance", 0.0f);

    fMaxDrawDistanceForNormalObjects = iniReader.ReadFloat("DistanceLimits", "MaxDrawDistanceForNormalObjects", 0.0);
    fDrawDistance = iniReader.ReadFloat("DistanceLimits", "DrawDistance", 0.0f);
    bPreloadLODs = iniReader.ReadInteger("DistanceLimits", "PreloadLODs", 0) != 0;

    bRandomExplosionEffects = iniReader.ReadInteger("Misc", "RandomExplosionEffects", 0) != 0;
    bReplaceSmokeTrailWithBulletTrail = iniReader.ReadInteger("Misc", "ReplaceSmokeTrailWithBulletTrail", 0) != 0;
    bFestiveLights = iniReader.ReadInteger("Misc", "FestiveLights", 1) != 0;
    bFestiveLightsAlways = iniReader.ReadInteger("Misc", "bFestiveLightsAlways", 0) != 0;

    ApplyMemoryPatches();
}

template<uintptr_t addr>
void CWorldAddHook()
{
    using func_hook = injector::function_hooker<addr, void(CEntityIII* EntityIII)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type CWorldAdd, CEntityIII* EntityIII)
    {
        if (CLODLightManager::III::bPreloadLODs)
        {
            if (EntityIII->m_nModelIndex == 404 || EntityIII->m_nModelIndex == 405 || EntityIII->m_nModelIndex == 416 || EntityIII->m_nModelIndex == 402 || EntityIII->m_nModelIndex == 403)
            {
                EntityIII->m_bIsVisible = 0;
            }
        }
        VecEntities.push_back(*EntityIII);

        CWorldAdd(EntityIII);
    });
}

template<uintptr_t addr>
void CBulletTracesAddTrace()
{
    using func_hook = injector::function_hooker<addr, void(CVector *, CVector *)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type AddTrace, CVector* start, CVector* end)
    {
        CVector endPoint = CVector();
        endPoint.x = (end->x - start->x) * 0.07f;
        endPoint.y = (end->y - start->y) * 0.07f;
        endPoint.z = (end->z - start->z) * 0.07f;
        injector::cstd<void(short, CVector const&, CVector const&, CEntity *, float, int, int, int, int)>::call<0x50D140>(47, *start, endPoint, 0, 0.0f, 0, 0, 0, 0);
    });
}

template<uintptr_t addr>
void CExplosionAddModifiedExplosion()
{
    using func_hook = injector::function_hooker<addr, bool(CEntity*, CEntity*, int, CVector*, uint32_t)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type AddExplosion, CEntity* pTarget, CEntity* pSource, int nType, CVector* pVector, uint32_t uTimer)
    {
        std::random_device rng;
        std::mt19937 urng(rng());
        std::shuffle(ExplosionTypes.begin(), ExplosionTypes.end(), urng);
        injector::MakeNOP(0x559FD3, 5, true);
        for (auto it = ExplosionTypes.begin(); it != ExplosionTypes.end(); ++it)
        {
            if (*it == nType)
                break;
            AddExplosion(pTarget, pSource, *it, pVector, uTimer);
        }
        injector::MakeCALL(0x559FD3, 0x4B1140, true);

        return AddExplosion(pTarget, pSource, nType, pVector, uTimer);
    });
}

template<uintptr_t addr>
void CCoronasRegisterFestiveCoronaForEntity()
{
    using func_hook = injector::function_hooker<addr, void(unsigned int nID, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, char a10, char a11, char a12, char a13, float a14)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type RegisterCorona, unsigned int nID, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, char a10, char a11, char a12, char a13, float a14)
    {
        auto it = CLODLights::FestiveLights.find(nID);
        if (it != CLODLights::FestiveLights.end())
        {
            RegisterCorona(nID, it->second.r, it->second.g, it->second.b, A, Position, Size, Range, pTex, a10, a11, a12, a13, a14);
        }
        else
        {
            CLODLights::FestiveLights[nID] = CRGBA(random(0, 255), random(0, 255), random(0, 255), 0);
            RegisterCorona(nID, R, G, B, A, Position, Size, Range, pTex, a10, a11, a12, a13, a14);
        }
    });
}

void CLODLightManager::III::ApplyMemoryPatches()
{
    injector::WriteMemory<float>(0x5F00BC, 300.0f, true); // Traffic lights coronas draw distance DOUBLE CHECK
    struct GenericIDEHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebx + 0x6ED4E0) = 32;

            static char* buffer = (char *)0x6ED4E0;
            unsigned int modelID = 0, IDEDrawDistance = 0;

            auto tempptr = strchr(buffer, ',');
            auto tempptr2 = strchr(buffer, '.');

            if (!tempptr && !tempptr2)
            {
                sscanf(buffer, "%d %*s %*s %*d %d %*s %*s %*s", &modelID, &IDEDrawDistance);
                if ((modelID >= 1100 && modelID <= 1438) || modelID == 887)
                {
                    if (IDEDrawDistance >= 10 && IDEDrawDistance < 300)
                    {
                        char sIDEDrawDistance[5] = { 0 }, Flags2[20] = { 0 };
                        sprintf(sIDEDrawDistance, "%d", IDEDrawDistance);
                        tempptr = strstr(buffer + 10, sIDEDrawDistance);

                        if (IDEDrawDistance >= 100)
                            strncpy(Flags2, tempptr + 4, 15);
                        else
                            strncpy(Flags2, tempptr + 3, 15);

                        strncpy(tempptr, "300  ", 5);
                        strncpy(tempptr + 5, Flags2, 15);
                    }
                }
            }
            else
            {
                tempptr2 = strstr(buffer, "shad_exp");
                if (!tempptr && tempptr2)
                {
                    sscanf(tempptr2 + 11, "%d", &IDEDrawDistance);

                    if (IDEDrawDistance >= 100 && IDEDrawDistance < 300)
                    {
                        strncpy(tempptr2 + 11, "300", 3);
                    }
                }
            }
        }
    }; injector::MakeInline<GenericIDEHook>(0x476208, 0x476208 + 7);

    struct Render
    {
        void operator()(injector::reg_pack&)
        {
            ((void(__cdecl *)())0x4F8FB0)(); //CCoronas::Render();

            if (bRenderLodLights)
                CLODLights::RenderBuffered();

            if (bRenderSearchlightEffects)
                CSearchlights::RenderSearchLightsIII();
        }
    }; injector::MakeInline<Render>(0x48E0B8);


    if (bRenderLodLights)
    {
        CLODLights::Inject();

        //injector::MakeNOP(0x4F8E82, 6, true); //disable ambientBrightness change
        //injector::MakeNOP(0x4F8F16, 6, true);

        CWorldAddHook<(0x4787FE)>();
        CWorldAddHook<(0x47899C)>();

        struct asmInit
        {
            void operator()(injector::reg_pack& regs)
            {
                LoadDatFile();

                for (auto i : VecEntities)
                {
                    if (m_bCatchLamppostsNow && IsModelALamppost(i.m_nModelIndex))
                    {
                        RegisterLamppost(&i);
                    }
                }

                RegisterCustomCoronas();
                m_bCatchLamppostsNow = false;
                m_pLampposts->shrink_to_fit();
                VecEntities.clear();
                pFileContent->clear();

                injector::MakeCALL(0x48D440, RegisterLODLights, true);

                if (fDrawDistance)
                {
                    injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (fDrawDistance / 1.8f), true);
                    injector::WriteMemory<float>(0x487629 + 6, 1.2f * (fDrawDistance / 1.8f), true);
                }
            }
        }; injector::MakeInline<asmInit>(0x48C09F);
    }

    if (nSmoothEffect)
    {
        nSmoothEffect = 1;
    }

    if (fTrafficLightsShadowsDrawDistance)
    {
        injector::WriteMemory(0x455E3F + 0x2, &fTrafficLightsShadowsDrawDistance, true);
        injector::WriteMemory(0x455F2D + 0x2, &fTrafficLightsShadowsDrawDistance, true);
    }

    if (fStaticShadowsDrawDistance)
    {
        injector::WriteMemory<float>(0x5F00E0, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x5EDF3C, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x5FB214, fStaticShadowsDrawDistance, true);
    }

    if (fStaticShadowsIntensity)
    {
        injector::WriteMemory<float>(0x5FB304, fStaticShadowsIntensity, true);
        injector::WriteMemory<int>(0x4FACE6, 255, true);
    }

    if (fTrafficLightsShadowsIntensity)
    {
        injector::WriteMemory<float>(0x5F00EC, fTrafficLightsShadowsIntensity, true);
    }

    if (bIncreasePedsCarsShadowsDrawDistance)
    {
        injector::WriteMemory<unsigned char>(0x00513AC2, 0x75u, true); //headlight on far distance
        injector::WriteMemory<unsigned char>(0x0051388F, 0x75u, true); //shadow on far distance
        injector::WriteMemory<unsigned char>(0x005394C6, 0x55u, true); //rgb
        injector::WriteMemory<unsigned char>(0x005394C8, 0x55u, true);
        injector::WriteMemory<unsigned char>(0x005394CA, 0x55u, true);
        injector::WriteMemory<unsigned int>(0x537983, 0x008F2A00, true);
        injector::MakeNOP(0x518DCA, 5, true);
        injector::MakeJMP(0x513CFF, 0x513D92); //ped shadows draw distance
    }

    if (fDrawDistance)
    {
        injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (fDrawDistance / 1.8f), true);
        injector::MakeInline<0x486AF2>([](injector::reg_pack&)
        {
            injector::thiscall<void()>::call<0x488CC0>();
            injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (fDrawDistance / 1.8f), true);
        });
        injector::MakeInline<0x486B3A>([](injector::reg_pack& regs)
        {
            *(uintptr_t*)regs.esp = 0x486D16;
            injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (fDrawDistance / 1.8f), true);
        });
        injector::MakeInline<0x48B314>([](injector::reg_pack& regs)
        {
            *(uintptr_t*)regs.esp = 0x48B42C;
            injector::WriteMemory<float>(0x5F726C, *(float*)0x5F726C * (fDrawDistance / 1.8f), true);
        });
        injector::WriteMemory<float>(0x487629 + 6, 1.2f * (fDrawDistance / 1.8f), true);
    }

    if (fMaxDrawDistanceForNormalObjects)
    {
        //injector::WriteMemory<float>(0x5F72A4, fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8AB1, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8AC6, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8AD9, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8B0E, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8B21, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8B34, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8B82, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8B97, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8BAA, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8BDF, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8BF2, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8C05, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4A8DA6, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA391, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA3A6, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA3B9, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA3EE, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA401, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA414, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA462, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA477, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA48A, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA4BF, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA4D2, &fMaxDrawDistanceForNormalObjects, true);
        injector::WriteMemory(0x4AA4E5, &fMaxDrawDistanceForNormalObjects, true);
    }

    if (bEnableDrawDistanceChanger)
    {
        injector::MakeJMP(0x48E072, DrawDistanceChanger, true);

        injector::WriteMemory(0x48E5DA + 0x2, &fNewFarClip, true);
        //injector::WriteMemory(0x48E5E6 + 0x2, &fNewFarClip, true);
    }

    if (bRandomExplosionEffects)
    {
        CExplosionAddModifiedExplosion<(0x4309EB)>();
        CExplosionAddModifiedExplosion<(0x442E65)>();
        CExplosionAddModifiedExplosion<(0x53BF2A)>();
        CExplosionAddModifiedExplosion<(0x53DA3C)>();
        CExplosionAddModifiedExplosion<(0x541DAB)>();
        CExplosionAddModifiedExplosion<(0x549773)>();
        CExplosionAddModifiedExplosion<(0x549F90)>();
        CExplosionAddModifiedExplosion<(0x54A349)>();
        CExplosionAddModifiedExplosion<(0x54C265)>();
        CExplosionAddModifiedExplosion<(0x54C6C0)>();
        CExplosionAddModifiedExplosion<(0x54C7AD)>();
        CExplosionAddModifiedExplosion<(0x54CC04)>();
        //CExplosionAddModifiedExplosion<(0x55B743)>(); // molotov/grenade expl
        CExplosionAddModifiedExplosion<(0x55B7A9)>();
        CExplosionAddModifiedExplosion<(0x564ADE)>(); //barrels
    }

    if (bReplaceSmokeTrailWithBulletTrail)
    {
        CBulletTracesAddTrace<(0x55F9C7)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
        //CBulletTracesAddTrace<(0x560599)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
        //CBulletTracesAddTrace<(0x560F21)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
        CBulletTracesAddTrace<(0x56186B)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
        //CBulletTracesAddTrace<(0x562B07)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
        //CBulletTracesAddTrace<(0x562E4F)>(); // = 0x518E90 + 0x0  -> call    sub_518E90
    }

    if (bPreloadLODs)
    {
        injector::WriteMemory<uint8_t>(0x47561F, 0xEB, true);
        injector::MakeNOP(0x475615, 9, true);
        injector::WriteMemory(0x475615, 0x005E43C7, true);
        injector::WriteMemory(0x475615 + 4, 0x90000000, true);

        injector::MakeInline<0x40B7DA, 0x40B8F4>([](injector::reg_pack& regs)
        {
            static auto CPopulationDealWithZoneChange = injector::cstd<void(int a1, int a2, char a3)>::call<0x4F6200>;
            static auto LoadCollisionFile1 = injector::cstd<void(int a1)>::call<0x476520>;
            static auto sub_595BD0 = injector::cstd<void()>::call<0x595BD0>;
            CPopulationDealWithZoneChange(0x8F6250, *(char*)0x941514, 0);
            LoadCollisionFile1(*(char*)0x941514);
            *(DWORD*)0x8F6250 = *(char*)0x941514;
            sub_595BD0();
        });
    }

    if (bFestiveLights)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm *date = std::localtime(&now_c);
        if (bFestiveLightsAlways || (date->tm_mon == 0 && date->tm_mday <= 1) || (date->tm_mon == 11 && date->tm_mday >= 31))
        {
            CLODLights::RegisterCorona = &CLODLights::RegisterFestiveCorona;
            CCoronasRegisterFestiveCoronaForEntity<(0x4FA8F0)>();
        }
    }
}

void CLODLightManager::III::RegisterCustomCoronas()
{
    unsigned short      nModelID = 65534;

    auto    itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f));
}

void CLODLightManager::III::RegisterLamppost(CEntityIII* entity)
{
    auto    itEnd = pFileContent->upper_bound(PackKey(entity->m_nModelIndex, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(entity->m_nModelIndex, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(entity->matrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, atan2(entity->matrix.GetUp()->y, -entity->matrix.GetUp()->x)));
}

void CLODLightManager::III::RegisterLODLights()
{
    if (GetIsTimeInRange(20, 7))
    {
        unsigned char   bAlpha = 0;
        float           fRadius = 0.0f;
        unsigned int    nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
        unsigned int    curMin = *CurrentTimeMinutes;
        fCoronaFarClip = autoFarClip ? **fCurrentFarClip : fCoronaFarClip;

        if (nTime >= 20 * 60)
            bAlpha = static_cast<unsigned char>((15.0f / 16.0f)*nTime - 1095.0f); // http://goo.gl/O03RpE {(20*60)a + y = 30,  (24*60)a + y = 255}
        else if (nTime < 3 * 60)
            bAlpha = 255;
        else
            bAlpha = static_cast<unsigned char>((-15.0f / 16.0f)*nTime + 424.0f); // http://goo.gl/M8Dev9 {(7*60)a + y = 30,  (3*60)a + y = 150}

        for (auto it = m_pLampposts->cbegin(); it != m_pLampposts->cend(); it++)
        {
            if ((it->vecPos.z >= -15.0f) && (it->vecPos.z <= 1030.0f))
            {
                CVector*    pCamPos = (CVector*)GetCamPos();
                float       fDistSqr = (pCamPos->x - it->vecPos.x)*(pCamPos->x - it->vecPos.x) + (pCamPos->y - it->vecPos.y)*(pCamPos->y - it->vecPos.y) + (pCamPos->z - it->vecPos.z)*(pCamPos->z - it->vecPos.z);

                if ((fDistSqr > 250.0f*250.0f && fDistSqr < fCoronaFarClip*fCoronaFarClip) || it->nNoDistance)
                {
                    if (it->nNoDistance)
                        fRadius = 3.5f;
                    else
                        fRadius = (fDistSqr < 300.0f*300.0f) ? (0.07f)*sqrt(fDistSqr) - 17.5f : 3.5f; // http://goo.gl/vhAZSx

                    if (bSlightlyIncreaseRadiusWithDistance)
                        fRadius *= min((0.0025f)*sqrt(fDistSqr) + 0.25f, 4.0f); // http://goo.gl/3kDpnC

                    if (it->fCustomSizeMult != 0.45f)
                    {
                        if (!it->nCoronaShowMode)
                        {
                            CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                            bRenderStaticShadowsForLODs ? CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), 2, *(RwTexture **)0x8F2A00, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, it->colour.r, it->colour.g, it->colour.b, 15.0f, 1.0f, fCoronaFarClip, false, 0.0f) : nullptr;
                        }
                        else
                        {
                            static float blinking;
                            if (IsBlinkingNeeded(it->nCoronaShowMode))
                                blinking -= CTimer::ms_fTimeStep / 1000.0f;
                            else
                                blinking += CTimer::ms_fTimeStep / 1000.0f;

                            (blinking > 1.0f) ? blinking = 1.0f : (blinking < 0.0f) ? blinking = 0.0f : 0.0f;

                            CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, blinking * (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                        }
                    }
                    else
                    {
                        if ((it->colour.r >= 250 && it->colour.g >= 100 && it->colour.b <= 100) && ((curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59))) //yellow
                        {
                            CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                        }
                        else
                        {
                            if ((abs(it->fHeading) >= (3.1415f / 6.0f) && abs(it->fHeading) <= (5.0f * 3.1415f / 6.0f)))
                            {
                                if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                                }
                                else
                                {
                                    if ((it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                                    }
                                }
                            }
                            else
                            {
                                if ((it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                                }
                                else
                                {
                                    if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0xFF, 255.0f, false, false);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    CLODLights::Update();
}

void CLODLightManager::III::DrawDistanceChanger()
{
    static Fps _fps;
    if (bAdaptiveDrawDistanceEnabled)
    {
        _fps.update();
        int FPScount = _fps.get();
        if (FPScount < nMinFPSValue)
        {
            fMinDrawDistanceOnTheGround -= 2.0f;
        }
        else if (FPScount >= nMaxFPSValue)
        {
            fMinDrawDistanceOnTheGround += 2.0f;
        }
        if (fMinDrawDistanceOnTheGround < 800.0f)
            fMinDrawDistanceOnTheGround = 800.0f;
        else if (fMinDrawDistanceOnTheGround > fMaxPossibleDrawDistance)
            fMinDrawDistanceOnTheGround = fMaxPossibleDrawDistance;
    }
    fNewFarClip = (fFactor1 / fFactor2) * (GetCamPos()->z) + fMinDrawDistanceOnTheGround;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (injector::address_manager::singleton().IsIII())
        {
            if (injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0)
            {
                CLODLightManager::III::Init();
            }
        }
    }
    return TRUE;
}