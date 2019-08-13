#include "..\includes\stdafx.h"
#include "..\includes\CLODLightManager.h"
#include "..\includes\CLODLights.h"
#include "CSearchlights.h"

int numCoronas;
std::vector<CEntityVC> VecEntities;
std::vector<int> ExplosionTypes = { 0,2,6,7,8,9,10,11 }; //1,3,4,5 - barrel crash
RwCamera*& Camera = *(RwCamera**)0x8100BC;
int& CTimer::m_snTimeInMillisecondsPauseMode = *(int*)0x974B2C;
float& CTimer::ms_fTimeStep = *(float*)0x975424;

char* CLODLightManager::VC::CurrentTimeHours = (char*)0xA10B6B;
char* CLODLightManager::VC::CurrentTimeMinutes = (char*)0xA10B92;
float** CLODLightManager::VC::fCurrentFarClip = (float**)0x4A602D;
char(__cdecl *CLODLightManager::VC::GetIsTimeInRange)(char hourA, char hourB) = (char(__cdecl *)(char, char)) 0x4870F0;
bool(__cdecl *const CLODLightManager::VC::CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = (decltype(CLODLightManager::VC::CShadowsStoreStaticShadow))0x56E780;
float(__cdecl *CLODLightManager::VC::FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject) = (float(__cdecl *)(float, float, float, BOOL *, CEntity **)) 0x4D53A0;
RwV3D* (__cdecl *CLODLightManager::VC::TransformPoint)(RwV3d *a1, RwV3d *a2, int a3, RwMatrix *a4) = (struct RwV3D *(__cdecl *)(RwV3d *a1, RwV3d *a2, int a3, RwMatrix *a4)) 0x647160;
bool CLODLightManager::VC::CCameraIsSphereVisible(RwV3D *origin, float radius)
{
    RwV3D SpherePos;
    SpherePos.x = origin->x;
    SpherePos.y = origin->y;
    SpherePos.z = origin->z;
    if (origin->z <= *(float *)0x688640)
        SpherePos.z = FindGroundZFor3DCoord(origin->x, origin->y, origin->z, nullptr, nullptr);
    TransformPoint(&SpherePos, &SpherePos, 1, (RwMatrix*)0x7E4EA8);

    if (SpherePos.y + radius >= *(float *)0x978534)
    {
        if (SpherePos.y - radius <= *(float *)0xA10678)
        {
            if (SpherePos.y * *(float *)0x7E4F44 + SpherePos.x * *(float *)0x7E4F40 <= radius)
            {
                if (SpherePos.y * *(float *)0x7E4F50 + SpherePos.x * *(float *)0x7E4F4C <= radius)
                {
                    if (SpherePos.z * *(float *)0x7E4F60 + SpherePos.y * *(float *)0x7E4F5C <= radius)
                    {
                        if (SpherePos.z * *(float *)0x7E4F6C + SpherePos.y * *(float *)0x7E4F68 <= radius)
                            return true;
                    }
                }
            }
        }
    }
    return false;
}
void(__cdecl *CLODLightManager::VC::RegisterCorona)(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16) = (void(__cdecl *)(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16)) 0x5427A0;
void(__fastcall *CLODLightManager::VC::CVectorNormalize)(RwV3D *in) = (void(__fastcall *)(RwV3D *in)) 0x4DFEA0;
RwV3D *(__cdecl *CLODLightManager::VC::CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b) = (RwV3D *(__cdecl *)(RwV3D *out, RwV3D *a, RwV3D *b)) 0x4E00B0;
int(__cdecl *CLODLightManager::VC::RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags) = (int(__cdecl *)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags)) 0x65AE90;
int(__cdecl *CLODLightManager::VC::RwIm3DRenderIndexedPrimitive)(int primType, short *indices, int numIndices) = (int(__cdecl *)(int primType, short *indices, int numIndices)) 0x65AF90;
int(__cdecl *CLODLightManager::VC::RwIm3DEnd)() = (int(__cdecl *)()) 0x65AF60;
int(__cdecl *CLODLightManager::VC::RwRenderStateSetVC)(RwRenderState nState, void *pParam) = (int(__cdecl *)(RwRenderState nState, void *pParam)) 0x649BA0;
void RwRenderStateSetVC(RwRenderState nState, void *pParam)
{
    CLODLightManager::VC::RwRenderStateSetVC(nState, pParam);
}
void(*_RwRenderStateSet)(RwRenderState nState, void *pParam) = &RwRenderStateSetVC;

