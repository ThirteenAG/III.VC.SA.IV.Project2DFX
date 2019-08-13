#include "..\includes\stdafx.h"
#include "..\includes\CLODLightManager.h"
#include "..\includes\CLODLights.h"
#include "CSearchlights.h"

int numCoronas;
CCamera& TheCamera = *(CCamera*)0xB6F028;
RwCamera*& Camera = *(RwCamera**)0xC1703C;
int& CTimer::m_snTimeInMillisecondsPauseMode = *(int*)0xB7CB7C;
float& CTimer::ms_fTimeStep = *(float*)0xB7CB5C;

char* CLODLightManager::SA::CurrentTimeHours = (char*)0xB70153;
char* CLODLightManager::SA::CurrentTimeMinutes = (char*)0xB70152;
float** CLODLightManager::SA::fCurrentFarClip = (float**)0x53EA95;
int* CLODLightManager::SA::ActiveInterior = (int*)0xB72914;
char(__cdecl *CLODLightManager::SA::GetIsTimeInRange)(char hourA, char hourB) = (char(__cdecl *)(char, char)) 0x52CEE0;
int* (__cdecl *CLODLightManager::SA::GetModelInfo)(const char*, int*) = (int*(__cdecl *)(const char*, int*)) 0x4C5940;
void(__cdecl *const CLODLightManager::SA::CShadowsUpdateStaticShadows)() = (decltype(CShadowsUpdateStaticShadows))0x707F40;
bool(__cdecl *const CLODLightManager::SA::CShadowsStoreStaticShadow)(unsigned int id, unsigned char type, RwTexture *particle, CVector *pos, float x1, float y1, float x2, float y2, short alpha, unsigned char red, unsigned char green, unsigned char blue, float, float, float drawdist, bool lifetime, float updist) = (decltype(CShadowsStoreStaticShadow))0x70BA00;
float(__cdecl *CLODLightManager::SA::FindGroundZFor3DCoord)(float x, float y, float z, BOOL *pCollisionResult, CEntity **pGroundObject) = (float(__cdecl *)(float, float, float, BOOL *, CEntity **)) 0x5696C0;
RwV3D *(__cdecl *CLODLightManager::SA::TransformPoint)(RwV3D *outPoint, CMatrix *m, RwV3D *point) = (RwV3D *(__cdecl *)(RwV3D *, CMatrix *, RwV3D *)) 0x59C890;
bool(__thiscall *CLODLightManager::SA::CCameraIsSphereVisible)(int *camera, RwV3d *origin, float radius) = (bool(__thiscall *)(int *, RwV3d *, float)) 0x00420D40;
void(__cdecl *CLODLightManager::SA::RegisterCorona)(int id, int *vehicle, char r, char g, char b, char alpha, RwV3d *pos, float radius, float farClip, char type, char flareType, char enableReflection, char checkObstacles, int notUsed, float normalAngle, char longDistance, float nearClip, char someFadeFlag, float flashInertia, char onlyFromBelow, char flag) = (void(__cdecl *)(int, int *, char, char, char, char, RwV3d *, float, float, char, char, char, char, int, float, char, float, char, float, char, char)) 0x6FC580;
void(__fastcall *CLODLightManager::SA::CVectorNormalize)(RwV3D *in) = (void(__fastcall *)(RwV3D *in)) 0x59C910;
RwV3D *(__cdecl *CLODLightManager::SA::CrossProduct)(RwV3D *out, RwV3D *a, RwV3D *b) = (RwV3D *(__cdecl *)(RwV3D *out, RwV3D *a, RwV3D *b)) 0x59C730;
int(__cdecl *CLODLightManager::SA::RwIm3DTransform)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags) = (int(__cdecl *)(RxObjSpace3dVertex *pVerts, unsigned int numVerts, RwMatrix *ltm, unsigned int flags)) 0x7EF450;
int(__cdecl *CLODLightManager::SA::RwIm3DRenderIndexedPrimitive)(int primType, short *indices, int numIndices) = (int(__cdecl *)(int primType, short *indices, int numIndices)) 0x7EF550;
int(__cdecl *CLODLightManager::SA::RwIm3DEnd)() = (int(__cdecl *)()) 0x7EF520;
C2dfx *(__fastcall *CLODLightManager::SA::Get2dfx)(CBaseModelInfo *model, int edx0, int number) = (C2dfx *(__fastcall *)(CBaseModelInfo *, int, int)) 0x4C4C70;
void(__cdecl *CLODLightManager::SA::drawSpotLight)(int coronaIndex, float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float TargetRadius, float intensity, char flag1, char drawShadow, RwV3D *pVec1, RwV3D *pVec2, RwV3D *pVec3, char unkTrue, float BaseRadius) = (void(cdecl *)(int, float, float, float, float, float, float, float, float, char, char, RwV3D *, RwV3D *, RwV3D *, char, float)) 0x6C58E0;
bool(__thiscall *CLODLightManager::SA::CObjectIsDamaged)(CObject *pclObject) = (bool(__thiscall *)(CObject *))0x0046A2F0;

