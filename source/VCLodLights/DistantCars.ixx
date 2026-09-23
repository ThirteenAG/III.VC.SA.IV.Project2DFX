module;

#include <stdafx.h>
#include "../DistantTraffic.hpp"

export module DistantCars;

import ComVars;
import Misc;
import Timer;
import Camera;
import Clock;
import Timecycle;
import LODLights;
import DistantCarRenderer;

using uint32 = uint32_t;
using int32 = int32_t;
using uint16 = uint16_t;
using int16 = int16_t;
using uint8 = uint8_t;
using int8 = int8_t;

#define Max(a, b) ((a) > (b) ? (a) : (b))
#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Clamp(v, low, high) ((v) < (low) ? (low) : (v) > (high) ? (high) : \
                                                                  (v))
constexpr auto SQR = [](auto x) { return x * x; };


enum
{
    GANG_CUBAN = 0,
    GANG_HAITIAN,
    GANG_STREET,
    GANG_DIAZ,
    GANG_SECURITY,
    GANG_BIKER,
    GANG_PLAYER,
    GANG_GOLFER,
    GANG_9,
    NUM_GANGS
};

enum eCarClass
{
    NORMAL = 0,
    POOR,
    RICH,
    EXEC,
    WORKER,
    BIG,
    TAXI,
    MOPED,
    MOTORBIKE,

    LEISUREBOAT,
    WORKERBOAT,

    COPS,
    CUBAN,
    HAITIAN,
    STREET,
    DIAZ,
    BIKER,
    SECURITY,
    PLAYER,
    GOLFERS,
    GANG9,
    COPS_BOAT,
    FIRST_CAR_RATING = NORMAL,
    FIRST_BOAT_RATING = LEISUREBOAT,
    FIRST_GANG_CAR_RATING = CUBAN,
    NUM_CAR_CLASSES = MOTORBIKE - FIRST_CAR_RATING + 1,
    NUM_BOAT_CLASSES = WORKERBOAT - FIRST_BOAT_RATING + 1,
    NUM_GANG_CAR_CLASSES = GANG9 - FIRST_GANG_CAR_RATING + 1,
    TOTAL_CUSTOM_CLASSES = NUM_CAR_CLASSES + NUM_BOAT_CLASSES
};

class CZoneInfo
{
public:
    // Car data
    int16 carDensity;
    int16 carThreshold[NUM_CAR_CLASSES];
    int16 boatThreshold[NUM_BOAT_CLASSES];
    int16 gangThreshold[NUM_GANGS];
    int16 copThreshold;

    // Ped data
    uint16 pedDensity;
    uint16 gangPedThreshold[NUM_GANGS];
    uint16 copPedThreshold;
    uint16 pedGroup;
};

export namespace CTheZones
{
    void (__cdecl* GetZoneInfoForTimeOfDay)(CVector* point, CZoneInfo* info) = nullptr;
}

constexpr auto LANE_WIDTH = 5.0f;
constexpr auto WIDTH_TO_PED_NODE_WIDTH = (31.f / (500.f * 8.f));

struct CPathNode
{
    int16_t prevIndex;
    int16_t nextIndex;
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t distance; // in path search
    int16_t firstLink;
    uint8_t width;
    int8_t group;

    uint8_t numLinks : 4;
    uint8_t bDeadEnd : 1;
    uint8_t bDisabled : 1;
    uint8_t bBetweenLevels : 1;
    uint8_t bUseInRoadBlock : 1;

    uint8_t bWaterPath : 1;
    uint8_t bOnlySmallBoats : 1;
    uint8_t bSelected : 1;
    uint8_t speedLimit : 2;
    //uint8_t flagB20 : 1;
    //uint8_t flagB40 : 1;
    //uint8_t flagB80 : 1;

    uint8_t spawnRate : 4;
    uint8_t flagsC : 4;

