#define NOMINMAX
#include "stdafx.h"
#include <ranges>
#include <deque>
#include "asi_settings/asi_settings.hpp"
#include <iterator>
#include <string>

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
import DistantCars;
import DistantCarRenderer;
import WaterLevel;

#include "TrafficLights.hpp"

using RwV3D = RwV3d;

void RegisterCustomCoronas()
{
    unsigned short nModelID = 65534;
    auto itEnd = FileContent.upper_bound(PackKey(nModelID, 0xFFFF));
    for (auto it = FileContent.lower_bound(PackKey(nModelID, 0)); it != itEnd; it++)
        m_Lampposts.push_back(CLamppostInfo(it->second.vecLocalPos, { 0.0f, 0.0f, 0.0f }, it->second.colour, it->second.fCustomSizeMult, it->second.nCoronaShowMode, it->second.nNoDistance, it->second.nDrawSearchlight, 0.0f, 0.0f, it->second.pPredicate));
}

// --------------------------------------------------------------------------
// Model swaps (03B6 COMMAND_SWAP_NEAREST_BUILDING_MODEL). Swapped models 
// never reach the lamppost collection. Each entry is a unique pair of models 
// that swap each other:
// for an entity of either model we additionally insert lampposts using the
// other model's 2dfx data. Both sets coexist in m_Lampposts and predicates
// decide which one is visible.
// --------------------------------------------------------------------------
static const std::pair<const char*, const char*> aModelSwaps[] =
{
    { "bldngst2mesh", "bldngst2meshdam" },
    { "od_wtsign", "od_chariot" },
    { "lodd_buildnew", "lodchariot" },
    { "ml_tarbrush", "ml_tarbrushdamage" },
    { "build1", "buildblown" },
    { "hdrugfactoryint", "hdrugfactorypol" },
    { "lodld1", "lodldblown" },
};

// --------------------------------------------------------------------------
// CTheScripts::BuildingSwapArray — ground truth for which models are
// currently swapped.
// --------------------------------------------------------------------------
struct CBuildingSwap
{
    CEntity* pBuilding;
    int32_t nNewModel;
    int32_t nOldModel;
};

static CBuildingSwap* BuildingSwapArray = nullptr; // address set in GetMemoryAddresses()
static constexpr int MAX_NUM_BUILDING_SWAPS = 25;

// Model is currently swapped out (replaced by another model) right now.
static bool IsModelSwappedOut(int nModelID)
{
    for (int i = 0; i < MAX_NUM_BUILDING_SWAPS; i++)
    {
        if (BuildingSwapArray[i].pBuilding && BuildingSwapArray[i].nOldModel == nModelID)
            return true;
    }
    return false;
}

// Model is currently the active replacement of a swapped building.
static bool IsActiveReplacement(int nModelID)
{
    for (int i = 0; i < MAX_NUM_BUILDING_SWAPS; i++)
    {
        if (BuildingSwapArray[i].pBuilding && BuildingSwapArray[i].nNewModel == nModelID)
            return true;
    }
    return false;
}

// Install per-model corona predicates for every swap pair:
// - swapped out right now  -> hidden
// - active replacement     -> visible
// - no swap for this pair  -> only the default (first) variant is visible
// Called once before LoadDatFile, when model info is already available.
void SetupSwapCoronaPredicates()
{
    for (const auto& pair : aModelSwaps)
    {
        bool bIsDefaultVariant = true;

        for (const char* szName : { pair.first, pair.second })
        {
            int nID = -1;
            CModelInfo::GetModelInfo(szName, &nID);

            CCoronaVisibility::SetModelPredicate((std::string("%") + szName).c_str(), [nID, bIsDefaultVariant]() -> bool
            {
                if (nID < 0)
                    return true;
                if (IsModelSwappedOut(nID))
                    return false;
                if (IsActiveReplacement(nID))
                    return true;
                return bIsDefaultVariant;
            });

            bIsDefaultVariant = false;
        }
    }
}

static int anModelSwapIDs[std::size(aModelSwaps)][2];

void ResolveModelSwapIDs()
{
    static bool bResolved = false;
    if (bResolved)
        return;
    bResolved = true;

    for (size_t i = 0; i < std::size(aModelSwaps); i++)
    {
        int nFrom = -1, nTo = -1;
        CModelInfo::GetModelInfo(aModelSwaps[i].first, &nFrom);
        CModelInfo::GetModelInfo(aModelSwaps[i].second, &nTo);
        anModelSwapIDs[i][0] = nFrom;
        anModelSwapIDs[i][1] = nTo;
    }
}