CVector* GetCamPos()
{
    return (CVector*)(0x7E4688 + 0x7D8);
}

void CLODLightManager::VC::Init()
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
    using func_hook = injector::function_hooker<addr, void(CEntityVC* EntityVC)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type CWorldAdd, CEntityVC* EntityVC)
    {
        if (CLODLightManager::VC::bPreloadLODs)
        {
            if (EntityVC->m_nModelIndex == 2600 || EntityVC->m_nModelIndex == 2544 || EntityVC->m_nModelIndex == 2634 || EntityVC->m_nModelIndex == 2545)
            {
                EntityVC->m_bIsVisible = 0;
            }
        }
        VecEntities.push_back(*EntityVC);

        CWorldAdd(EntityVC);
    });
}

template<uintptr_t addr>
void RenderSirenParticles()
{
    using func_hook = injector::function_hooker<addr, void(int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type RegisterCorona, int id, char r, char g, char b, char alpha, RwV3D *pos, float radius, float farClp, char a9, char lensflare, char a11, char see_through_effect, char trace, float a14, char a15, float a16)
    {
        RegisterCorona(id, r, g, b, alpha, pos, radius, farClp, a9, lensflare, a11, see_through_effect, trace, a14, a15, a16);
        CLODLightManager::VC::CShadowsStoreStaticShadow(id, 2, *(RwTexture **)0x978DB4, (CVector*)pos, 8.0f, 0.0f, 0.0f, -8.0f, 80, r != 0 ? 25 : 0, g != 0 ? 25 : 0, b != 0 ? 25 : 0, 15.0f, 1.0f, farClp, false, 8.0f);
    });
}

template<uintptr_t addr>
void CBulletTracesAddTrace()
{
    using func_hook = injector::function_hooker<addr, void(CVector *, CVector *, float, unsigned int, unsigned char)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type AddTrace, CVector* start, CVector* end, float, unsigned int, unsigned char)
    {
        CVector endPoint = CVector();
        endPoint.x = (end->x - start->x) * 0.15f;
        endPoint.y = (end->y - start->y) * 0.15f;
        endPoint.z = (end->z - start->z) * 0.15f;
        injector::cstd<void(short, CVector const&, CVector const&, CEntity *, float, int, int, int, int)>::call<0x5648F0>(56, *start, endPoint, 0, 0.0f, 0, 0, 0, 0);
    });
}

template<uintptr_t addr>
void CExplosionAddModifiedExplosion()
{
    using func_hook = injector::function_hooker<addr, bool(CEntity*, CEntity*, int, CVector*, uint32_t, bool)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type AddExplosion, CEntity* pTarget, CEntity* pSource, int nType, CVector* pVector, uint32_t uTimer, bool bUnknown)
    {
        std::random_device rng;
        std::mt19937 urng(rng());
        std::shuffle(ExplosionTypes.begin(), ExplosionTypes.end(), urng);
        injector::MakeNOP(0x5C6661, 5, true);
        for (auto it = ExplosionTypes.begin(); it != ExplosionTypes.end(); ++it)
        {
            if (*it == nType)
                break;
            AddExplosion(pTarget, pSource, *it, pVector, uTimer, bUnknown);
        }
        injector::MakeCALL(0x5C6661, 0x4D82D0, true);

        return AddExplosion(pTarget, pSource, nType, pVector, uTimer, bUnknown);
    });
}

