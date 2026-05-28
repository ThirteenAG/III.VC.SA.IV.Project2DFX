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
import Heli;
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

    auto foundElements = FileContent | std::views::filter([minKey = PackKey(nModelID, 0), maxKey = PackKey(nModelID, 0xFFFF)](const auto& kv)
    {
        return kv.first >= minKey && kv.first <= maxKey;
    });

    auto ms_modelInfoPtrs = *CModelInfo::pp_modelInfoPtrs;
    auto modelInfo = (CSimpleModelInfo*)ms_modelInfoPtrs[nModelID];

    // Get bounding box height from model info
    float objectHeight = 0.0f;
    if (modelInfo && modelInfo->m_colModel)
    {
        // Get height from bounding box
        objectHeight = modelInfo->m_colModel->boundingBox.max.z - modelInfo->m_colModel->boundingBox.min.z;
    }

    float heading = atan2(-pObj->GetMatrix().GetRight().y, pObj->GetMatrix().GetRight().x);
    for (const auto& [key, data] : foundElements)
    {
        CVector worldPos = pObj->GetMatrix() * data.vecLocalPos;
        m_Lampposts.push_back(CLamppostInfo(
            worldPos,
            data.vecLocalPos,
            data.colour,
            data.fCustomSizeMult,
            data.nCoronaShowMode,
            data.nNoDistance,
            data.nDrawSearchlight ? static_cast<int>(objectHeight) : 0,
            heading,
            std::min(data.fObjectDrawDistance, modelInfo->m_lodDistances[2])
        ));
    }
}

CEntity* PossiblyAddThisEntity(CEntity* pEntity)
{
    if (pEntity && m_bCatchLamppostsNow && IsModelALamppost(pEntity->GetModelIndex()))
        RegisterLamppost(pEntity);
    return pEntity;
}

namespace CWorld
{
    std::vector<CEntity*> aBigBuildings;
    injector::hook_back<void(__cdecl*)(CEntity*)> hbAdd;
    void __cdecl Add(CEntity* entity)
    {
        aBigBuildings.emplace_back(entity);
        hbAdd.fun(entity);
    }

    SafetyHookInline shRepositionCertainDynamicObjects = {};
    void __cdecl RepositionCertainDynamicObjects()
    {
        static bool bOnce = false;

        if (!bOnce)
        {
            LoadDatFile();
            if (!m_bCatchLamppostsNow)
            {
                bRenderLodLights = 0;
                bRenderSearchlightEffects = 0;
            }

            CLODLights::Init(numCoronas);
        }

        shRepositionCertainDynamicObjects.unsafe_ccall();

        if (!bOnce)
        {
            for (auto entity : CWorld::aBigBuildings)
                PossiblyAddThisEntity(entity);

            RegisterCustomCoronas();
            m_bCatchLamppostsNow = false;
            m_Lampposts.shrink_to_fit();
            FileContent.clear();
            CWorld::aBigBuildings.clear();
        }

        bOnce = true;
    }
}