RwTexture* CLODLightManager::SA::gpCustomCoronaTexture = nullptr;
RwImage*(__cdecl *const RtPNGImageReadSA)(const char* imageName) = (decltype(RtPNGImageReadSA))0x7CF9B0;
RwImage*(__cdecl *const RwImageFindRasterFormatSA)(RwImage* ipImage, int nRasterType, int* npWidth, int* npHeight, int* npDepth, int* npFormat) = (decltype(RwImageFindRasterFormatSA))0x8042C0;
RwRaster*(__cdecl *const RwRasterCreateSA)(int width, int height, int depth, int flags) = (decltype(RwRasterCreateSA))0x7FB230;
RwRaster*(__cdecl *const RwRasterSetFromImageSA)(RwRaster* raster, RwImage* image) = (decltype(RwRasterSetFromImageSA))0x804290;
RwTexture*(__cdecl *const RwTextureCreateSA)(RwRaster* raster) = (decltype(RwTextureCreateSA))0x7F37C0;
bool*(__cdecl *const RwImageDestroySA)(RwImage* image) = (decltype(RwImageDestroySA))0x802740;
RwTexture* CPNGFileReadFromFile(const char* pFileName)
{
    RwTexture*      pTexture = nullptr;

    if (RwImage* pImage = RtPNGImageReadSA(pFileName))
    {
        int     dwWidth, dwHeight, dwDepth, dwFlags;
        RwImageFindRasterFormatSA(pImage, 4/*rwRASTERTYPETEXTURE*/, &dwWidth, &dwHeight, &dwDepth, &dwFlags);
        if (RwRaster* pRaster = RwRasterCreateSA(dwWidth, dwHeight, dwDepth, dwFlags))
        {
            RwRasterSetFromImageSA(pRaster, pImage);

            pTexture = RwTextureCreateSA(pRaster);
        }
        RwImageDestroySA(pImage);
    }
    return pTexture;
}

void RwRenderStateSetSA(RwRenderState nState, void *pParam)
{
    RwEngineInstance = *(void**)0xC97B24;
    RwRenderStateSet(nState, pParam);
}
void(*_RwRenderStateSet)(RwRenderState nState, void *pParam) = &RwRenderStateSetSA;

CVector* GetCamPos()
{
    return &TheCamera.GetCoords();
}

