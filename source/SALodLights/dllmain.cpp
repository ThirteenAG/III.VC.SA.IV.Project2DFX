#define NOMINMAX
#include "stdafx.h"
#include <ranges>
#include <deque>
#include "asi_settings/asi_settings.hpp"

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
import DistantCars;
import DistantCarRenderer;
import WaterLevel;

using RwV3D = RwV3d;

void RegisterCustomCoronas()
{
    unsigned short nModelID = 65534;
    auto itEnd = FileContent.upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = FileContent.lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_Lampposts.push_back(CLamppostInfo(it->second.vecLocalPos, { 0.0f, 0.0f, 0.0f }, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f, 0.0f, it->second.pPredicate));
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

    const size_t firstLamp = m_Lampposts.size();
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
            std::min(data.fObjectDrawDistance, modelInfo->m_fDrawDistance),
            data.pPredicate
        ));
        m_Lampposts.back().pool = data.pool;
        if (modelInfo && modelInfo->m_pColModel)
        {
            const CVector base{ dummyMatrix * CVector(data.vecLocalPos.x, data.vecLocalPos.y, modelInfo->m_pColModel->m_Box.min.z) };
            m_Lampposts.back().fPoolBaseZ = base.z;
        }
        m_Lampposts.back().poolTexture = data.poolTexture;
        if (data.fCustomSizeMult == .45f)
            m_Lampposts.back().nTrafficLightType = CTrafficLights::Type(pObj, dummyMatrix.GetForward());
    }
    // Lamp coronas reach 300 m, see LamppostsCoronaFarclpHook
    LampEntities::Attach(firstLamp, objTransform.m_translate, 300.0f, nModelID);
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

bool ExteriorView()
{
    return CGame::currArea == 0 && !*reinterpret_cast<bool*>(0xB5F851)
        && CEntryExitManager::ms_exitEnterState == 0
        && CWeather::UnderWaterness == 0.0f && *reinterpret_cast<float*>(0xC81334) == 0.0f;
}

#include "../ViewDistance.hpp"
#include "ProceduralDistance.hpp"