template<uintptr_t addr>
void CCoronasRegisterFestiveCoronaForEntity()
{
    using func_hook = injector::function_hooker<addr, void(unsigned int nID, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, char a10, char a11, char a12, char a13, float a14, char a15, float a16)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type RegisterCorona, unsigned int nID, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, char a10, char a11, char a12, char a13, float a14, char a15, float a16)
    {
        auto it = CLODLights::FestiveLights.find(nID);
        if (it != CLODLights::FestiveLights.end())
        {
            RegisterCorona(nID, it->second.r, it->second.g, it->second.b, A, Position, Size, Range, pTex, a10, a11, a12, a13, a14, a15, a16);
        }
        else
        {
            CLODLights::FestiveLights[nID] = CRGBA(random(0, 255), random(0, 255), random(0, 255), 0);
            RegisterCorona(nID, R, G, B, A, Position, Size, Range, pTex, a10, a11, a12, a13, a14, a15, a16);
        }
    });
}

void CLODLightManager::VC::ApplyMemoryPatches()
{
    injector::WriteMemory<char>(0x542E66, 127, true); // sun reflection
    injector::WriteMemory<float>(0x68A860, 300.0f, true); // Traffic lights coronas draw distance

    RenderSirenParticles<(0x58C704)>();
    RenderSirenParticles<(0x58C764)>();

    struct GenericIDEHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)(regs.ebp + 0x7D7C38) = 32;

            static char* buffer = (char *)0x7D7C38;
            unsigned int modelID = 0, IDEDrawDistance = 0;

            auto tempptr = strchr(buffer, ',');
            auto tempptr2 = strchr(buffer, '.');

            if (!tempptr && !tempptr2)
            {
                sscanf(buffer, "%d %*s %*s %*d %d %*s %*s %*s", &modelID, &IDEDrawDistance);
                if ((modelID >= 300 && modelID <= 632))
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
    }; injector::MakeInline<GenericIDEHook>(0x48D4BD, 0x48D4BD + 7);

    struct Render
    {
        void operator()(injector::reg_pack&)
        {
            ((void(__cdecl *)())0x543500)(); //CCoronas::Render();

            if (bRenderLodLights)
                CLODLights::RenderBuffered();

            if (bRenderSearchlightEffects)
                CSearchlights::RenderSearchLightsVC();
        }
    }; injector::MakeInline<Render>(0x4A653D);


    if (bRenderLodLights)
    {
        CLODLights::Inject();

        //injector::MakeNOP(0x544186, 6, true); //disable ambientBrightness change
        //injector::MakeNOP(0x544533, 6, true);

        CWorldAddHook<(0x48AD9C)>();
        CWorldAddHook<(0x48AF52)>();

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

                injector::MakeCALL(0x4A6547, RegisterLODLights, true);
            }
        }; injector::MakeInline<asmInit>(0x4A4D10);
    }

    if (nSmoothEffect)
    {
        nSmoothEffect = 1;
    }

    if (fTrafficLightsShadowsDrawDistance)
    {
        injector::WriteMemory<float>(0x68A848, fTrafficLightsShadowsDrawDistance, true);
    }

    if (fStaticShadowsDrawDistance)
    {
        injector::WriteMemory<float>(0x6882A4, fStaticShadowsDrawDistance, true);
        injector::WriteMemory(0x541590 + 0x7A4 + 2, &fStaticShadowsDrawDistance, true);
        injector::WriteMemory(0x541590 + 0x8D5 + 2, &fStaticShadowsDrawDistance, true);
    }

    if (fStaticShadowsIntensity)
    {
        injector::WriteMemory<float>(0x69587C, fStaticShadowsIntensity, true);
        injector::WriteMemory<int>(0x465914, 255, true);
    }

    if (fTrafficLightsShadowsIntensity)
    {
        injector::WriteMemory(0x463F90 + 0x7E9 + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0x82E + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0x873 + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0xF4F + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0xF94 + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0xFD7 + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0x18CE + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0x1913 + 2, &fTrafficLightsShadowsIntensity, true);
        injector::WriteMemory(0x463F90 + 0x1956 + 2, &fTrafficLightsShadowsIntensity, true);
    }

    if (bIncreasePedsCarsShadowsDrawDistance)
    {
        injector::MakeJMP(0x56DA3F, 0x56DBF3, true); //ped shadows draw distance

        //Car Shadows
        injector::WriteMemory<unsigned char>(0x0056DEC1, 0x85u, true); //headlight twitching fix
        injector::WriteMemory<unsigned char>(0x0056DD36, 0x75u, true); //headlight on far distance
        injector::WriteMemory<unsigned char>(0x0056E004, 0x75u, true); //shadow on far distance
        injector::WriteMemory<unsigned char>(0x0058E2B7, 0x55u, true); //rgb
        injector::WriteMemory<unsigned char>(0x0058E2B9, 0x55u, true);
        injector::WriteMemory<unsigned char>(0x0058E2BB, 0x55u, true);
    }

    if (fDrawDistance)
    {
        injector::WriteMemory<float>(0x690220, *(float*)0x690220 * (fDrawDistance / 1.8f), true);
        injector::MakeInline<0x498B65>([](injector::reg_pack& regs)
        {
            *(uintptr_t*)regs.esp = 0x498CC8;
            injector::WriteMemory<float>(0x690220, *(float*)0x690220 * (fDrawDistance / 1.8f), true);
        });


        struct DDHookNoLambda
        {
            void operator()(injector::reg_pack& regs)
            {
                _asm {fstp dword ptr ds : [00690220h]}
                injector::WriteMemory<float>(0x690220, *(float*)0x690220 * (fDrawDistance / 1.8f), true);
            }
        }; injector::MakeInline<DDHookNoLambda>(0x490132, 0x490132 + 6);
        injector::WriteMemory<float>(0x499800 + 3, 1.2f * (fDrawDistance / 1.8f), true);
    }

    if (fMaxDrawDistanceForNormalObjects)
    {
        injector::WriteMemory<float>(0x69022C, fMaxDrawDistanceForNormalObjects, true);
    }

    if (bEnableDrawDistanceChanger)
    {
        injector::MakeJMP(0x4A65CD, DrawDistanceChanger, true);

        injector::WriteMemory(0x4A602B + 0x2, &fNewFarClip, true);
        //injector::WriteMemory(0x4A6037 + 0x2, &fNewFarClip, true);
    }

    if (bPreloadLODs)
    {
        injector::WriteMemory<uint8_t>(0x487CD8, 0xEB, true);
        injector::WriteMemory<uint8_t>(0x4A68A0, 0xC3, true);

        injector::MakeNOP(0x40DFE4, 5, true);
        injector::MakeNOP(0x40E242, 5, true);

        injector::MakeNOP(0x40E150, 5, true);
        injector::MakeNOP(0x40E157, 5, true);

        struct SetupBigBuildingVisibilityHook //Lods in the interiors
        {
            void operator()(injector::reg_pack& regs)
            {
                if (*(uint32_t*)0x978810 == 0) //nCurrentInterior
                    *(uintptr_t*)(regs.esp - 4) = 0x4C799A;
                else
                    *(uintptr_t*)(regs.esp - 4) = 0x4C7961;
            }
        }; injector::MakeInline<SetupBigBuildingVisibilityHook>(0x4C7957);
    }

    if (bRandomExplosionEffects)
    {
        CExplosionAddModifiedExplosion<(0x44038A)>();
        CExplosionAddModifiedExplosion<(0x4579A7)>();
        CExplosionAddModifiedExplosion<(0x5869B5)>();
        CExplosionAddModifiedExplosion<(0x588DC9)>();
        CExplosionAddModifiedExplosion<(0x5997C6)>();
        CExplosionAddModifiedExplosion<(0x59F819)>();
        CExplosionAddModifiedExplosion<(0x5AD18F)>();
        CExplosionAddModifiedExplosion<(0x5AD3F2)>();
        CExplosionAddModifiedExplosion<(0x5AFEC2)>();
        CExplosionAddModifiedExplosion<(0x5B0320)>();
        CExplosionAddModifiedExplosion<(0x5B040F)>();
        CExplosionAddModifiedExplosion<(0x5B0867)>();
        CExplosionAddModifiedExplosion<(0x5C6DB9)>();
        CExplosionAddModifiedExplosion<(0x5C6DDC)>();
        CExplosionAddModifiedExplosion<(0x5C6E23)>();
        CExplosionAddModifiedExplosion<(0x5C6EDE)>();
        //CExplosionAddModifiedExplosion<(0x5C6EFD)>(); //molotov
        CExplosionAddModifiedExplosion<(0x5C6F3D)>();
        CExplosionAddModifiedExplosion<(0x5C704E)>();
        //CExplosionAddModifiedExplosion<(0x5C706D)>(); //molotov
        CExplosionAddModifiedExplosion<(0x5C70AD)>();
        CExplosionAddModifiedExplosion<(0x5C71C5)>();
        CExplosionAddModifiedExplosion<(0x5C720F)>();
        CExplosionAddModifiedExplosion<(0x5C8B89)>(); //barrels
        CExplosionAddModifiedExplosion<(0x60A51D)>();
        CExplosionAddModifiedExplosion<(0x630168)>();
    }

    if (bReplaceSmokeTrailWithBulletTrail)
    {
        CBulletTracesAddTrace<(0x573E69)>();
    }

    if (bFestiveLights)
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm *date = std::localtime(&now_c);
        if (bFestiveLightsAlways || (date->tm_mon == 0 && date->tm_mday <= 1) || (date->tm_mon == 11 && date->tm_mday >= 31))
        {
            CLODLights::RegisterCorona = &CLODLights::RegisterFestiveCorona;
            CCoronasRegisterFestiveCoronaForEntity<(0x5419E0)>();
            CCoronasRegisterFestiveCoronaForEntity<(0x5421A5)>();
        }
    }
}