void CLODLightManager::SA::Init()
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
    static auto tex = iniReader.ReadString("LodLights", "CustomCoronaTexturePath", ".\\corona.png");
    szCustomCoronaTexturePath = const_cast<char*>(tex.c_str());

    bRenderStaticShadowsForLODs = iniReader.ReadInteger("StaticShadows", "RenderStaticShadowsForLODs", 0) != 0;
    bIncreasePedsCarsShadowsDrawDistance = iniReader.ReadInteger("StaticShadows", "IncreaseCarsShadowsDrawDistance", 0) != 0;
    fStaticShadowsIntensity = iniReader.ReadFloat("StaticShadows", "StaticShadowsIntensity", 0.0f);
    fStaticShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "StaticShadowsDrawDistance", 0.0f);
    fTrafficLightsShadowsIntensity = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsIntensity", 0.0f);
    fTrafficLightsShadowsDrawDistance = iniReader.ReadFloat("StaticShadows", "TrafficLightsShadowsDrawDistance", 0.0f);

    bRenderSearchlightEffects = iniReader.ReadInteger("SearchLights", "RenderSearchlightEffects", 1) != 0;
    bRenderOnlyDuringFoggyWeather = iniReader.ReadInteger("SearchLights", "RenderOnlyDuringFoggyWeather", 0) != 0;
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

    fMaxDrawDistanceForNormalObjects = iniReader.ReadFloat("IDETweaker", "MaxDrawDistanceForNormalObjects", 300.0f);
    fTimedObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "TimedObjectsDrawDistance", 0.0f);
    fNeonsDrawDistance = iniReader.ReadFloat("IDETweaker", "NeonsDrawDistance", 0.0f);
    fLODObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "LODObjectsDrawDistance", 0.0f);
    fGenericObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "GenericObjectsDrawDistance", 0.0f);
    fAllNormalObjectsDrawDistance = iniReader.ReadFloat("IDETweaker", "AllNormalObjectsDrawDistance", 0.0f);
    fVegetationDrawDistance = iniReader.ReadFloat("IDETweaker", "VegetationDrawDistance", 0.0f);
    bLoadAllBinaryIPLs = iniReader.ReadInteger("IDETweaker", "LoadAllBinaryIPLs", 0) != 0;
    bPreloadLODs = iniReader.ReadInteger("IDETweaker", "PreloadLODs", 0) != 0;
    bFestiveLights = iniReader.ReadInteger("Misc", "FestiveLights", 1) != 0;
    bFestiveLightsAlways = iniReader.ReadInteger("Misc", "bFestiveLightsAlways", 0) != 0;

    ApplyMemoryPatches();
}

template<uintptr_t addr>
void CCoronasRegisterFestiveCoronaForEntity()
{
    using func_hook = injector::function_hooker<addr, void(unsigned int nID, CEntity* entity, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, char flare, char enableReflection, char checkObstacles, int notUsed, float angle, char longDistance, float nearClip, char fadeState, float fadeSpeed, char onlyFromBelow, char reflectionDelay)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type RegisterCorona, unsigned int nID, CEntity* entity, unsigned char R, unsigned char G, unsigned char B, unsigned char A, const CVector& Position, float Size, float Range, RwTexture* pTex, char flare, char enableReflection, char checkObstacles, int notUsed, float angle, char longDistance, float nearClip, char fadeState, float fadeSpeed, char onlyFromBelow, char reflectionDelay)
    {
        auto it = CLODLights::FestiveLights.find(nID);
        if (it != CLODLights::FestiveLights.end())
        {
            RegisterCorona(nID, entity, it->second.r, it->second.g, it->second.b, A, Position, Size, Range, pTex, flare, enableReflection, checkObstacles, notUsed, angle, longDistance, nearClip, fadeState, fadeSpeed, onlyFromBelow, reflectionDelay);
        }
        else
        {
            CLODLights::FestiveLights[nID] = CRGBA(random(0, 255), random(0, 255), random(0, 255), 0);
            RegisterCorona(nID, entity, R, G, B, A, Position, Size, Range, pTex, flare, enableReflection, checkObstacles, notUsed, angle, longDistance, nearClip, fadeState, fadeSpeed, onlyFromBelow, reflectionDelay);
        }
    });
}