void ApplyMemoryPatches()
{
    auto pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? E8");
    static auto CFileLoaderLoadLevelHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        LoadDatFile();
        if (!m_bCatchLamppostsNow)
        {
            bRenderLodLights = 0;
            bRenderSearchlightEffects = 0;
        }
    });


    pattern = hook::pattern("E8 ? ? ? ? 8B F0 8A 44 24");
    CFileLoader::shLoadObjectInstance = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CFileLoader::LoadObjectInstance);

    pattern = hook::pattern("BA ? ? ? ? 57 8B FA 33 C0 81 C2 ? ? ? ? 81 FA ? ? ? ? B9 ? ? ? ? ? ? 7C ? E8");
    static auto CMovingThingsInitHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CLODLights::Init(numCoronas);

        RegisterCustomCoronas();
        m_bCatchLamppostsNow = false;
        m_Lampposts.shrink_to_fit();
        FileContent.clear();

        CMovingThings::InitDistantCarImpostors();
    });

    pattern = hook::pattern("8B 4C 24 ? 89 4F ? 89 4C 24 ? B9 ? ? ? ? 89 57 ? E8 ? ? ? ? 8A 1D");
    static auto CMovingThingsUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        ProceduralDistance::Update();
        if (bRenderLodLights)
            CLODLights::RegisterLODLights();
        if (nDistantTraffic > 0)
            CMovingThings::UpdateDistantCarImpostors();
    });

    pattern = hook::pattern("6A ? 6A ? FF 51 ? 8B 15 ? ? ? ? 6A ? 6A ? FF 52 ? 83 C4 ? E8 ? ? ? ? E8 ? ? ? ? E9");
    static auto distantCarsSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
    {
        if (nDistantTraffic > 0)
            CMovingThings::RenderDistantCarImpostors();
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 6A ? 50 B9");
    static auto CMovingThingsRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bRenderLodLights)
        {
            CLODLights::RenderBuffered();
        }

        if (bRenderSearchlightEffects)
            RenderAllSearchLights();

    });


    pattern = hook::pattern("BE ? ? ? ? 8D 4E ? E8 ? ? ? ? ? ? ? 81 C6 ? ? ? ? 81 FE ? ? ? ? 7C ? 5E C3");
    static auto CMovingThingsShutdownHook = safetyhook::create_mid(pattern.count(2).get(1).get<void*>(), [](SafetyHookContext& regs)
    {
        CMovingThings::ShutdownDistantCarImpostors();
        CLODLights::Shutdown();
    });

    auto timecyclePattern = hook::pattern("E8 ? ? ? ? 84 1D ? ? ? ? 74 ? B9 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 84 C0");
    auto cameraPattern = hook::pattern("E8 ? ? ? ? 84 C0 B9 ? ? ? ? 74 ? E8 ? ? ? ? EB ? E8");
    auto sunCorePattern = hook::pattern("D8 0D 34 25 87 00 A1 ? ? ? ? 68 00 00 70 41");
    auto sunCoronaPattern = hook::pattern("D8 0D 44 8B 85 00 A1 ? ? ? ? 68 00 00 70 41");
    if (timecyclePattern.size() == 1 && cameraPattern.size() == 1 && sunCorePattern.size() == 1 && sunCoronaPattern.size() == 1
        && *timecyclePattern.get_first<uint8_t>(-5) == 0xE8)
    {
        injector::WriteMemory(sunCorePattern.get_first<uint8_t>() + 2, &ViewDistance::sunCore, true);
        injector::WriteMemory(sunCoronaPattern.get_first<uint8_t>() + 2, &ViewDistance::sunCorona, true);
        ViewDistance::InstallCamera(cameraPattern.get_first<uint8_t>() + 14);
        ViewDistance::InstallTimecycle(timecyclePattern.get_first<uint8_t>() - 5);
    }
    else
        OutputDebugStringA("Project2DFX: view distance hooks unavailable\n");

    injector::WriteMemory(0x6FC051 + 0x2, 0x7080 * 0xA, true); // sun reflection
    injector::WriteMemory<float>(0x49DCF4, 550.0f, true); //Traffic lights corona draw distance

    struct LamppostsCoronaFarclpHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax &= 1;

            *(float*)(regs.esi + 0x14) = 300.0f;
        }
    }; injector::MakeInline<LamppostsCoronaFarclpHook>(0x6FCFBA);

    // Shadows: the game's distances times ShadowDistance, the intensity Project2DFX was tuned with
    for (auto address : { 0x6FD3A6, 0x6FD44F, 0x455EF3, 0x4561B3, 0x49DF7A, 0x53B5E2, 0x70C88B, 0x70C9F4 })
    {
        ProtectedGameRef<float> distance((float*)address);
        distance = distance * fShadowDistance;
    }

    if (fStaticShadowsIntensity)
    {
        for (auto address : { 0x6FD13C, 0x6FD16E, 0x6FD17C, 0x6FD1CE, 0x6FD2C0, 0x6FD301, 0x6FD30F, 0x6FD3BC, 0x6FD3DA, 0x6FD3F8 })
            injector::WriteMemory(address, &fStaticShadowsIntensity, true); // fmul flt_859AA0
    }

    // The game's lamp coronas and light pools, which the distant ones hand over to
    CLODLights::ConfigureLightVolumes(0xC97C28);
    static auto lightVolumeRenderHook = safetyhook::create_mid(reinterpret_cast<void*>(0x53DFE2), [](SafetyHookContext&)
    {
        if (bRenderLodLights)
            CLODLights::RenderPools();
    });

    GameLights::fTrafficRange = 550.0f;
    GameLights::fPoolRange = *reinterpret_cast<float*>(0x6FD3A6);
    GameLights::fPoolColour = (fStaticShadowsIntensity ? fStaticShadowsIntensity : *reinterpret_cast<float*>(0x859AA0));
    static auto ProcessLightsForEntityHook = safetyhook::create_mid(reinterpret_cast<void*>(0x6FC7A0), [](SafetyHookContext& regs)
    {
        LampEntities::MarkProcessed(reinterpret_cast<CEntity*>(regs.ecx)->GetCoords());
    });

    // Small objects and vegetation keep their own range, read when the IDE files load.
    static const float detailFactor = fDetailDistance;

    struct IncreaseDrawDistanceForTimedObjectsHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edi = regs.eax;
            *(float*)(regs.esi + 0x18) = *(float*)&regs.ecx;
            if (*(float*)(regs.esi + 0x18) <= 300.0f)
                *(float*)(regs.esi + 0x18) *= detailFactor;

        }
    }; injector::MakeInline<IncreaseDrawDistanceForTimedObjectsHook>(0x5B3F4C);

    {
        struct IncreaseDrawDistanceForObjectsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)&regs.edx = *(float*)(regs.esp + 0xC);
                regs.esi = regs.eax;

                auto modelID = regs.ecx;
                float drawDist = *(float*)(regs.esp + 0xC);

                auto isInteriorObjectID = [](int id) -> bool
                {
                    if (id < 0) return false;
                    if (id >= 910 && id <= 955) return true;
                    if (id >= 1700 && id <= 1776) return true;
                    if (id >= 2360 && id <= 2872) return true;
                    if (id >= 2880 && id <= 2882) return true;
                    if (id >= 13590 && id <= 13667) return true;
                    if (id >= 14383 && id <= 14495) return true;
                    if (id >= 14497 && id <= 14528) return true;
                    if (id >= 14530 && id <= 14540) return true;
                    if (id >= 14542 && id <= 14554) return true;
                    if (id == 14556) return true;
                    if (id >= 14558 && id <= 14614) return true;
                    if (id >= 14616 && id <= 14630) return true;
                    if (id >= 14632 && id <= 14633) return true;
                    if (id >= 14635 && id <= 14641) return true;
                    if (id == 14643) return true;
                    if (id >= 14650 && id <= 14657) return true;
                    if (id >= 14660 && id <= 14695) return true;
                    if (id >= 14699 && id <= 14728) return true;
                    if (id >= 14735 && id <= 14762) return true;
                    if (id >= 14764 && id <= 14765) return true;
                    if (id >= 14770 && id <= 14856) return true;
                    if (id >= 14858 && id <= 14883) return true;
                    if (id >= 14885 && id <= 14898) return true;
                    if (id >= 14900 && id <= 14903) return true;
                    if (id >= 15025 && id <= 15043) return true;
                    if (id >= 15046 && id <= 15064) return true;
                    if (id >= 18000 && id <= 18036) return true;
                    if (id >= 18038 && id <= 18075) return true;
                    if (id >= 18077 && id <= 18086) return true;
                    if (id >= 18088 && id <= 18092) return true;
                    if (id >= 18094 && id <= 18095) return true;
                    if (id >= 18098 && id <= 18101) return true;
                    if (id >= 18104 && id <= 18105) return true;
                    if (id == 18109) return true;
                    if (id == 18112) return true;

                    if (id >= 1327 && id <= 1572) return true; //dynamic2.ide
                    return false;
                };

                if (isInteriorObjectID(modelID))
                {
                    *(float*)&regs.edx = drawDist;
                    return;
                }

                if (drawDist <= 300.0f)
                {
                    drawDist *= detailFactor;
                    fMaxDrawDistanceForNormalObjects = std::max(fMaxDrawDistanceForNormalObjects, drawDist);
                }
                *(float*)&regs.edx = drawDist;
            }
        }; injector::MakeInline<IncreaseDrawDistanceForObjectsHook>(0x5B3D9F, 0x5B3D9F + 6);

        // The 300 m the renderer draws non-LOD objects to
        for (auto address : { 0x554230 + 0x3B6 + 2, 0x554230 + 0x3D0 + 2, 0x554230 + 0x3FA + 2, 0x554FE0 + 0x192 + 2, 0x554FE0 + 0x1B8 + 2,
                              0x554FE0 + 0x1DB + 2, 0x554FE0 + 0x24E + 2, 0x554FE0 + 0x258 + 2, 0x554FE0 + 0x262 + 2, 0x554FE0 + 0x314 + 2,
                              0x554FE0 + 0x31E + 2, 0x554FE0 + 0x328 + 2, 0x554FE0 + 0x382 + 2, 0x554FE0 + 0x39A + 2, 0x554FE0 + 0x3A8 + 2,
                              0x5B51E0 + 0x09A + 2 })
            injector::WriteMemory(address, &fMaxDrawDistanceForNormalObjects, true);
    }

    // Keeps LOD models loaded, so they don't pop in. Only with the limit adjuster's streaming memory: they stay in it.
    if (bPreloadLODs && LimitAdjusterStreamingBudget() >= 256 * 1024 * 1024)
    {
        enum eStreamingFlags
        {
            DEFAULT = 0x0,
            GAME_REQUIRED = 0x2,
            MISSION_REQUIRED = 0x4,
            KEEP_IN_MEMORY = 0x8,
            PRIORITY_REQUEST = 0x10,
            LOADING_SCENE = 0x20,
            SCM_COMMAND_FLAG = 0x0C
        };

        static std::vector<CEntity*> lodPtrs;

        pattern = hook::pattern("8A 48 ? 80 F9 ? 8B 3C 95");
        static auto LinkLodsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            lodPtrs.push_back((CEntity*)regs.eax);
        });

        pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? A0");
        static auto CGameProcessHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            if (gGameState != 9) //GAMESTATE_LOADED
                return;

            if (lodPtrs.empty())
                return;

            auto uniquePtrs = lodPtrs;
            std::sort(uniquePtrs.begin(), uniquePtrs.end(), [](CEntity* a, CEntity* b) { return a->GetModelIndex() < b->GetModelIndex(); });
            uniquePtrs.erase(std::unique(uniquePtrs.begin(), uniquePtrs.end(), [](CEntity* a, CEntity* b) { return a->GetModelIndex() == b->GetModelIndex(); }), uniquePtrs.end());
            for (auto* e : uniquePtrs) CStreaming::RequestModel(e->GetModelIndex(), eStreamingFlags::GAME_REQUIRED | eStreamingFlags::KEEP_IN_MEMORY);
            CStreaming::LoadAllRequestedModels(false);
            for (auto* e : lodPtrs) if (!e->m_pRwObject) CreateRwObject(e, nullptr);
            lodPtrs.clear();
        });
    }
}