bool IsModelASwapSource(unsigned short nModelID)
{
    ResolveModelSwapIDs();
    for (auto& pair : anModelSwapIDs)
    {
        if ((pair[0] == nModelID && pair[1] >= 0) || (pair[1] == nModelID && pair[0] >= 0))
            return true;
    }
    return false;
}

void InsertLamppostsForModel(CEntity* pObj, unsigned short nModelID)
{
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
    const size_t firstLamp = m_Lampposts.size();
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
            std::min(data.fObjectDrawDistance, modelInfo->m_lodDistances[2]),
            data.pPredicate
        ));
        m_Lampposts.back().fGameCoronaRange = data.fObjectDrawDistance;
        m_Lampposts.back().pool = data.pool;
        if (modelInfo && modelInfo->m_colModel)
        {
            const CVector base{ pObj->GetMatrix() * CVector(data.vecLocalPos.x, data.vecLocalPos.y, modelInfo->m_colModel->boundingBox.min.z) };
            m_Lampposts.back().fPoolBaseZ = base.z;
        }
        m_Lampposts.back().poolTexture = data.poolTexture;
        if (data.fCustomSizeMult == .45f)
            m_Lampposts.back().nTrafficLightType = CTrafficLights::Type(pObj, pObj->GetMatrix().GetForward());
    }
    LampEntities::Attach(firstLamp, pObj->GetPosition(), 0.0f, nModelID);
}

void RegisterLamppost(CEntity* pObj)
{
    // Regular lampposts for the entity's own model
    InsertLamppostsForModel(pObj, pObj->GetModelIndex());

    // Each swap pair swaps with each other, so for either side of a pair we
    // also insert the other side's 2dfx data. Predicates pick which side is
    // visible at any given time.
    ResolveModelSwapIDs();
    unsigned short nModelID = pObj->GetModelIndex();
    for (auto& pair : anModelSwapIDs)
    {
        if (pair[0] == nModelID && pair[1] >= 0 && pair[1] <= 0xFFFF)
            InsertLamppostsForModel(pObj, static_cast<unsigned short>(pair[1]));
        else if (pair[1] == nModelID && pair[0] >= 0 && pair[0] <= 0xFFFF)
            InsertLamppostsForModel(pObj, static_cast<unsigned short>(pair[0]));
    }
}