    CVector GetPosition(void) { return CVector(x / 8.0f, y / 8.0f, z / 8.0f); }
    void SetPosition(const CVector& p)
    {
        x = p.x * 8.0f;
        y = p.y * 8.0f;
        z = p.z * 8.0f;
    }
    float GetX(void) { return x / 8.0f; }
    float GetY(void) { return y / 8.0f; }
    float GetZ(void) { return z / 8.0f; }
    bool HasDivider(void) { return width != 0; }
    float GetDividerWidth(void) { return width / (2 * 8.0f); }
    float GetPedNodeWidth(void) { return width * WIDTH_TO_PED_NODE_WIDTH; }
    CPathNode* GetPrev(void);
    CPathNode* GetNext(void);
    void SetPrev(CPathNode* node);
    void SetNext(CPathNode* node);
};

struct CCarPathLink
{
    int16_t x;
    int16_t y;
    int16_t pathNodeIndex;
    int8_t dirX;
    int8_t dirY;
    int8_t numLeftLanes : 3;
    int8_t numRightLanes : 3;
    uint8_t trafficLightDirection : 1;
    uint8_t trafficLightType : 2;
    uint8_t bBridgeLights : 1;
    uint8_t width;

    CVector2D GetPosition(void) { return CVector2D(x / 8.0f, y / 8.0f); }
    CVector2D GetDirection(void) { return CVector2D(dirX / 100.0f, dirY / 100.0f); }
    float GetX(void) { return x / 8.0f; }
    float GetY(void) { return y / 8.0f; }
    float GetDirX(void) { return dirX / 100.0f; }
    float GetDirY(void) { return dirY / 100.0f; }
    float GetLaneOffset(void) { return width / (2 * 8.0f * LANE_WIDTH); }

    float OneWayLaneOffset()
    {
        if (numLeftLanes == 0)
            return 0.5f - 0.5f * numRightLanes;
        if (numRightLanes == 0)
            return 0.5f - 0.5f * numLeftLanes;
        return 0.5f + GetLaneOffset();
    }
};

using CTreadable = void;

union CConnectionFlags
{
    uint8_t flags;
    struct
    {
        uint8_t bCrossesRoad : 1;
        uint8_t bTrafficLight : 1;
    };
};

constexpr auto NUM_PATHNODES = 9650;
constexpr auto NUM_CARPATHLINKS = 3500;
constexpr auto NUM_MAPOBJECTS = 1250;
constexpr auto NUM_PATHCONNECTIONS = 20400;

class CPathFind
{
public:
    CPathNode m_pathNodes[NUM_PATHNODES];
    CCarPathLink m_carPathLinks[NUM_CARPATHLINKS];
    CTreadable* m_mapObjects[NUM_MAPOBJECTS];
    uint16 m_connections[NUM_PATHCONNECTIONS]; // and flags
    uint8 m_distances[NUM_PATHCONNECTIONS];
    int16 m_carPathConnections[NUM_PATHCONNECTIONS];

    int32 m_numPathNodes;
    int32 m_numCarPathNodes;
    int32 m_numPedPathNodes;
    int16 m_numMapObjects;
    int16 m_numConnections;
    int32 m_numCarPathLinks;
    int32 unk;
    uint8 m_numGroups[2];
    CPathNode m_searchNodes[512];

    void Init(void);
    void AllocatePathFindInfoMem(int16 numPathGroups);
    void RegisterMapObject(CTreadable* mapObject);
    void StoreNodeInfoPed(int16 id, int16 node, int8 type, int8 next, int16 x, int16 y, int16 z, float width, bool crossing, uint8 spawnRate);
    void StoreNodeInfoCar(int16 id, int16 node, int8 type, int8 next, int16 x, int16 y, int16 z, float width, int8 numLeft, int8 numRight,
        bool disabled, bool betweenLevels, uint8 speedLimit, bool roadBlock, bool waterPath, uint8 spawnRate);
    void StoreDetachedNodeInfoPed(int32 node, int8 type, int32 next, float x, float y, float z, float width, bool crossing,
        bool disabled, bool betweenLevels, uint8 spawnRate);
    void StoreDetachedNodeInfoCar(int32 node, int8 type, int32 next, float x, float y, float z, float width, int8 numLeft, int8 numRight,
        bool disabled, bool betweenLevels, uint8 speedLimit, bool roadBlock, bool waterPath, uint8 spawnRate, bool unk);
    void CalcNodeCoors(float x, float y, float z, int32 id, CVector* out);
    bool LoadPathFindData(void);
    void PreparePathData(void);
    void CountFloodFillGroups(uint8 type);
    //void PreparePathDataForType(uint8 type, CTempNode* tempnodes, CPathInfoForObject* objectpathinfo,
    //    float maxdist, CPathInfoForObject* detachednodes, int32 numDetached);

