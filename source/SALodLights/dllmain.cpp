#define NOMINMAX
#include "stdafx.h"
#include <ranges>
#include <deque>

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

float fSunSizeMult1 = 2.7335f;
float fSunSizeMult2 = 6.0f;
void DrawDistanceChanger()
{
    if (fFarClipStaticMultiplier != 0.0f)
    {
        fFarClipMultiplier = fFarClipStaticMultiplier;
        fSunSizeMult1 = 2.7335f * fFarClipMultiplier;
        fSunSizeMult2 = 6.0f * fFarClipMultiplier;
        return;
    }

    static LARGE_INTEGER freq = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    static std::deque<int64_t> frameTimes;
    static float adaptiveBase = fFarClipMinMultiplier;

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    frameTimes.push_back(now.QuadPart);
    if (frameTimes.size() > 60)
        frameTimes.pop_front();

    if (frameTimes.size() >= 2)
    {
        float smoothFPS = (float)(frameTimes.size() - 1) * (float)freq.QuadPart / (float)(frameTimes.back() - frameTimes.front());
        const float step = (fFarClipMaxMultiplier - fFarClipMinMultiplier) * 0.002f;
        if (smoothFPS < (float)nFarClipTargetFPS)
            adaptiveBase -= step;
        else
            adaptiveBase += step;

        adaptiveBase = std::clamp(adaptiveBase, fFarClipMinMultiplier, fFarClipMaxMultiplier);
    }

    float camZ = GetCamPos()->z;
    float heightBonus = fFarClipHeightFactor * std::max(camZ, 0.0f);

    fFarClipMultiplier = std::clamp(adaptiveBase + heightBonus, fFarClipMinMultiplier, fFarClipMaxMultiplier);
    fSunSizeMult1 = 2.7335f * fFarClipMultiplier;
    fSunSizeMult2 = 6.0f * fFarClipMultiplier;
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

    pattern = hook::pattern("DB 05 ? ? ? ? D8 15 ? ? ? ? DF E0 F6 C4 05 7A");
    static auto FarClipHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (CGame::currArea == 0)
            CTimeCycle::m_fCurrentFarClip *= fFarClipMultiplier;
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 8B 0D ? ? ? ? 51 E8 ? ? ? ? 8B 15 ? ? ? ? 52 E8 ? ? ? ? 83 C4 ? 83 C4");
    injector::MakeCALL(pattern.get_first(), DrawDistanceChanger, true);

    injector::WriteMemory(0x6FC65C + 2, &fSunSizeMult1, true);
    injector::WriteMemory(0x6FC6E8 + 2, &fSunSizeMult2, true);

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

    if (bPreloadLODs)
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

        pattern = hook::pattern("0F BF 48 ? 0F BF 56");
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

    CShadows::gpShadowExplosionTex = (RwTexture**)0xC403F4;
    CShadows::StoreStaticShadow = (decltype(CShadows::StoreStaticShadow))0x70BA00;

    CPointLights::AddLightWithEntity = (decltype(CPointLights::AddLightWithEntity))0x7000E0;

    CreateRwObject = (decltype(CreateRwObject))0x533D30;
    CStreaming::RequestModel = (decltype(CStreaming::RequestModel))0x4087E0;
    CStreaming::LoadAllRequestedModels = (decltype(CStreaming::LoadAllRequestedModels))0x40EA10;
}

void Init()
{
    ReadIniSettings();
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
