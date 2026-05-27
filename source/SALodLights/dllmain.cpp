#define NOMINMAX
#include "stdafx.h"
#include <ranges>

import ComVars;
import LamppostInfo;
import LODLights;
import Game;
import Timer;
import ModelInfo;
import Camera;
import Clock;
import Entity;
import Sprite;
import Misc;
import SearchLightCone;
import Timecycle;
import ModelInfo;
import PointLights;

using RwV3D = RwV3d;

void RegisterCustomCoronas()
{
    unsigned short nModelID = 65534;
    auto itEnd = FileContent.upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = FileContent.lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_Lampposts.push_back(CLamppostInfo(it->second.vecLocalPos, { 0.0f, 0.0f, 0.0f }, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f));
}

void RegisterLamppost(CEntity* pObj)
{
    unsigned short nModelID = pObj->GetModelIndex();
    CMatrix dummyMatrix;
    CSimpleTransform& objTransform = pObj->GetTransform();

    if (objTransform.m_translate.x == 0.0f && objTransform.m_translate.y == 0.0f)
        return;

    dummyMatrix.SetTranslateOnly(objTransform.m_translate.x, objTransform.m_translate.y, objTransform.m_translate.z);
    dummyMatrix.SetRotateZOnly(objTransform.m_heading);

    auto foundElements = FileContent | std::views::filter([minKey = PackKey(nModelID, 0), maxKey = PackKey(nModelID, 0xFFFF)](const auto& kv)
    {
        return kv.first >= minKey && kv.first <= maxKey;
    });

    auto ms_modelInfoPtrs = *CModelInfo::pp_modelInfoPtrs;
    auto modelInfo = ms_modelInfoPtrs[nModelID];

    // Get bounding box height from model info
    float objectHeight = 0.0f;
    if (modelInfo && modelInfo->m_pColModel)
    {
        // Get height from bounding box
        objectHeight = modelInfo->m_pColModel->m_Box.max.z - modelInfo->m_pColModel->m_Box.min.z;
        //objectHeight = colModel->m_Sphere.sphere.m_fRadius * 2.0f;
    }

    for (const auto& [key, data] : foundElements)
    {
        CVector worldPos = dummyMatrix * data.vecLocalPos;

        m_Lampposts.push_back(CLamppostInfo(
            worldPos,
            data.vecLocalPos,
            data.colour,
            data.fCustomSizeMult,
            data.nCoronaShowMode,
            data.nNoDistance,
            data.nDrawSearchlight ? static_cast<int>(objectHeight) : 0,
            pObj->GetTransform().m_heading,
            std::min(data.fObjectDrawDistance, modelInfo->m_fDrawDistance)
        ));
    }
}

CEntity* PossiblyAddThisEntity(CEntity* pEntity)
{
    if (pEntity && m_bCatchLamppostsNow && IsModelALamppost(pEntity->GetModelIndex()))
        RegisterLamppost(pEntity);
    return pEntity;
}

namespace CFileLoader
{
    SafetyHookInline shLoadObjectInstance = {};
    CEntity* __cdecl LoadObjectInstance(void* inst)
    {
        return PossiblyAddThisEntity(shLoadObjectInstance.unsafe_ccall<CEntity*>(inst));
    }
}

void ApplyMemoryPatches()
{
    auto pattern = hook::pattern("E8 ? ? ? ? 6A ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8");
    static auto CFileLoaderLoadLevelHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        LoadDatFile();
        if (!m_bCatchLamppostsNow)
        {
            bRenderLodLights = 0;
            bRenderSearchlightEffects = 0;
        }
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 6A ? 50 B9");
    static auto RenderEffectsHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bRenderLodLights)
            CLODLights::RenderBuffered();
    });

    pattern = hook::pattern("E8 ? ? ? ? 80 3D ? ? ? ? ? 74 ? 6A");
    static auto RenderEffectsHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bRenderSearchlightEffects)
            RenderAllSearchLights();
    });

    if (bRenderLodLights)
    {
        auto pattern = hook::pattern("E8 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 6A ? 6A ? 6A ? E8 ? ? ? ? E8 ? ? ? ? E8");
        static auto CGameInit2Hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            CLODLights::Init(numCoronas);
        });

        pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 56");
        static auto CGameInitialiseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            RegisterCustomCoronas();
            m_bCatchLamppostsNow = false;
            m_Lampposts.shrink_to_fit();
            FileContent.clear();
        });

        pattern = hook::pattern("E8 ? ? ? ? 8B F0 8A 44 24");
        CFileLoader::shLoadObjectInstance = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CFileLoader::LoadObjectInstance);

        pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 85 C0 74");
        static auto CGameProcessHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            CLODLights::RegisterLODLights();
        });
    }

    if (fStaticSunSize)
    {
        injector::WriteMemory(0x6FC656, &fStaticSunSize, true);
        injector::WriteMemory(0x6FC6E2, &fStaticSunSize, true);
    }

    injector::WriteMemory(0x6FC051 + 0x2, 0x7080 * 0xA, true); // sun reflection
    injector::WriteMemory<float>(0x49DCF4, 550.0f, true); //Traffic lights corona draw distance

    //{
    //    fNewFarClip = 2000.0f;
    //    injector::WriteMemory(0x40C524, &fNewFarClip, true);
    //    injector::WriteMemory(0x553F79, &fNewFarClip, true);
    //    injector::WriteMemory(0x5556A7, &fNewFarClip, true);
    //    injector::WriteMemory(0x732515, &fNewFarClip, true);
    //
    //    injector::WriteMemory(0x53D532, &fNewFarClip, true);
    //    injector::WriteMemory(0x53DC7B, &fNewFarClip, true);
    //    injector::WriteMemory(0x53DCB8, &fNewFarClip, true);
    //    injector::WriteMemory(0x53EA95, &fNewFarClip, true);
    //}
}