    //bool IsPathObject(int id) { return id < PATHNODESIZE && (InfoForTileCars[id * 12].type != 0 || InfoForTilePeds[id * 12].type != 0); }

    float CalcRoadDensity(float x, float y);
    bool TestForPedTrafficLight(CPathNode* n1, CPathNode* n2);
    bool TestCrossesRoad(CPathNode* n1, CPathNode* n2);
    void AddNodeToList(CPathNode* node, int32 listId);
    void RemoveNodeFromList(CPathNode* node);
    void RemoveBadStartNode(CVector pos, CPathNode** nodes, int16* n);
    void SetLinksBridgeLights(float, float, float, float, bool);
    void SwitchOffNodeAndNeighbours(int32 nodeId, bool disable);
    void SwitchRoadsOffInArea(float x1, float x2, float y1, float y2, float z1, float z2, bool disable);
    void SwitchPedRoadsOffInArea(float x1, float x2, float y1, float y2, float z1, float z2, bool disable);
    void SwitchRoadsInAngledArea(float x1, float y1, float z1, float x2, float y2, float z2, float length, uint8 type, uint8 enable);
    void MarkRoadsBetweenLevelsNodeAndNeighbours(int32 nodeId);
    void MarkRoadsBetweenLevelsInArea(float x1, float x2, float y1, float y2, float z1, float z2);
    void PedMarkRoadsBetweenLevelsInArea(float x1, float x2, float y1, float y2, float z1, float z2);
    int32 FindNodeClosestToCoors(CVector coors, uint8 type, float distLimit, bool ignoreDisabled = false, bool ignoreBetweenLevels = false, bool ignoreSelected = false, bool bWaterPath = false);
    int32 FindNodeClosestToCoorsFavourDirection(CVector coors, uint8 type, float dirX, float dirY);
    void FindNodePairClosestToCoors(CVector coors, uint8 type, int* node1, int* node2, float* angle, float minDist, float maxDist, bool ignoreDisabled = false, bool ignoreBetweenLevels = false, bool bWaterPath = false);
    int32 FindNthNodeClosestToCoors(CVector coors, uint8 type, float distLimit, bool ignoreDisabled, bool ignoreBetweenLevels, int N, bool bWaterPath = false);
    CVector FindNodeCoorsForScript(int32 id);
    float FindNodeOrientationForCarPlacement(int32 nodeId);
    float FindNodeOrientationForCarPlacementFacingDestination(int32 nodeId, float x, float y, bool towards);
    bool GenerateCarCreationCoors(float x, float y, float dirX, float dirY, float spawnDist, float angleLimit, bool forward, CVector* pPosition, int32* pNode1, int32* pNode2, float* pPositionBetweenNodes, bool ignoreDisabled = false);
    bool GeneratePedCreationCoors(float x, float y, float minDist, float maxDist, float minDistOffScreen, float maxDistOffScreen, CVector* pPosition, int32* pNode1, int32* pNode2, float* pPositionBetweenNodes, CMatrix* camMatrix);
    void FindNextNodeWandering(uint8, CVector, CPathNode**, CPathNode**, uint8, uint8*);
    void DoPathSearch(uint8 type, CVector start, int32 startNodeId, CVector target, CPathNode** nodes, int16* numNodes, int16 maxNumNodes, void* vehicle, float* dist, float distLimit, int32 forcedTargetNode);
    bool TestCoorsCloseness(CVector target, uint8 type, CVector start);
    void Save(uint8* buf, uint32* size);
    void Load(uint8* buf, uint32 size);

    static CVector TakeWidthIntoAccountForWandering(CPathNode*, uint16);
    static void TakeWidthIntoAccountForCoors(CPathNode*, CPathNode*, uint16, float*, float*);