void CLODLightManager::VC::RegisterCustomCoronas()
{
    unsigned short      nModelID = 65534;

    auto    itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f));
}

void CLODLightManager::VC::RegisterLamppost(CEntityVC* entity)
{
    auto    itEnd = pFileContent->upper_bound(PackKey(entity->m_nModelIndex, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(entity->m_nModelIndex, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(entity->matrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, atan2(entity->matrix.GetUp()->y, -entity->matrix.GetUp()->x)));
}

void CLODLightManager::VC::RegisterLODLights()
{
    if (GetIsTimeInRange(19, 7))
    {
        unsigned char   bAlpha = 0;
        float           fRadius = 0.0f;
        unsigned int    nTime = *CurrentTimeHours * 60 + *CurrentTimeMinutes;
        unsigned int    curMin = *CurrentTimeMinutes;
        fCoronaFarClip = autoFarClip ? **fCurrentFarClip : fCoronaFarClip;

        if (nTime >= 19 * 60)
            bAlpha = static_cast<unsigned char>((3.0f / 4.0f)*nTime - 825.0f); // http://goo.gl/O03RpE {(19*60)a + y = 30,  (24*60)a + y = 255}
        else if (nTime < 3 * 60)
            bAlpha = 255;
        else
            bAlpha = static_cast<unsigned char>((-15.0f / 16.0f)*nTime + 424.0f); // http://goo.gl/M8Dev9 {(7*60)a + y = 30,  (3*60)a + y = 255}

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
                            bRenderStaticShadowsForLODs ? CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), 2, *(RwTexture **)0x978DB4, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, it->colour.r, it->colour.g, it->colour.b, 15.0f, 1.0f, fCoronaFarClip, false, 0.0f) : nullptr;
                        }
                        else
                        {
                            static float blinking = 1.0f;
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


void CLODLightManager::VC::DrawDistanceChanger()
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
        if (injector::address_manager::singleton().IsVC())
        {
            if (injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0)
            {
                CLODLightManager::VC::Init();
            }
        }
    }
    return TRUE;
}