CEntity* PossiblyAddThisEntity(CEntity* pEntity)
{
    if (pEntity && m_bCatchLamppostsNow && (IsModelALamppost(pEntity->GetModelIndex()) || IsModelASwapSource(pEntity->GetModelIndex())))
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

bool ExteriorView()
{
    return CGame::currArea == 0 && !*reinterpret_cast<bool*>(0xA10AB2);
}

#include "../ViewDistance.hpp"

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
    //1,3,4,5 - barrel crash
    static const eExplosionType allTypes[] = {
        EXPLOSION_GRENADE,
        //EXPLOSION_MOLOTOV,
        EXPLOSION_ROCKET,
        //EXPLOSION_CAR,
        //EXPLOSION_CAR_QUICK,
        //EXPLOSION_BOAT,
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
            // Install per-model corona predicates here, before LoadDatFile.
            // Both swapped model variants are inserted into the lamppost
            // array; the game's BuildingSwapArray decides which side is
            // actually present, so each predicate hides the model that is
            // currently swapped out.
            SetupSwapCoronaPredicates();
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

    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 31 ED");
    static auto CMovingThingsInitHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CMovingThings::InitDistantCarImpostors();
    });

    pattern = hook::pattern("31 FF 31 ED BE ? ? ? ? 90");
    static auto CMovingThingsUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bRenderLodLights)
            CLODLights::RegisterLODLights();
        if (nDistantTraffic > 0)
            CMovingThings::UpdateDistantCarImpostors();
    });

    pattern = hook::pattern("59 59 6A ? 6A ? E8 ? ? ? ? 59 59 6A ? 6A ? E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? E8");
    static auto distantCarsSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&)
    {
        if (nDistantTraffic > 0)
            CMovingThings::RenderDistantCarImpostors();
    });

    pattern = hook::pattern("BD C0 43 93 00 8D 80");
    static auto CMovingThingsRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (bRenderLodLights)
        {
            CLODLights::RenderBuffered();
        }

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

    pattern = hook::pattern("C6 05 ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 83 C4 08");
    static auto CMovingThingsShutdownHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CMovingThings::ShutdownDistantCarImpostors();
        CLODLights::Shutdown();
    });

    // Shadows: the game's distances times ShadowDistance, the intensities Project2DFX was tuned with
    {
        ProtectedGameRef<float> trafficLights((float*)0x68A848);
        trafficLights = trafficLights * fShadowDistance;
        ProtectedGameRef<float> staticShadows((float*)0x6882A4);
        staticShadows = staticShadows * fShadowDistance;
    }
    static float fStaticShadowsDrawDistance = **(float**)(0x541590 + 0x7A4 + 2) * fShadowDistance;
    injector::WriteMemory(0x541590 + 0x7A4 + 2, &fStaticShadowsDrawDistance, true);
    injector::WriteMemory(0x541590 + 0x8D5 + 2, &fStaticShadowsDrawDistance, true);

    if (fStaticShadowsIntensity)
        injector::WriteMemory<float>(0x69587C, fStaticShadowsIntensity, true);

    // The game's lamp coronas and light pools, which the distant ones hand over to
    CLODLights::ConfigureLightVolumes(0x7897A8);
    static auto lightVolumeRenderHook = safetyhook::create_mid(reinterpret_cast<void*>(0x4A659E), [](SafetyHookContext&)
    {
        if (bRenderLodLights)
            CLODLights::RenderPools();
    });

    GameLights::fTrafficRange = 300.0f;
    GameLights::fPoolRange = fStaticShadowsDrawDistance;
    GameLights::fPoolColour = *reinterpret_cast<float*>(0x69587C);
    static auto ProcessLightsForEntityHook = safetyhook::create_mid(reinterpret_cast<void*>(0x541590), [](SafetyHookContext& regs)
    {
        LampEntities::MarkProcessed(reinterpret_cast<CEntity*>(regs.ecx)->GetPosition());
    });
    if (fStaticShadowsIntensity)
        injector::WriteMemory<int>(0x465914, 255, true);
    if (fTrafficLightsShadowsIntensity)
    {
        for (auto offset : { 0x7E9, 0x82E, 0x873, 0xF4F, 0xF94, 0xFD7, 0x18CE, 0x1913, 0x1956 })
            injector::WriteMemory(0x463F90 + offset + 2, &fTrafficLightsShadowsIntensity, true);
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

    auto cameraPattern = hook::pattern("E8 ? ? ? ? 84 C0 74 ? B9 ? ? ? ? E8 ? ? ? ? E8");
    if (cameraPattern.size() == 1 && *cameraPattern.get_first<uint8_t>(-5) == 0xE8)
    {
        auto cameraProcess = cameraPattern.get_first<uint8_t>();
        ViewDistance::InstallCamera(cameraProcess + 14);
        ViewDistance::InstallTimecycle(cameraProcess - 5);
    }
    else
        OutputDebugStringA("Project2DFX: view distance hooks unavailable\n");

    // Detail distance: the 300 m the renderer draws non-LOD objects to
    OnSettingsApplied = []
    {
        ProtectedGameRef<float> normalObjects((float*)0x69022C);
        normalObjects = fMaxDrawDistanceForNormalObjects;
    };
    OnSettingsApplied();

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

    injector::WriteMemory<char>(0x542E66, 127, true); // sun reflection
    injector::WriteMemory<float>(0x68A860, 300.0f, true); // Traffic lights coronas draw distance
}