    CPathNode* GetNode(int16 index);
    int16 GetIndex(CPathNode* node);

    uint16 ConnectedNode(int id) { return m_connections[id] & 0x3FFF; }
    bool ConnectionCrossesRoad(int id) { return !!(m_connections[id] & 0x8000); }
    bool ConnectionHasTrafficLight(int id) { return !!(m_connections[id] & 0x4000); }
    void ConnectionSetTrafficLight(int id) { m_connections[id] |= 0x4000; }
};

export GameRef<CPathFind> ThePaths([]() -> CPathFind*
{
    auto pattern = hook::pattern("B9 ? ? ? ? 50 6A ? FF 35");
    if (!pattern.empty())
        return *pattern.get_first<CPathFind*>(1);
    return nullptr;
});

namespace CCarCtrl
{
    export GameRef<float> CarDensityMultiplier([]() -> float*
    {
        auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? DB 05");
        if (!pattern.empty())
            return *pattern.get_first<float*>(2);
        return nullptr;
    });
}

struct TrafficGraph
{
    using Node = DistantTraffic::Node;
    using Edge = DistantTraffic::Edge;
    static bool NodeValid(Node id)
    {
        auto&& paths = *ThePaths.get_ptr();
        return id < static_cast<unsigned>(paths.m_numCarPathNodes) && !paths.m_pathNodes[id].bDisabled && paths.m_pathNodes[id].GetPosition().z < 500.0f;
    }
    static bool Valid(const Edge& edge) { return NodeValid(edge.from) && NodeValid(edge.to); }
    static unsigned Degree(Node id)
    {
        auto&& paths = *ThePaths.get_ptr();
        return NodeValid(id) ? paths.m_pathNodes[id].numLinks : 0;
    }
    static Node RandomNode(uint32_t random)
    {
        auto&& paths = *ThePaths.get_ptr();
        int count = paths.m_numCarPathNodes;
        return count > 0 ? random % count : DistantTraffic::InvalidNode;
    }
    static size_t Outgoing(Node from, std::array<Edge, 16>& result)
    {
        if (!NodeValid(from))
            return 0;
        auto&& paths = *ThePaths.get_ptr();
        auto& a = paths.m_pathNodes[from];
        size_t count = 0;
        for (int i = 0; i < a.numLinks && count < result.size(); ++i)
        {
            int connection = a.firstLink + i;
            if (connection < 0 || connection >= paths.m_numConnections)
                continue;
            Node to = (paths.m_connections[connection] & 0x3FFF);
            if (!NodeValid(to) || to == from)
                continue;
            int linkIndex = paths.m_carPathConnections[connection];
            if (linkIndex < 0 || linkIndex >= paths.m_numCarPathLinks)
                continue;
            auto& link = paths.m_carPathLinks[linkIndex];
            // CarCtrl::GenerateOneRandomCar uses the attached endpoint, not a
            // dot product, to choose which lane count permits this traversal.
            int lanes = link.pathNodeIndex == to ? link.numLeftLanes : link.numRightLanes;
            if (lanes <= 0)
                continue;
            auto& b = paths.m_pathNodes[to];
            if (a.bWaterPath != b.bWaterPath || (a.bWaterPath && !bDistantMaritimeTraffic))
                continue;
            CVector start = a.GetPosition(), end = b.GetPosition(), segment = end - start;
            float length = segment.Magnitude2D();
            if (length < 1.0f)
                continue;
            CVector2D linkPosition = link.GetPosition(), linkDirection = link.GetDirection();
            // re3/reVC CarCtrl's m_nNextDirection convention.
            float sign = from >= to ? 1.0f : -1.0f;
            CVector direction(linkDirection.x * sign, linkDirection.y * sign, segment.z / length);
            direction.Normalise();
            float t = (std::clamp)(((linkPosition.x - start.x) * segment.x + (linkPosition.y - start.y) * segment.y) / (length * length), 0.0f, 1.0f);
            Edge edge;
            edge.from = from;
            edge.to = to;
            edge.lanes = static_cast<unsigned>(lanes);
            edge.position = {linkPosition.x, linkPosition.y, start.z + segment.z * t};
            edge.direction = direction;
            edge.laneOffset = link.OneWayLaneOffset();
            edge.water = (a.bWaterPath || b.bWaterPath);
            edge.speed = edge.water ? 8.0f : 16.0f;
            result[count++] = edge;
        }
        return count;
    }
    static bool SpawnAllowed(const Edge& edge)
    {
        if (edge.water)
            return bDistantMaritimeTraffic;
        if (!CTheZones::GetZoneInfoForTimeOfDay)
            return true;
        CVector position = edge.position;
        CZoneInfo info{};
        CTheZones::GetZoneInfoForTimeOfDay(&position, &info);
        return info.carDensity > 0;
    }
    static void Hide(uint32_t id)
    {
        DistantCarRenderer::HideLights(id);
        CLODLights::UnregisterCorona(id);
        CLODLights::UnregisterCorona(0x7E000000u + (id - 0x7F000000u));
        CLODLights::UnregisterCorona(0x7D000000u + (id - 0x7F000000u));
    }
};