void GetMemoryAddresses()
{
    CModelInfo::GetModelInfo = (decltype(CModelInfo::GetModelInfo))0x4C5940;
    CTimer::m_snTimeInMillisecondsPauseMode.SetAddress((unsigned int*)0xB7CB7C);
    CTimer::m_snTimeInMilliseconds.SetAddress((unsigned int*)0xB7CB84);
    CTrafficLights::TimeDivisor = 2;
    CTrafficLights::LightForCars1_Visual = reinterpret_cast<decltype(CTrafficLights::LightForCars1_Visual)>(0x49D350); // _TimeToSwitchTrafficLightToYellow
    CTrafficLights::LightForCars2_Visual = reinterpret_cast<decltype(CTrafficLights::LightForCars2_Visual)>(0x49D3A0); // _TimeToSwitchTrafficLightToGreen
    CTrafficLights::FindTrafficLightType = reinterpret_cast<decltype(CTrafficLights::FindTrafficLightType)>(0x49D580);
    CTimer::ms_fTimeStep.SetAddress((float*)0xB7CB5C);
    TheCamera.SetAddress((CCamera*)0xB6F028);

    CWeather::Rain.SetAddress((float*)0xC81324);
    CWeather::Foggyness.SetAddress((float*)0xC81300);
    CWeather::UnderWaterness.SetAddress((float*)0xC8132C);
    gpCoronaTexture = (RwTexture**)0xC3E000;

    CSprite::CalcScreenCoorsMaxMin = (decltype(CSprite::CalcScreenCoorsMaxMin))0x70CE30;
    CSprite::FlushSpriteBuffer = (decltype(CSprite::FlushSpriteBuffer))0x70CF20;
    CSprite::RenderOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderOneXLUSprite_Rotate_Aspect))0x70D490;
    CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect))0x70E780;

    CDraw::ms_fNearClipZ.SetAddress((float*)0xC3EFA0);
    CDraw::ms_fFarClipZ.SetAddress((float*)0xC3EF9C);

    CSprite::m_f2DNearScreenZ.SetAddress((float*)0xC4B8D8);
    CSprite::m_f2DFarScreenZ.SetAddress((float*)0xC4B8D4);

    Scene.SetAddress((CScene*)0xC17038);
    RwEngineInstance.SetAddress((RwGlobals**)0xC97B24);

    CWaterLevel::GetWaterLevelNoWaves = reinterpret_cast<decltype(CWaterLevel::GetWaterLevelNoWaves)>(0x6E8580);

    RwIm3DTransform = (decltype(RwIm3DTransform))0x7EF450;
    RwIm3DRenderIndexedPrimitive = (decltype(RwIm3DRenderIndexedPrimitive))0x7EF550;
    RwIm3DEnd = (decltype(RwIm3DEnd))0x7EF520;

    RwIm2DRenderIndexedPrimitive = (decltype(RwIm2DRenderIndexedPrimitive))0x734EA0;
    RwIm2DRenderPrimitive = (decltype(RwIm2DRenderPrimitive))0x734E90;

    CGame::currArea.SetAddress((int*)0xB72914);

    CWorld::FindGroundZFor3DCoordCRGO = (decltype(CWorld::FindGroundZFor3DCoordCRGO))0x5696C0;
    CClock::GetIsTimeInRange = (decltype(CClock::GetIsTimeInRange))0x52CEE0;

    CClock::ms_nGameClockHours.SetAddress((uint8_t*)0xB70153);
    CClock::ms_nGameClockMinutes.SetAddress((uint8_t*)0xB70152);

    CTimeCycle::m_fCurrentFarClip.SetAddress((float*)0xB7C4F0);
    CTimeCycle::m_fCurrentFogStart.SetAddress((float*)0xB7C4F4);

    CModelInfo::pp_modelInfoPtrs = (CBaseModelInfo***)(0x49DB82 + 3);

    CModelInfo::Get2dEffect = (decltype(CModelInfo::Get2dEffect))0x4C4C70;

    CRenderer::ms_lodDistScale.SetAddress((float*)0x8CD800);

    CTimeCycle::m_fCurrentSpriteBrightness.SetAddress((float*)0xB7C4E4);
    CTimeCycle::m_fCurrentSpriteSize.SetAddress((float*)0xB7C4E0);
    GameLights::FOV.SetAddress((float*)0x8D5038);
    GameLights::fSpriteBrightnessScale = 0.1f;
    GameLights::fSpriteSizeScale = 0.1f;
    GameLights::fLampPull = 0.8f;
    GameLights::bDayNightBalance = true;
    RwRasterCreate = (decltype(RwRasterCreate))0x7FB230;
    RwRasterLock = (decltype(RwRasterLock))0x7FB2D0;
    RwRasterUnlock = (decltype(RwRasterUnlock))0x7FAEC0;

    CPointLights::AddLightWithEntity = (decltype(CPointLights::AddLightWithEntity))0x7000E0;

    CreateRwObject = (decltype(CreateRwObject))0x533D30;
    CStreaming::RequestModel = (decltype(CStreaming::RequestModel))0x4087E0;
    CStreaming::LoadAllRequestedModels = (decltype(CStreaming::LoadAllRequestedModels))0x40EA10;

    MakeRequestForNodesToBeLoaded = (decltype(MakeRequestForNodesToBeLoaded))0x450D70;
}

void Init()
{
    fCoronaRadiusBase = 0.5f;
    ReadSettings();
    GetMemoryAddresses();
    ApplyMemoryPatches();
    Settings().OnChange("DISTANCE.DetailDistance", [] { ReadSettings(); });
}

ASI_SETTINGS_EXPORT(Settings())

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8B 2D ? ? ? ? 83 C4 ? 6A"));
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
