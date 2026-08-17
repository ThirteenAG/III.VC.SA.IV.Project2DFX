#define NOMINMAX
#include "stdafx.h"
#include <ranges>
#include <deque>
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
    { "indhelix_barrier", "lod_land014" },
    { "nbbridgcabls01", "nbbridgfk2" },
    { "nbbridgerdb", "damgbridgerdb" },
    { "nbbridgerda", "damgbbridgerda" },
    { "lodridgcabls01", "lodridgfk2" },
    { "lodridgerdb", "lodgbridgerdb" },
    { "lodridgerda", "lodgbbrridgerda" },
    { "fishfctory", "fshfctry_dstryd" },
    { "rustship_structure", "lod_land014" },
    { "boatramp1", "lod_land014" },
    { "police_celhole", "police_cell_wall" },
    { "convstore01", "convstre_dmge01" },
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

// Install per-model corona predicates for every swap pair: a model stays
// visible only while the game has not swapped it out. Called once before
// LoadDatFile, when model info is already available.
void SetupSwapCoronaPredicates()
{
    for (const auto& pair : aModelSwaps)
    {
        for (const char* szName : { pair.first, pair.second })
        {
            int nID = -1;
            CModelInfo::GetModelInfo(szName, &nID);

            CCoronaVisibility::SetModelPredicate((std::string("%") + szName).c_str(), [nID]() -> bool
            {
                return nID < 0 || !IsModelSwappedOut(nID);
            });
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
    }
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

    SafetyHookInline shRepositionCertainDynamicObjects = {};
    void __cdecl RepositionCertainDynamicObjects()
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

void DrawDistanceFMUL(SafetyHookContext& ctx)
{
    float f = fDrawDistance;
    _asm {fmul dword ptr[f]}
}

void ApplyMemoryPatches()
{
    auto pattern = hook::pattern("E8 ? ? ? ? 59 53 E8 ? ? ? ? 59 81 C4 ? ? ? ? 5D");
    CWorld::hbAdd.fun = injector::MakeCALL(pattern.get_first(), CWorld::Add, true).get();

    pattern = hook::pattern("E8 ? ? ? ? 66 8B 45 ? 59");
    CWorld::hbAdd.fun = injector::MakeCALL(pattern.get_first(), CWorld::Add, true).get();

    pattern = hook::pattern("E8 ? ? ? ? 6A ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8");
    CWorld::shRepositionCertainDynamicObjects = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CWorld::RepositionCertainDynamicObjects);

    pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? 66 C7 05 ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 8D 44 20");
    static auto CMovingThingsInitHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CMovingThings::InitDistantCarImpostors();
    });

    pattern = hook::pattern("01 C2 89 C8 01 D6");
    static auto CMovingThingsUpdateHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CLODLights::RegisterLODLights();
        CMovingThings::UpdateDistantCarImpostors();
    });

    pattern = hook::pattern("BE B0 F6 62 00");
    static auto CMovingThingsRenderHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
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

        if (bRenderLodLights)
            CMovingThings::RenderDistantCarImpostors();
    });

    pattern = hook::pattern("C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? C6 04 C5");
    static auto CMovingThingsShutdownHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        CMovingThings::ShutdownDistantCarImpostors();
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
        pattern = hook::pattern("D8 0D ? ? ? ? D9 9B ? ? ? ? 80 7B");
        static auto CRendererms_lodDistScaleHook = safetyhook::create_mid(pattern.get_first(), DrawDistanceFMUL);
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

    CWeather::Rain.SetAddress((float*)0x8E2BFC);
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

    CShadows::gpShadowExplosionTex = (RwTexture**)0x8F2A00;
    CShadows::StoreStaticShadow = (decltype(CShadows::StoreStaticShadow))0x5130A0;

    RwRenderStateSet = (decltype(RwRenderStateSet))0x5A43C0;
    RwRenderStateGet = (decltype(RwRenderStateGet))0x5A4410;

    pHelis = (CHeli**)0x72CF50;
    pNumRandomHelis = (int16_t*)0x95CCAA;

    CPointLights::AddLightWithoutEntity = (decltype(CPointLights::AddLightWithoutEntity))0x510790;

    AddParticle = (decltype(AddParticle))0x50D140;

    CTheZones::GetZoneInfoForTimeOfDay = (decltype(CTheZones::GetZoneInfoForTimeOfDay))0x4B6FB0;

    CStats::IndustrialPassed.SetAddress(0x8E2A68);

    BuildingSwapArray = reinterpret_cast<CBuildingSwap*>(0x880E30);
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? ? ? 68"));
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