using TrafficSimulation = DistantTraffic::Simulation<TrafficGraph, DistantCarRenderer::State>;
static TrafficSimulation traffic;
export class CMovingThings
{
  public:
    using CDistantCarImpostor = DistantTraffic::Car<DistantCarRenderer::State>;
    static std::vector<CDistantCarImpostor>& aDistantCarImpostors;
    static void InitDistantCarImpostors()
    {
        traffic.Clear();
        traffic.Reserve(static_cast<size_t>((std::clamp)(nNumDistantCarImpostors, 0, 10000)));
    }
    static void ShutdownDistantCarImpostors() { traffic.Clear(); }
    static void UpdateDistantCarImpostors();
    static void RenderDistantCarImpostors();
};
std::vector<CMovingThings::CDistantCarImpostor>& CMovingThings::aDistantCarImpostors = traffic.cars;
static uint32 ImpostorPortSideCoronaId(int32 i)
{
    return 0x7E000000u + i;
}
static uint32 ImpostorStarboardSideCoronaId(int32 i)
{
    return 0x7D000000u + i;
}

void CMovingThings::UpdateDistantCarImpostors()
{
    CVector camera = TheCamera->GetPosition();
    float farClip = CTimeCycle::m_fCurrentFarClip;

    float density = (std::clamp)(static_cast<float>(CCarCtrl::CarDensityMultiplier), 0.0f, 1.0f);
    int hour = CClock::ms_nGameClockHours;
    if (hour <= 5)
        density *= .65f;
    density *= 1.0f - .22f * static_cast<float>(CWeather::Rain);
    density *= 1.0f - .16f * static_cast<float>(CWeather::Foggyness);
    traffic.Update(CTimer::GetTimeStepInSeconds(), static_cast<size_t>((std::clamp)(nNumDistantCarImpostors, 0, 10000)), density, camera, farClip);
}