void CLODLightManager::SA::ApplyMemoryPatches()
{
    struct asmInit
    {
        void operator()(injector::reg_pack& regs)
        {
            injector::cstd<void(const char *a1)>::call(0x5B3680, (char*)0x869B30);
            CLODLightManager::SA::LoadDatFile();
        }
    }; injector::MakeInline<asmInit>(0x5B9253, 0x5B9258);

    struct Render
    {
        void operator()(injector::reg_pack&)
        {
            if (bRenderLodLights)
                CLODLights::RenderBuffered();

            if (bRenderSearchlightEffects)
                CSearchlights::RenderSearchLightsSA();
        }
    }; injector::MakeInline<Render>(0x53E184);

    if (bRenderLodLights)
    {
        //injector::MakeNOP(0x6FAD78, 10, true); //disable ambientBrightness change
        //injector::MakeNOP(0x6FAD84, 6, true);
        //injector::MakeNOP(0x6FAE13, 10, true);
        //injector::MakeNOP(0x6FAE1F, 6, true);

        CLODLights::Inject();

        struct asmEnd
        {
            void operator()(injector::reg_pack& regs)
            {
                injector::cstd<void(const char *a1, const char *a2)>::call(0x53DED0, (char*)0x869B30, (char*)0x863A80);
                CLODLightManager::SA::RegisterCustomCoronas();
                m_bCatchLamppostsNow = false;
                m_pLampposts->shrink_to_fit();
                pFileContent->clear();
            }
        }; injector::MakeInline<asmEnd>(0x53BCBC);

        struct LoadObjectInstHook
        {
            void operator()(injector::reg_pack& regs)
            {
                uint32_t _ecx = regs.ecx;
                __asm mov ecx, _ecx
                __asm mov fs : [00000000], ecx

                CLODLightManager::SA::PossiblyAddThisEntity((CEntity*)regs.eax);
            }
        }; injector::MakeInline<LoadObjectInstHook>(0x538432, 0x538439);

        injector::MakeCALL(0x53C131, RegisterLODLights, true);
    }

    if (bEnableDrawDistanceChanger)
    {
        injector::MakeCALL(0x53EBE4, DrawDistanceChanger, true);

        injector::WriteMemory(0x40C524, &fNewFarClip, true);
        injector::WriteMemory(0x553F79, &fNewFarClip, true);
        injector::WriteMemory(0x5556A7, &fNewFarClip, true);
        injector::WriteMemory(0x732515, &fNewFarClip, true);

        injector::WriteMemory(0x53D532, &fNewFarClip, true);
        injector::WriteMemory(0x53DC7B, &fNewFarClip, true);
        injector::WriteMemory(0x53DCB8, &fNewFarClip, true);
        injector::WriteMemory(0x53EA95, &fNewFarClip, true);
    }

    if (fStaticSunSize)
    {
        injector::WriteMemory(0x6FC656, &fStaticSunSize, true);
        injector::WriteMemory(0x6FC6E2, &fStaticSunSize, true);
    }

    /*if (nSmoothEffect)
    {
        nSmoothEffect = 1;
        injector::WriteMemory(0x6C60F2 + 1, 0xC4D994 + 0x24, true);
        injector::WriteMemory(0x6C6222 + 3, 0xC4D970 + 0x24, true);
    }*/

    //CPatch::RedirectCall(0x53E0BE, StoreCustomStaticShadows);

    injector::WriteMemory(0x6FC051 + 0x2, 0x7080 * 0xA, true); // sun reflection
    injector::WriteMemory<float>(0x49DCF4, 550.0f, true); //Traffic lights corona draw distance

    struct LamppostsCoronaFarclpHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax &= 1;

            *(float*)(regs.esi + 0x14) = 3000.0f;
        }
    }; injector::MakeInline<LamppostsCoronaFarclpHook>(0x6FCFBA);


    if (fStaticShadowsDrawDistance)
    {
        injector::WriteMemory<float>(0x6FD3A6, fStaticShadowsDrawDistance, true); // Lampposts shadows draw distance
        injector::WriteMemory<float>(0x6FD44F, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x455EF3, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x4561B3, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x49DF7A, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x53B5E2, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x70C88B, fStaticShadowsDrawDistance, true);
        injector::WriteMemory<float>(0x70C9F4, fStaticShadowsDrawDistance, true);
    }

    if (fStaticShadowsIntensity)
    {
        fStaticShadowsIntensity *= 0.00390625f;
        injector::WriteMemory(0x6FD13C, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD16E, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD17C, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD1CE, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD2C0, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD301, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD30F, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD3BC, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD3DA, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
        injector::WriteMemory(0x6FD3F8, &fStaticShadowsIntensity, true); // = 0x859AA0 + 0x0->fmul    ds : flt_859AA0
    }

    if (fTimedObjectsDrawDistance || fNeonsDrawDistance)
    {
        struct IncreaseDrawDistanceForTimedObjectsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edi = regs.eax;
                *(float*)(regs.esi + 0x18) = *(float*)&regs.ecx;

                if ((fNeonsDrawDistance && strstr((char*)(regs.esp + 0x28), "neon") != NULL) || fTimedObjectsDrawDistance)
                {
                    if (fTimedObjectsDrawDistance <= 10.0f)
                        *(float*)(regs.esi + 0x18) *= fTimedObjectsDrawDistance;
                    else
                        *(float*)(regs.esi + 0x18) = fTimedObjectsDrawDistance;
                }
            }
        }; injector::MakeInline<IncreaseDrawDistanceForTimedObjectsHook>(0x5B3F4C);
    }

    if (fLODObjectsDrawDistance || fGenericObjectsDrawDistance || fAllNormalObjectsDrawDistance || fVegetationDrawDistance)
    {
        struct IncreaseDrawDistanceForObjectsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.edx = *(float*)(regs.esp + 0xC);
                regs.esi = regs.eax;

                auto modelID = regs.ecx;
                float drawDist = *(float*)(regs.esp + 0xC);

                if (fVegetationDrawDistance)
                {
                    if (modelID >= 615 && modelID <= 792 && drawDist <= 300.0f)
                    {
                        if (fVegetationDrawDistance <= 10.0f)
                            drawDist *= fVegetationDrawDistance;
                        else
                            drawDist = fVegetationDrawDistance;

                        if (drawDist > fMaxDrawDistanceForNormalObjects)
                            fMaxDrawDistanceForNormalObjects = drawDist;

                        *(float*)&regs.edx = drawDist;
                        return;
                    }
                }

                if (drawDist > 300.0f)
                {
                    if (fLODObjectsDrawDistance)
                    {
                        if (fLODObjectsDrawDistance <= 10.0f)
                            drawDist *= fLODObjectsDrawDistance;
                        else if (fLODObjectsDrawDistance > drawDist)
                            drawDist = fLODObjectsDrawDistance;
                    }
                }
                else
                {
                    if (fGenericObjectsDrawDistance)
                    {
                        if (modelID >= 615 && modelID <= 1572)
                        {
                            if (fGenericObjectsDrawDistance <= 10.0f)
                                drawDist *= fGenericObjectsDrawDistance;
                            else
                                drawDist = fGenericObjectsDrawDistance;
                        }
                        else
                        {
                            if (fAllNormalObjectsDrawDistance)
                            {
                                if (fAllNormalObjectsDrawDistance <= 10.0f)
                                    drawDist *= fAllNormalObjectsDrawDistance;
                                else
                                    drawDist = fAllNormalObjectsDrawDistance;
                            }
                        }
                    }
                    else
                    {
                        if (fAllNormalObjectsDrawDistance)
                        {
                            if (fAllNormalObjectsDrawDistance <= 10.0f)
                                drawDist *= fAllNormalObjectsDrawDistance;
                            else
                                drawDist = fAllNormalObjectsDrawDistance;
                        }
                    }
                    if (drawDist > fMaxDrawDistanceForNormalObjects)
                        fMaxDrawDistanceForNormalObjects = drawDist;
                }
                *(float*)&regs.edx = drawDist;
            }
        }; injector::MakeInline<IncreaseDrawDistanceForObjectsHook>(0x5B3D9F, 0x5B3D9F + 6);

        if (fGenericObjectsDrawDistance || fAllNormalObjectsDrawDistance || fVegetationDrawDistance)
        {
            injector::WriteMemory(0x554230 + 0x3FA + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fsub    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x192 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x1B8 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x1DB + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x24E + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x258 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x262 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x314 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x31E + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x328 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x382 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x39A + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x554FE0 + 0x3A8 + 0x2, &fMaxDrawDistanceForNormalObjects, true); //fmul    ds:flt_858FD8
            injector::WriteMemory(0x5B51E0 + 0x09A + 0x2, &fMaxDrawDistanceForNormalObjects, true); // fcomp   ds:flt_858FD8
            injector::WriteMemory(0x554230 + 0x3B6 + 0x2, &fMaxDrawDistanceForNormalObjects, true); // fcomp   ds:flt_858FD8
            injector::WriteMemory(0x554230 + 0x3D0 + 0x2, &fMaxDrawDistanceForNormalObjects, true); // fld     ds:flt_858FD8
            injector::WriteMemory(0x554230 + 0x3FA + 0x2, &fMaxDrawDistanceForNormalObjects, true); // fsub    ds:flt_858FD8
        }
    }

    if (bLoadAllBinaryIPLs)
    {
        struct LoadAllBinaryIPLs
        {
            void operator()(injector::reg_pack&)
            {
                static auto CIplStoreLoad = (char *(__cdecl *)()) 0x5D54A0;
                CIplStoreLoad();

                static auto IplFilePoolLocate = (int(__cdecl *)(const char *name)) 0x404AC0;
                static auto CIplStoreRequestIplAndIgnore = (char *(__cdecl *)(int a1)) 0x405850;

                injector::address_manager::singleton().IsHoodlum() ?
                injector::WriteMemory<char>(0x015651C1 + 3, 0, true) :
                injector::WriteMemory<char>(0x405881 + 3, 0, true);

                static std::vector<std::string> IPLStreamNames = { "LAE_STREAM0", "LAE_STREAM1", "LAE_STREAM2", "LAE_STREAM3", "LAE_STREAM4", "LAE_STREAM5",
                                                                   "LAE2_STREAM0", "LAE2_STREAM1", "LAE2_STREAM2", "LAE2_STREAM3", "LAE2_STREAM4", "LAE2_STREAM5", "LAE2_STREAM6", "LAHILLS_STREAM0",
                                                                   "LAHILLS_STREAM1", "LAHILLS_STREAM2", "LAHILLS_STREAM3", "LAHILLS_STREAM4", "LAN_STREAM0", "LAN_STREAM1", "LAN_STREAM2", "LAN2_STREAM0",
                                                                   "LAN2_STREAM1", "LAN2_STREAM2", "LAN2_STREAM3", "LAS_STREAM0", "LAS_STREAM1", "LAS_STREAM2", "LAS_STREAM3", "LAS_STREAM4", "LAS_STREAM5",
                                                                   "LAS2_STREAM0", "LAS2_STREAM1", "LAS2_STREAM2", "LAS2_STREAM3", "LAS2_STREAM4", "LAW_STREAM0", "LAW_STREAM1", "LAW_STREAM2", "LAW_STREAM3", "LAW_STREAM4",
                                                                   "LAW_STREAM5", "LAW2_STREAM0", "LAW2_STREAM1", "LAW2_STREAM2", "LAW2_STREAM3", "LAW2_STREAM4", "LAWN_STREAM0", "LAWN_STREAM1", "LAWN_STREAM2", "LAWN_STREAM3",
                                                                   "COUNTN2_STREAM0", "COUNTN2_STREAM1", "COUNTN2_STREAM2", "COUNTN2_STREAM3", "COUNTN2_STREAM4", "COUNTN2_STREAM5", "COUNTN2_STREAM6", "COUNTN2_STREAM7", "COUNTN2_STREAM8",
                                                                   "COUNTRYE_STREAM0", "COUNTRYE_STREAM1", "COUNTRYE_STREAM2", "COUNTRYE_STREAM3", "COUNTRYE_STREAM4", "COUNTRYE_STREAM5", "COUNTRYE_STREAM6", "COUNTRYE_STREAM7", "COUNTRYE_STREAM8",
                                                                   "COUNTRYE_STREAM9", "COUNTRYN_STREAM0", "COUNTRYN_STREAM1", "COUNTRYN_STREAM2", "COUNTRYN_STREAM3", "COUNTRYS_STREAM0", "COUNTRYS_STREAM1", "COUNTRYS_STREAM2", "COUNTRYS_STREAM3", "COUNTRYS_STREAM4",
                                                                   "COUNTRYW_STREAM0", "COUNTRYW_STREAM1", "COUNTRYW_STREAM2", "COUNTRYW_STREAM3", "COUNTRYW_STREAM4", "COUNTRYW_STREAM5", "COUNTRYW_STREAM6", "COUNTRYW_STREAM7", "COUNTRYW_STREAM8", "SFE_STREAM0",
                                                                   "SFE_STREAM1", "SFE_STREAM2", "SFE_STREAM3", "SFN_STREAM0", "SFN_STREAM1", "SFN_STREAM2", "SFS_STREAM0", "SFS_STREAM1", "SFS_STREAM2", "SFS_STREAM3", "SFS_STREAM4", "SFS_STREAM5", "SFS_STREAM6",
                                                                   "SFS_STREAM7", "SFS_STREAM8", "SFSE_STREAM0", "SFSE_STREAM1", "SFSE_STREAM2", "SFSE_STREAM3", "SFSE_STREAM4", "SFSE_STREAM5", "SFSE_STREAM6", "SFW_STREAM0", "SFW_STREAM1", "SFW_STREAM2", "SFW_STREAM3",
                                                                   "SFW_STREAM4", "SFW_STREAM5", "VEGASE_STREAM0", "VEGASE_STREAM1", "VEGASE_STREAM2", "VEGASE_STREAM3", "VEGASE_STREAM4", "VEGASE_STREAM5", "VEGASE_STREAM6", "VEGASE_STREAM7", "VEGASE_STREAM8",
                                                                   "VEGASN_STREAM0", "VEGASN_STREAM1", "VEGASN_STREAM2", "VEGASN_STREAM3", "VEGASN_STREAM4", "VEGASN_STREAM5", "VEGASN_STREAM6", "VEGASN_STREAM7", "VEGASN_STREAM8", "VEGASS_STREAM0", "VEGASS_STREAM1",
                                                                   "VEGASS_STREAM2", "VEGASS_STREAM3", "VEGASS_STREAM4", "VEGASS_STREAM5", "VEGASW_STREAM0", "VEGASW_STREAM1", "VEGASW_STREAM2", "VEGASW_STREAM3", "VEGASW_STREAM4", "VEGASW_STREAM5", "VEGASW_STREAM6",
                                                                   "VEGASW_STREAM7", "VEGASW_STREAM8", "VEGASW_STREAM9"
                                                                 };

                for (auto it = IPLStreamNames.cbegin(); it != IPLStreamNames.cend(); it++)
                {
                    CIplStoreRequestIplAndIgnore(IplFilePoolLocate(it->c_str()));
                }

                injector::address_manager::singleton().IsHoodlum() ?
                injector::WriteMemory<char>(0x015651C1 + 3, 1, true) :
                injector::WriteMemory<char>(0x405881 + 3, 1, true);
            }
        }; injector::MakeInline<LoadAllBinaryIPLs>(0x5D19A4);
    }


    if (bPreloadLODs)
    {
        static std::vector<void*> lods; // CEntity*
        static auto RequestModel = injector::cstd<void(int, int)>::call<0x4087E0>;
        static auto LoadAllRequestedModels = injector::cstd<void(bool)>::call<0x40EA10>;

        using h53BCAB = injector::function_hooker<0x53BCAB, void()>;

        injector::MakeInline<0x5B5295, 0x5B5295 + 8>([](injector::reg_pack& regs)
        {
            regs.ecx = *(uint16_t*)(regs.eax + 0x22);  // mah let's make it unsigned
            regs.edx = *(uint16_t*)(regs.esi + 0x22);
            lods.push_back((void*)regs.eax);
        });

        injector::make_static_hook<h53BCAB>([](h53BCAB::func_type AfterInit2)
        {
            // Put the id of the lods in another container
            std::vector<uint16_t> lods_id(lods.size());
            std::transform(lods.begin(), lods.end(), lods_id.begin(), [](void* entity)
            {
                return *(uint16_t*)((uintptr_t)(entity)+0x22);
            });

            // Load all lod models
            std::for_each(lods_id.begin(), std::unique(lods_id.begin(), lods_id.end()), [](uint16_t id) { RequestModel(id, 2); }); // STREAMING_CANNOT_DELETE
            LoadAllRequestedModels(false);

            // Instantiate all lod entities RwObject
            if (false)
                std::for_each(lods.begin(), lods.end(), [](void* entity)
            {
                auto rwObject = *(void**)((uintptr_t)(entity)+0x18);
                if (rwObject == nullptr)
                    injector::thiscall<void(void*)>::vtbl<7>(entity);
            });

            return AfterInit2();
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
            CCoronasRegisterFestiveCoronaForEntity<(0x6FCA80)>();
            CCoronasRegisterFestiveCoronaForEntity<(0x6FD02E)>();
            CCoronasRegisterFestiveCoronaForEntity<(0x5363A8)>();
            CCoronasRegisterFestiveCoronaForEntity<(0x536511)>();
        }
    }
}

