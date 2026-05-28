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
    EXPLOSION_BOAT,
    EXPLOSION_HELI,
    EXPLOSION_HELI2,
    EXPLOSION_MINE,
    EXPLOSION_BARREL,
    EXPLOSION_TANK_GRENADE,
    EXPLOSION_HELI_BOMB
};

SafetyHookInline shAddExplosion = {};
bool __cdecl AddExplosion(CEntity* explodingEntity, CEntity* culprit, eExplosionType type, const CVector* pos, uint32_t lifetime, bool makeSound)
{
    static const eExplosionType allTypes[] = {
        EXPLOSION_GRENADE,
        //EXPLOSION_MOLOTOV,
        EXPLOSION_ROCKET,
        EXPLOSION_CAR,
        EXPLOSION_CAR_QUICK,
        EXPLOSION_BOAT,
        //EXPLOSION_HELI,
        //EXPLOSION_HELI2,
        EXPLOSION_MINE,
        EXPLOSION_BARREL,
        EXPLOSION_TANK_GRENADE,
        EXPLOSION_HELI_BOMB,
    };

    if (!std::any_of(std::begin(allTypes), std::end(allTypes), [type](eExplosionType t) { return t == type; }))
        return shAddExplosion.unsafe_ccall<bool>(explodingEntity, culprit, type, pos, lifetime, makeSound);

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
        shAddExplosion.unsafe_ccall<bool>(explodingEntity, culprit, shuffled[i], pos, lifetime, false);
    }

    return shAddExplosion.unsafe_ccall<bool>(explodingEntity, culprit, type, pos, lifetime, makeSound);
}

void (__cdecl* AddParticle)(int16_t type, CVector const* vecPos, CVector const* vecDir, CEntity* pEntity, float fSize, int32_t nRotationSpeed, int32_t nRotation, int32_t nCurFrame, int32_t nLifeSpan) = nullptr;

SafetyHookInline shAddTrace = {};
void __cdecl AddTrace(CVector* start, CVector* end, float thickness, uint32_t lifeTime, uint8_t visibility)
{
    CVector tracerDir = (*end - *start);
    float tracerLen = tracerDir.Magnitude();
    if (tracerLen > 0.001f)
    {
        constexpr auto PARTICLE_HELI_ATTACK = 56;
        float speed = 0.1f + thickness * 0.05f;
        auto dir = tracerDir * speed;
        AddParticle(PARTICLE_HELI_ATTACK, start, &dir, nullptr, 0.0f, 0, 0, 0, 0);
    }

    shAddTrace.unsafe_ccall(start, end, 0.0f, 0, 0);
}

void ApplyMemoryPatches()
{
    auto pattern = hook::pattern("E8 ? ? ? ? 0F BF 43 ? 59 8B 0C 85 ? ? ? ? 89 CF");
    CWorld::hbAdd.fun = injector::MakeCALL(pattern.get_first(), CWorld::Add, true).get();

    pattern = hook::pattern("E8 ? ? ? ? 0F BF 56");
    CWorld::hbAdd.fun = injector::MakeCALL(pattern.get_first(), CWorld::Add, true).get();

    pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
    static auto CGameInitialiseHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
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

            for (auto entity : CWorld::aBigBuildings)
                PossiblyAddThisEntity(entity);

            RegisterCustomCoronas();
            m_bCatchLamppostsNow = false;
            m_Lampposts.shrink_to_fit();
            FileContent.clear();
            CWorld::aBigBuildings.clear();
        }

        bOnce = true;
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? C3");
    static auto RenderEffectsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
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
                _asm {fstp dword ptr ds : [00690220h] }
                injector::WriteMemory<float>(0x690220, *(float*)0x690220 * (fDrawDistance / 1.8f), true);
            }
        }; injector::MakeInline<DDHookNoLambda>(0x490132, 0x490132 + 6);
        injector::WriteMemory<float>(0x499800 + 3, 1.2f * (fDrawDistance / 1.8f), true);
    }

    if (fMaxDrawDistanceForNormalObjects)
    {
        injector::WriteMemory<float>(0x69022C, fMaxDrawDistanceForNormalObjects, true);
    }

    pattern = hook::pattern("E8 ? ? ? ? 84 C0 74 ? B9 ? ? ? ? E8 ? ? ? ? E8");
    static auto FarClipHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (CGame::currArea == 0)
            CTimeCycle::m_fCurrentFarClip *= fFarClipMultiplier;
    });

    pattern = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? A1 ? ? ? ? 59 50 E8 ? ? ? ? 80 3D");
    injector::MakeCALL(pattern.get_first(), DrawDistanceChanger, true);

    if (bRandomExplosionEffects)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B 46 ? 83 C4 ? 85 C0 74 ? 50 E8 ? ? ? ? 59");
        shAddExplosion = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AddExplosion);
    }

    if (bReplaceSmokeTrailWithBulletTrail)
    {
        pattern = hook::pattern("E8 ? ? ? ? 83 C4 ? 83 C4 ? 5D 5E 5B C3 ? ? EB");
        shAddTrace = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), AddTrace);
    }
}