void CMovingThings::RenderDistantCarImpostors()
{
    if (nNumDistantCarImpostors <= 0 || aDistantCarImpostors.empty())
        return;

    CVector camPos = TheCamera->GetPosition();
    auto maxDist = CTimeCycle::m_fCurrentFarClip;
    DistantCarRenderer::Frame models(camPos, maxDist);

    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
    {
        CDistantCarImpostor& impostor = aDistantCarImpostors[i];
        if (!impostor.m_bActive)
            continue;

        if (impostor.m_bWaterNode && !bDistantMaritimeTraffic)
            continue;

        CVector toImpostor = traffic.RenderPosition(impostor) - camPos;
        float distSqr = toImpostor.MagnitudeSqr2D();
        if (distSqr < SQR(140.0f) || distSqr > SQR(maxDist))
            continue;

        float dist = Sqrt(distSqr);
        if (dist < 0.001f)
            continue;

        float dirDot = DotProduct(traffic.RenderDirection(impostor), camPos - traffic.RenderPosition(impostor));
        bool approaching = dirDot > 0.0f;

        uint8 red, green, blue;
        if (impostor.m_bWaterNode)
        {
            // Boats show an all-round white masthead/stern light (warm white).
            red = 255;
            green = 242;
            blue = 218;
        }
        else
        {
            // Road vehicle: white headlights (approaching), red tail lights (receding)
            red = 255;
            green = approaching ? 255 : 40;
            blue = approaching ? 230 : 40;
        }

        float fadeFar = Clamp((maxDist - dist) / 250.0f, 0.0f, 1.0f);
        float fadeNear = Clamp((dist - 140.0f) / 120.0f, 0.0f, 1.0f);
        float fade = fadeFar * fadeNear;

        if (models.Add(traffic.RenderPosition(impostor), traffic.RenderDirection(impostor), impostor.m_visual, impostor.m_nCoronaId, fade, impostor.m_bWaterNode))
            continue;

        float size = 4.0f * fDistantCarsRadiusMultiplier;
        fade *= impostor.m_visual.fade;
        uint8 alpha = (uint8)(150 * fade);

        // Boat navigation lights: compute the camera bearing relative to the
        // boat heading (fwd = cos, cross > 0 -> camera on the port side).
        uint8 sideAlpha = 0;
        float boatCross = 0.0f, sectorBlend = 0.0f;
        CVector portPos, stbdPos;
        if (impostor.m_bWaterNode)
        {
            float toCamX = camPos.x - traffic.RenderPosition(impostor).x;
            float toCamY = camPos.y - traffic.RenderPosition(impostor).y;
            float toCamLen = Sqrt(toCamX * toCamX + toCamY * toCamY);
            if (toCamLen > 0.001f)
            {
                float fwd = (traffic.RenderDirection(impostor).x * toCamX + traffic.RenderDirection(impostor).y * toCamY) / toCamLen;
                boatCross = traffic.RenderDirection(impostor).x * toCamY - traffic.RenderDirection(impostor).y * toCamX;

                // Sidelights cover from dead ahead to 22.5° abaft the beam.
                constexpr float SECTOR_EDGE = -0.38f; // cos(112.5°)
                constexpr float EDGE_RAMP = 0.15f;
                sectorBlend = Clamp((fwd - SECTOR_EDGE) / EDGE_RAMP, 0.0f, 1.0f);

                // White light: bright masthead in the forward arc, dimmer
                // stern light astern.
                size = 2.6f * fDistantCarsRadiusMultiplier;
                alpha = (uint8)((70.0f + 40.0f * sectorBlend) * fade);
                sideAlpha = (uint8)(180.0f * fade * sectorBlend);

                // Port/starboard light positions, offset to each side of the
                // bow so red and green are clearly separated and never wash
                // out against the white masthead light.
                const float beamHalf = 2.0f;
                portPos = traffic.RenderPosition(impostor) +
                          CVector(-traffic.RenderDirection(impostor).y * beamHalf, traffic.RenderDirection(impostor).x * beamHalf, 0.0f);
                stbdPos = traffic.RenderPosition(impostor) +
                          CVector(traffic.RenderDirection(impostor).y * beamHalf, -traffic.RenderDirection(impostor).x * beamHalf, 0.0f);
            }
        }
        if (alpha == 0 && sideAlpha == 0)
            continue;

        if (alpha)
        {
            CLODLights::RegisterCorona(impostor.m_nCoronaId, nullptr, red, green, blue, alpha, traffic.RenderPosition(impostor), size, maxDist, 1, 0, false,
                                       false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
        }

        if (impostor.m_bWaterNode && sideAlpha)
        {
            uint32 baseId = impostor.m_nCoronaId - 0x7F000000u;

            // Red port light — camera anywhere on the port side of the bow.
            if (boatCross >= 0.0f)
            {
                CLODLights::RegisterCorona(ImpostorPortSideCoronaId((int32)baseId), nullptr, 255, 30, 30, sideAlpha, portPos,
                                           2.4f * fDistantCarsRadiusMultiplier, maxDist, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
            }

            // Green starboard light.
            if (boatCross <= 0.0f)
            {
                CLODLights::RegisterCorona(ImpostorStarboardSideCoronaId((int32)baseId), nullptr, 30, 255, 60, sideAlpha, stbdPos,
                                           2.4f * fDistantCarsRadiusMultiplier, maxDist, 1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
            }
        }
    }
    models.Flush();
}