void CLODLightManager::SA::RegisterCustomCoronas()
{
    unsigned short      nModelID = 65534;

    gpCustomCoronaTexture = CPNGFileReadFromFile(szCustomCoronaTexturePath);
    if (gpCustomCoronaTexture)
        injector::WriteMemory<unsigned int>(0xC3E004, *(unsigned int*)gpCustomCoronaTexture);

    auto    itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f));
}

CEntity * CLODLightManager::SA::PossiblyAddThisEntity(CEntity * pEntity)
{
    if (m_bCatchLamppostsNow && IsModelALamppost(pEntity->GetModelIndex()))
        RegisterLamppost(pEntity);

    // Saves some hacking
    return pEntity;
}

void CLODLightManager::SA::RegisterLamppost(CEntity * pObj)
{
    unsigned short      nModelID = pObj->GetModelIndex();
    CMatrix             dummyMatrix;
    CSimpleTransform&   objTransform = pObj->GetTransform();

    if (objTransform.m_translate.x == 0.0f && objTransform.m_translate.y == 0.0f)
        return;

    dummyMatrix.SetTranslateOnly(objTransform.m_translate.x, objTransform.m_translate.y, objTransform.m_translate.z);
    dummyMatrix.SetRotateZOnly(objTransform.m_heading);

    auto    itEnd = pFileContent->upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = pFileContent->lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_pLampposts->push_back(CLamppostInfo(dummyMatrix * it->second.vecPos, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, pObj->GetTransform().m_heading));
}