void GetMemoryAddresses()
{
    CModelInfo::GetModelInfo = (decltype(CModelInfo::GetModelInfo))0x55F7D0;
    CTimer::m_snTimeInMilliseconds.SetAddress((unsigned int*)0x974B2C);
    CTimer::EffectsTime = &CTimer::m_snTimeInMilliseconds;
    CTrafficLights::LightForCars1 = &LocalTrafficLights::LightForCars1;
    CTrafficLights::LightForCars2 = &LocalTrafficLights::LightForCars2;
    CTrafficLights::FindTrafficLightType = &LocalTrafficLights::FindTrafficLightType;
    CTrafficLights::LightForCars1_Visual = &LocalTrafficLights::LightForCars1_Visual;
    CTrafficLights::LightForCars2_Visual = &LocalTrafficLights::LightForCars2_Visual;
    LocalTrafficLights::Wind = reinterpret_cast<float*>(0x97533C); // CWeather::Wind
    LocalTrafficLights::bGreenLightsCheat = reinterpret_cast<bool*>(0xA10ADC);
    CTimer::ms_fTimeStep.SetAddress((float*)0x975424);
    TheCamera.SetAddress((CCamera*)0x7E4688);

    CWeather::Rain.SetAddress((float*)0x975340);
    CWeather::Foggyness.SetAddress((float*)0x94DDC0);
    gpCoronaTexture = (RwTexture**)0x695538;

    CSprite::CalcScreenCoorsMax = (decltype(CSprite::CalcScreenCoorsMax))0x5778B0;
    CSprite::FlushSpriteBuffer = (decltype(CSprite::FlushSpriteBuffer))0x577790;
    CSprite::RenderOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderOneXLUSprite_Rotate_Aspect))0x576FE0;
    CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect = (decltype(CSprite::RenderBufferedOneXLUSprite_Rotate_Aspect))0x576B30;

    CDraw::ms_fNearClipZ.SetAddress((float*)0x978534);
    CDraw::ms_fFarClipZ.SetAddress((float*)0xA10678);

    CSprite::m_f2DNearScreenZ.SetAddress((float*)0x97863C);
    CSprite::m_f2DFarScreenZ.SetAddress((float*)0x97F33C);

    Scene.SetAddress((CScene*)0x8100B8);
    RwEngineInstance.SetAddress((RwGlobals**)0x7870C0);

    DistantCarRenderer::fNightAmbient = .38f;
    CWaterLevel::GetWaterLevelNoWaves = reinterpret_cast<decltype(CWaterLevel::GetWaterLevelNoWaves)>(0x5C2BE0);

    RwIm3DTransform = (decltype(RwIm3DTransform))0x65AE90;
    RwIm3DRenderIndexedPrimitive = (decltype(RwIm3DRenderIndexedPrimitive))0x65AF90;
    RwIm3DEnd = (decltype(RwIm3DEnd))0x65AF60;

    CGame::currArea.SetAddress((int*)0x978810);

    CWorld::FindGroundZFor3DCoordCR = (decltype(CWorld::FindGroundZFor3DCoordCR))0x4D53A0;
    CClock::GetIsTimeInRange = (decltype(CClock::GetIsTimeInRange))0x4870F0;

    CClock::ms_nGameClockHours.SetAddress((uint8_t*)0xA10B6B);
    CClock::ms_nGameClockMinutes.SetAddress((uint8_t*)0xA10B92);

    CTimeCycle::m_fCurrentFarClip.SetAddress((float*)0x9B6A6C);
    CTimeCycle::m_fCurrentFogStart.SetAddress((float*)0x978660);

    CModelInfo::pp_modelInfoPtrs = (CBaseModelInfo***)(0x406557 + 3);

    CModelInfo::Get2dEffect = (decltype(CModelInfo::Get2dEffect))0x53F260;

    CRenderer::ms_lodDistScale.SetAddress((float*)0x690220);

    CTimeCycle::m_fCurrentSpriteBrightness.SetAddress((float*)0x9B6A70);
    CTimeCycle::m_fCurrentSpriteSize.SetAddress((float*)0x978610);
    GameLights::FOV.SetAddress((float*)0x696658);
    RwRasterCreate = (decltype(RwRasterCreate))0x655490;
    RwRasterLock = (decltype(RwRasterLock))0x655530;
    RwRasterUnlock = (decltype(RwRasterUnlock))0x655250;

    RwRenderStateSet = (decltype(RwRenderStateSet))0x649BA0;
    RwRenderStateGet = (decltype(RwRenderStateGet))0x649BF0;

    RwIm2DRenderIndexedPrimitive = (decltype(RwIm2DRenderIndexedPrimitive))0x649C20;
    RwIm2DRenderPrimitive = (decltype(RwIm2DRenderPrimitive))0x649C10;

    pHelis = (CHeli**)0x813D10;
    pNumRandomHelis = (int16_t*)0xA10A6A;

    CPointLights::AddLightWithoutEntity = (decltype(CPointLights::AddLightWithoutEntity))0x567700;

    AddParticle = (decltype(AddParticle))0x5648F0;

    CTheZones::GetZoneInfoForTimeOfDay = (decltype(CTheZones::GetZoneInfoForTimeOfDay))0x4DC500;

    BuildingSwapArray = reinterpret_cast<CBuildingSwap*>(0x94DBE8);
}

void Init()
{
    ReadSettings();
    GetMemoryAddresses();
    ApplyMemoryPatches();
}

ASI_SETTINGS_EXPORT(Settings())

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 C5 59 85 ED 59 74"));
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