void DrawDistanceChanger()
{
    if (fFarClipStaticMultiplier != 0.0f)
    {
        fFarClipMultiplier = fFarClipStaticMultiplier;
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
}

enum eExplosionType
{
    EXPLOSION_GRENADE,
    EXPLOSION_MOLOTOV,
    EXPLOSION_ROCKET,
    EXPLOSION_CAR,
    EXPLOSION_CAR_QUICK,
    EXPLOSION_HELI,
    EXPLOSION_MINE,
    EXPLOSION_BARREL,
    EXPLOSION_TANK_GRENADE,
    EXPLOSION_HELI_BOMB
};

SafetyHookInline shAddExplosion = {};
bool __cdecl AddExplosion(CEntity* explodingEntity, CEntity* culprit, eExplosionType type, const CVector* pos, uint32_t lifetime)
{
    static const eExplosionType allTypes[] = {
        EXPLOSION_GRENADE,
        //EXPLOSION_MOLOTOV,
        EXPLOSION_ROCKET,
        EXPLOSION_CAR,
        EXPLOSION_CAR_QUICK,
        //EXPLOSION_HELI,
        EXPLOSION_MINE,
        EXPLOSION_BARREL,
        EXPLOSION_TANK_GRENADE,
        EXPLOSION_HELI_BOMB,
    };

    if (!std::any_of(std::begin(allTypes), std::end(allTypes), [type](eExplosionType t) { return t == type; }))
        return shAddExplosion.unsafe_ccall<bool>(explodingEntity, culprit, type, pos, lifetime);

    constexpr int typeCount = sizeof(allTypes) / sizeof(allTypes[0]);

    eExplosionType shuffled[typeCount];
    for (int i = 0; i < typeCount; i++) shuffled[i] = allTypes[i];

    for (int i = typeCount - 1; i > 0; i--)
    {
        int j = CGeneral::GetRandomNumberInRange(0, i + 1);
        eExplosionType tmp = shuffled[i];
        shuffled[i] = shuffled[j];
        shuffled[j] = tmp;
    }

    for (int i = 0; i < typeCount; i++)
    {
        if (shuffled[i] == type)
            break;
        shAddExplosion.unsafe_ccall<bool>(explodingEntity, culprit, shuffled[i], pos, lifetime);
    }

    return shAddExplosion.unsafe_ccall<bool>(explodingEntity, culprit, type, pos, lifetime);
}

void (__cdecl* AddParticle)(int16_t type, CVector const* vecPos, CVector const* vecDir, CEntity* pEntity, float fSize, int32_t nRotationSpeed, int32_t nRotation, int32_t nCurFrame, int32_t nLifeSpan) = nullptr;

SafetyHookInline shAddTrace = {};
void __cdecl AddTrace(CVector* start, CVector* end)
{
    CVector tracerDir = *end - *start;
    float tracerDist = tracerDir.Magnitude();
    if (tracerDist > 0.0f)
    {
        constexpr auto PARTICLE_HELI_ATTACK = 47;
        tracerDir *= (1.0f / tracerDist);
        auto dir = tracerDir * 2.0f;
        AddParticle(PARTICLE_HELI_ATTACK, start, &dir, nullptr, 0.0f, 0, 0, 0, 0);
    }

    shAddTrace.unsafe_ccall(start, end, 0.0f, 0, 0);
}

void ApplyMemoryPatches()
{
    auto pattern = hook::pattern("E8 ? ? ? ? 59 53 E8 ? ? ? ? 59 81 C4 ? ? ? ? 5D");
    CWorld::hbAdd.fun = injector::MakeCALL(pattern.get_first(), CWorld::Add, true).get();

    pattern = hook::pattern("E8 ? ? ? ? 66 8B 45 ? 59");
    CWorld::hbAdd.fun = injector::MakeCALL(pattern.get_first(), CWorld::Add, true).get();

    pattern = hook::pattern("E8 ? ? ? ? 6A ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
    CWorld::shRepositionCertainDynamicObjects = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CWorld::RepositionCertainDynamicObjects);

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? C3");
    static auto RenderEffectsHook = safetyhook::create_mid(pattern.count(2).get(1).get<void*>(), [](SafetyHookContext& regs)
    {
        if (bRenderLodLights)
            CLODLights::RenderBuffered();

        if (bRenderSearchlightEffects)
        {
            RenderAllSearchLights();

            if (pHelis)
            {
                auto NumOfHelisRequired = *pNumRandomHelis;
                if (NumOfHelisRequired)
                {
                    Pre_SearchLightCone();
                    for (auto i = 0; i < NumOfHelisRequired; i++)
                    {
                        auto heli = pHelis[i];
                        if (heli && heli->m_nLastShotTime)
                        {
                            RwV3D EndPoint = { heli->m_fSearchLightX, heli->m_fSearchLightY, heli->GetPosition().z };
                            if (EndPoint.x && EndPoint.y)
                            {
                                EndPoint.z = CWorld::FindGroundZFor3DCoord(EndPoint.x, EndPoint.y, EndPoint.z, nullptr, nullptr);
                                SearchLightCone(heli->GetPosition(), EndPoint, 13.0f, 0.1f, CRGBA(255, 255, 255, 255), 1.0f, true);
                            }
                        }
                    }
                    Post_SearchLightCone();
                }
            }
        }
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 80 3D");
    static auto CGameProcessHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CLODLights::RegisterLODLights();
    });

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

    pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 ? B9 ? ? ? ? E8 ? ? ? ? E8");
    static auto FarClipHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (CGame::currArea == 0)
            CTimeCycle::m_fCurrentFarClip *= fFarClipMultiplier;
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59");
    injector::MakeCALL(pattern.get_first(), DrawDistanceChanger, true);

    if (bRandomExplosionEffects)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B 46 ? 83 C4 ? 50");
        shAddExplosion = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AddExplosion);
    }

    if (bReplaceSmokeTrailWithBulletTrail)
    {
        pattern = hook::pattern("E8 ? ? ? ? 83 BC 24 ? ? ? ? ? 59 59 74 ? E8");
        shAddTrace = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AddTrace);
    }
}