void CLODLightManager::SA::RegisterLODLights()
{
    if (GetIsTimeInRange(20, 7) && *ActiveInterior == 0)
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
                CVector*    pCamPos = &TheCamera.Cams[TheCamera.ActiveCam].Source;
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
                            CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                            bRenderStaticShadowsForLODs ? CShadowsStoreStaticShadow(reinterpret_cast<unsigned int>(&*it), SSHADT_INTENSIVE, *(RwTexture **)0xC403F4, (CVector*)&it->vecPos, 8.0f, 0.0f, 0.0f, -8.0f, bAlpha, (it->colour.r / 3), (it->colour.g / 3), (it->colour.b / 3), 15.0f, 1.0f, fCoronaFarClip, false, 0.0f) : nullptr;
                        }
                        else
                        {
                            static float blinking;
                            if (IsBlinkingNeeded(it->nCoronaShowMode))
                                blinking -= CTimer::ms_fTimeStep / 1000.0f;
                            else
                                blinking += CTimer::ms_fTimeStep / 1000.0f;

                            (blinking > 1.0f) ? blinking = 1.0f : (blinking < 0.0f) ? blinking = 0.0f : 0.0f;

                            CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, blinking * (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                        }
                    }
                    else
                    {
                        if ((it->colour.r >= 250 && it->colour.g >= 100 && it->colour.b <= 100) && ((curMin == 9 || curMin == 19 || curMin == 29 || curMin == 39 || curMin == 49 || curMin == 59))) //yellow
                        {
                            CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                        }
                        else
                        {
                            if ((abs(it->fHeading) >= (3.1415f / 6.0f) && abs(it->fHeading) <= (5.0f * 3.1415f / 6.0f)))
                            {
                                if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                                }
                                else
                                {
                                    if ((it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                                    }
                                }
                            }
                            else
                            {
                                if ((it->colour.r == 0 && it->colour.g >= 250 && it->colour.b == 0) && (((curMin >= 0 && curMin < 9) || (curMin >= 20 && curMin < 29) || (curMin >= 40 && curMin < 49)))) //red
                                {
                                    CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                                }
                                else
                                {
                                    if ((it->colour.r >= 250 && it->colour.g < 100 && it->colour.b == 0) && (((curMin > 9 && curMin < 19) || (curMin > 29 && curMin < 39) || (curMin > 49 && curMin < 59)))) //green
                                    {
                                        CLODLights::RegisterCorona(reinterpret_cast<unsigned int>(&*it), nullptr, it->colour.r, it->colour.g, it->colour.b, (bAlpha * (it->colour.a / 255.0f)), it->vecPos, (fRadius * it->fCustomSizeMult * fCoronaRadiusMultiplier), fCoronaFarClip, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
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

void CLODLightManager::SA::DrawDistanceChanger()
{
    static Fps _fps;
    fNewFarClip = 500.0f;
    static DWORD* pPlayerPed = (DWORD*)0xB6F5F0;

    if (*pPlayerPed)
    {
        if (*(BYTE*)((*pPlayerPed) + 0x2F) != 8) //[byte] Location status
        {
            if (*ActiveInterior == 0)
            {
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
        }
    }
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (injector::address_manager::singleton().IsSA() && injector::address_manager::singleton().IsUS())
        {
            if (injector::address_manager::singleton().GetMajorVersion() == 1 && injector::address_manager::singleton().GetMinorVersion() == 0)
            {
                CLODLightManager::SA::Init();
            }
        }
    }
    return TRUE;
}