void GetMemoryAddresses()
{
    CModelInfo::GetModelInfo = (decltype(CModelInfo::GetModelInfo))0x55F7D0;
    CTimer::m_snTimeInMillisecondsPauseMode.SetAddress((unsigned int*)0x974B2C);
    CTimer::ms_fTimeStep.SetAddress((float*)0x975424);
    TheCamera.SetAddress((CCamera*)0x7E4688);

    CWeather::Foggyness.SetAddress((float*)0x94DDC0);
    gpCoronaTexture = (RwTexture**)0x695538;

    CSprite::CalcScreenCoorsMax = (decltype(CSprite::CalcScreenCoorsMax))0x5778B0;
    CSprite::FlushSpriteBuffer = (decltype(CSprite::FlushSpriteBuffer))0x577790;
    CSprite::RenderOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderOneXLUSprite_Rotate_Aspect))0x576FE0;
    CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect))0x576B30;

    Scene.SetAddress((CScene*)0x8100B8);
    RwEngineInstance.SetAddress((RwGlobals**)0x7870C0);

    RwIm3DTransform = (decltype(RwIm3DTransform))0x65AE90;
    RwIm3DRenderIndexedPrimitive = (decltype(RwIm3DRenderIndexedPrimitive))0x65AF90;
    RwIm3DEnd = (decltype(RwIm3DEnd))0x65AF60;

    CGame::currArea.SetAddress((int*)0x978810);

    CWorld::FindGroundZFor3DCoordCR = (decltype(CWorld::FindGroundZFor3DCoordCR))0x4D53A0;
    CClock::GetIsTimeInRange = (decltype(CClock::GetIsTimeInRange))0x4870F0;

    CClock::ms_nGameClockHours.SetAddress((uint8_t*)0xA10B6B);
    CClock::ms_nGameClockMinutes.SetAddress((uint8_t*)0xA10B92);

    CTimeCycle::m_fCurrentFarClip.SetAddress((float*)0x9B6A6C);

    CModelInfo::pp_modelInfoPtrs = (CBaseModelInfo***)(0x406557 + 3);

    CModelInfo::Get2dEffect = (decltype(CModelInfo::Get2dEffect))0x53F260;

    CRenderer::ms_lodDistScale.SetAddress((float*)0x690220);

    CShadows::StoreStaticShadow = (decltype(CShadows::StoreStaticShadow))0x56E780;

    RwRenderStateSet = (decltype(RwRenderStateSet))0x649BA0;

    pHelis = (CHeli**)0x813D10;
    pNumRandomHelis = (int16_t*)0xA10A6A;

    CPointLights::AddLightWithoutEntity = (decltype(CPointLights::AddLightWithoutEntity))0x567700;

    AddParticle = (decltype(AddParticle))0x5648F0;
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