void GetMemoryAddresses()
{
    CModelInfo::GetModelInfo = (decltype(CModelInfo::GetModelInfo))0x50B860;
    CTimer::m_snTimeInMillisecondsPauseMode.SetAddress((unsigned int*)0x885B48);
    CTimer::ms_fTimeStep.SetAddress((float*)0x8E2CB4);
    TheCamera.SetAddress((CCamera*)0x6FACF8);

    CWeather::Foggyness.SetAddress((float*)0x885AF4);
    gpCoronaTexture = (RwTexture**)0x5FAF44;

    CSprite::CalcScreenCoorsMax = (decltype(CSprite::CalcScreenCoorsMax))0x51C3A0;
    CSprite::FlushSpriteBuffer = (decltype(CSprite::FlushSpriteBuffer))0x51C520;
    CSprite::RenderOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderOneXLUSprite_Rotate_Aspect))0x51D110;
    CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect))0x51CCD0;

    Scene.SetAddress((CScene*)0x726768);
    RwEngineInstance.SetAddress((RwGlobals**)0x661228);

    RwIm3DTransform = (decltype(RwIm3DTransform))0x5B6720;
    RwIm3DRenderIndexedPrimitive = (decltype(RwIm3DRenderIndexedPrimitive))0x5B6820;
    RwIm3DEnd = (decltype(RwIm3DEnd))0x5B67F0;

    static int currArea = 0;
    CGame::currArea.SetAddress(&currArea);

    CWorld::FindGroundZFor3DCoordCR = (decltype(CWorld::FindGroundZFor3DCoordCR))0x4B3AE0;
    CClock::GetIsTimeInRange = (decltype(CClock::GetIsTimeInRange))0x473420;

    CClock::ms_nGameClockHours.SetAddress((uint8_t*)0x95CDA6);
    CClock::ms_nGameClockMinutes.SetAddress((uint8_t*)0x95CDC8);

    CTimeCycle::m_fCurrentFarClip.SetAddress((float*)0x8F5FD8);

    CModelInfo::pp_modelInfoPtrs = (CBaseModelInfo***)(0x40394A + 3);

    CModelInfo::Get2dEffect = (decltype(CModelInfo::Get2dEffect))0x4F6B00;

    CRenderer::ms_lodDistScale.SetAddress((float*)0x5F726C);

    CShadows::StoreStaticShadow = (decltype(CShadows::StoreStaticShadow))0x5130A0;

    RwRenderStateSet = (decltype(RwRenderStateSet))0x5A43C0;

    pHelis = (CHeli**)0x72CF50;
    pNumRandomHelis = (int16_t*)0x95CCAA;

    CPointLights::AddLightWithoutEntity = (decltype(CPointLights::AddLightWithoutEntity))0x510790;

    AddParticle = (decltype(AddParticle))0x50D140;
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