void GetMemoryAddresses()
{
    CModelInfo::GetModelInfo = (decltype(CModelInfo::GetModelInfo))0x4C5940;
    CTimer::m_snTimeInMillisecondsPauseMode.SetAddress((unsigned int*)0xB7CB7C);
    CTimer::ms_fTimeStep.SetAddress((float*)0xB7CB5C);
    TheCamera.SetAddress((CCamera*)0xB6F028);

    CWeather::Foggyness.SetAddress((float*)0xC81300);
    gpCoronaTexture = (RwTexture**)0xC3E000;

    CSprite::CalcScreenCoorsMaxMin = (decltype(CSprite::CalcScreenCoorsMaxMin))0x70CE30;
    CSprite::FlushSpriteBuffer = (decltype(CSprite::FlushSpriteBuffer))0x70CF20;
    CSprite::RenderOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderOneXLUSprite_Rotate_Aspect))0x70D490;
    CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect))0x70E780;

    Scene.SetAddress((CScene*)0xC17038);
    RwEngineInstance.SetAddress((RwGlobals**)0xC97B24);

    RwIm3DTransform = (decltype(RwIm3DTransform))0x7EF450;
    RwIm3DRenderIndexedPrimitive = (decltype(RwIm3DRenderIndexedPrimitive))0x7EF550;
    RwIm3DEnd = (decltype(RwIm3DEnd))0x7EF520;

    CGame::currArea.SetAddress((int*)0xB72914);

    CWorld::FindGroundZFor3DCoordCRGO = (decltype(CWorld::FindGroundZFor3DCoordCRGO))0x5696C0;
    CClock::GetIsTimeInRange = (decltype(CClock::GetIsTimeInRange))0x52CEE0;

    CClock::ms_nGameClockHours.SetAddress((uint8_t*)0xB70153);
    CClock::ms_nGameClockMinutes.SetAddress((uint8_t*)0xB70152);

    CTimeCycle::m_fCurrentFarClip.SetAddress((float*)0xB7C4F0);

    CModelInfo::pp_modelInfoPtrs = (CBaseModelInfo***)(0x49DB82 + 3);

    CModelInfo::Get2dEffect = (decltype(CModelInfo::Get2dEffect))0x4C4C70;

    CRenderer::ms_lodDistScale.SetAddress((float*)0x8CD800);

    CShadows::StoreStaticShadow = (decltype(CShadows::StoreStaticShadow))0x70BA00;

    CPointLights::AddLightWithEntity = (decltype(CPointLights::AddLightWithEntity))0x7000E0;
}

void Init()
{
    CIniReader iniReader("");
    bRenderLodLights = iniReader.ReadInteger("LodLights", "RenderLodLights", 1) != 0;
    numCoronas = iniReader.ReadInteger("LodLights", "MaxNumberOfLodLights", 25000);
    fCoronaRadiusMultiplier = iniReader.ReadFloat("LodLights", "CoronaRadiusMultiplier", 1.0f);
    bSlightlyIncreaseRadiusWithDistance = iniReader.ReadInteger("LodLights", "SlightlyIncreaseRadiusWithDistance", 1) != 0;
    if (iniReader.ReadString("LodLights", "CoronaFarClip", "auto") != "auto")
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

    GetMemoryAddresses();
    ApplyMemoryPatches();
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        Init();
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
