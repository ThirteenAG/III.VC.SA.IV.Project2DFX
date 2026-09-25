module;

#include <stdafx.h>
#include "../DistantTraffic.hpp"
#include "WaterPaths.hpp"

#include <Facade.hpp>

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
    GANG_MAFIA = 0,
    GANG_TRIAD,
    GANG_DIABLOS,
    GANG_YAKUZA,
    GANG_YARDIE,
    GANG_COLUMB,
    GANG_HOODS,
    GANG_7,
    GANG_8,
    NUM_GANGS
};

class CZoneInfo
{
public:
    // Car data
    int16 carDensity;
    int16 carThreshold[6];
    int16 copThreshold;
    int16 gangThreshold[NUM_GANGS];

    // Ped data
    uint16 pedDensity;
    uint16 copDensity;
    uint16 gangDensity[NUM_GANGS];
    uint16 pedGroup;
};

export namespace CTheZones
{
    void (__cdecl* GetZoneInfoForTimeOfDay)(CVector* point, CZoneInfo* info) = nullptr;
}

struct CPathNode
{
    CVector pos;
    CPathNode* prev;
    CPathNode* next;
    int16_t distance;
    int16_t objectIndex;
    int16_t firstLink;
    uint8_t numLinks;

    uint8_t unkBits : 2;
    uint8_t bDeadEnd : 1;
    uint8_t bDisabled : 1;
    uint8_t bBetweenLevels : 1;

    int8_t group;

    CVector& GetPosition(void) { return pos; }
    void SetPosition(const CVector& p) { pos = p; }
    float GetX(void) { return pos.x; }
    float GetY(void) { return pos.y; }
    float GetZ(void) { return pos.z; }

    CPathNode* GetPrev(void) { return prev; }
    CPathNode* GetNext(void) { return next; }
    void SetPrev(CPathNode* node) { prev = node; }
    void SetNext(CPathNode* node) { next = node; }
};

struct CCarPathLink
{
    CVector2D pos;
    CVector2D dir;
    int16_t pathNodeIndex;
    int8_t numLeftLanes;
    int8_t numRightLanes;
    uint8_t trafficLightType;

    uint8_t bBridgeLights : 1;

    CVector2D& GetPosition(void) { return pos; }
    CVector2D& GetDirection(void) { return dir; }
    float GetX(void) { return pos.x; }
    float GetY(void) { return pos.y; }
    float GetDirX(void) { return dir.x; }
    float GetDirY(void) { return dir.y; }

    float OneWayLaneOffset()
    {
        if (numLeftLanes == 0)
            return 0.5f - 0.5f * numRightLanes;
        if (numRightLanes == 0)
            return 0.5f - 0.5f * numLeftLanes;
        return 0.5f;
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

constexpr auto NUM_PATHNODES = 4930;
constexpr auto NUM_CARPATHLINKS = 2076;
constexpr auto NUM_MAPOBJECTS = 1250;
constexpr auto NUM_PATHCONNECTIONS = 10260;

class CPathFind
{
public:
    CPathNode m_pathNodes[NUM_PATHNODES];
    CCarPathLink m_carPathLinks[NUM_CARPATHLINKS];
    CTreadable* m_mapObjects[NUM_MAPOBJECTS];
    uint8_t m_objectFlags[NUM_MAPOBJECTS];
    int16_t m_connections[NUM_PATHCONNECTIONS];
    int16_t m_distances[NUM_PATHCONNECTIONS];
    CConnectionFlags m_connectionFlags[NUM_PATHCONNECTIONS];
    int16_t m_carPathConnections[NUM_PATHCONNECTIONS];

    int32_t m_numPathNodes;
    int32_t m_numCarPathNodes;
    int32_t m_numPedPathNodes;
    int16_t m_numMapObjects;
    int16_t m_numConnections;
    int32_t m_numCarPathLinks;
    int32_t unk;
    uint8_t m_numGroups[2];
    CPathNode m_searchNodes[512];

    void Init(void);
    void AllocatePathFindInfoMem(int16_t numPathGroups);
    void RegisterMapObject(CTreadable* mapObject);
    void StoreNodeInfoPed(int16_t id, int16_t node, int8_t type, int8_t next, int16_t x, int16_t y, int16_t z, int16_t width, bool crossing);
    void StoreNodeInfoCar(int16_t id, int16_t node, int8_t type, int8_t next, int16_t x, int16_t y, int16_t z, int16_t width, int8_t numLeft, int8_t numRight);
    void CalcNodeCoors(int16_t x, int16_t y, int16_t z, int32_t id, CVector* out);
    bool LoadPathFindData(void);
    void PreparePathData(void);
    void CountFloodFillGroups(uint8_t type);
    //void PreparePathDataForType(uint8_t type, CTempNode* tempnodes, CPathInfoForObject* objectpathinfo,
    //    float maxdist, CTempDetachedNode* detachednodes, int32 numDetached);

    //bool IsPathObject(int id) { return id < PATHNODESIZE && (InfoForTileCars[id * 12].type != 0 || InfoForTilePeds[id * 12].type != 0); }

    float CalcRoadDensity(float x, float y);
    bool TestForPedTrafficLight(CPathNode* n1, CPathNode* n2);
    bool TestCrossesRoad(CPathNode* n1, CPathNode* n2);
    void AddNodeToList(CPathNode* node, int32_t listId);
    void RemoveNodeFromList(CPathNode* node);
    void RemoveBadStartNode(CVector pos, CPathNode** nodes, int16_t* n);
    void SetLinksBridgeLights(float, float, float, float, bool);
    void SwitchOffNodeAndNeighbours(int32_t nodeId, bool disable);
    void SwitchRoadsOffInArea(float x1, float x2, float y1, float y2, float z1, float z2, bool disable);
    void SwitchPedRoadsOffInArea(float x1, float x2, float y1, float y2, float z1, float z2, bool disable);
    void SwitchRoadsInAngledArea(float x1, float y1, float z1, float x2, float y2, float z2, float length, uint8_t type, uint8_t enable);
    void MarkRoadsBetweenLevelsNodeAndNeighbours(int32_t nodeId);
    void MarkRoadsBetweenLevelsInArea(float x1, float x2, float y1, float y2, float z1, float z2);
    void PedMarkRoadsBetweenLevelsInArea(float x1, float x2, float y1, float y2, float z1, float z2);
    int32_t FindNodeClosestToCoors(CVector coors, uint8_t type, float distLimit, bool ignoreDisabled = false, bool ignoreBetweenLevels = false);
    int32_t FindNodeClosestToCoorsFavourDirection(CVector coors, uint8_t type, float dirX, float dirY);
    float FindNodeOrientationForCarPlacement(int32_t nodeId);
    float FindNodeOrientationForCarPlacementFacingDestination(int32_t nodeId, float x, float y, bool towards);
    bool NewGenerateCarCreationCoors(float x, float y, float dirX, float dirY, float spawnDist, float angleLimit, bool forward, CVector* pPosition, int32_t* pNode1, int32_t* pNode2, float* pPositionBetweenNodes, bool ignoreDisabled = false);
    bool GeneratePedCreationCoors(float x, float y, float minDist, float maxDist, float minDistOffScreen, float maxDistOffScreen, CVector* pPosition, int32_t* pNode1, int32_t* pNode2, float* pPositionBetweenNodes, CMatrix* camMatrix);
    CTreadable* FindRoadObjectClosestToCoors(CVector coors, uint8_t type);
    void FindNextNodeWandering(uint8_t, CVector, CPathNode**, CPathNode**, uint8_t, uint8_t*);
    void DoPathSearch(uint8_t type, CVector start, int32_t startNodeId, CVector target, CPathNode** nodes, int16_t* numNodes, int16_t maxNumNodes, void* vehicle, float* dist, float distLimit, int32_t forcedTargetNode);
    bool TestCoorsCloseness(CVector target, uint8_t type, CVector start);
    void Save(uint8_t* buf, uint32_t* size);
    void Load(uint8_t* buf, uint32_t size);
    uint16_t ConnectedNode(int id) { return m_connections[id]; }
    bool ConnectionCrossesRoad(int id) { return m_connectionFlags[id].bCrossesRoad; }
    bool ConnectionHasTrafficLight(int id) { return m_connectionFlags[id].bTrafficLight; }
    void ConnectionSetTrafficLight(int id) { m_connectionFlags[id].bTrafficLight = true; }

    void DisplayPathData(void);
};

class CPathFindFacade
{
public:
    CPathFindFacade(CPathFind* obj)
        : FACADE_INIT_MEMBER(obj, m_pathNodes)
        , FACADE_INIT_MEMBER(obj, m_carPathLinks)
        , FACADE_INIT_MEMBER(obj, m_mapObjects)
        , FACADE_INIT_MEMBER(obj, m_objectFlags)
        , FACADE_INIT_MEMBER(obj, m_connections)
        //, FACADE_INIT_MEMBER(obj, m_anDistances)
        , FACADE_INIT_MEMBER(obj, m_connectionFlags)
        , FACADE_INIT_MEMBER(obj, m_carPathConnections)
        , FACADE_INIT_MEMBER(obj, m_numPathNodes)
        , FACADE_INIT_MEMBER(obj, m_numCarPathNodes)
        , FACADE_INIT_MEMBER(obj, m_numPedPathNodes)
        , FACADE_INIT_MEMBER(obj, m_numMapObjects)
        , FACADE_INIT_MEMBER(obj, m_numConnections)
        , FACADE_INIT_MEMBER(obj, m_numCarPathLinks)
        //, FACADE_INIT_MEMBER(obj, field_45BEC)
        //, FACADE_INIT_MEMBER(obj, m_nNumGroups)
        //, FACADE_INIT_MEMBER(obj, m_aSearchNodes)
    {
    }

public:
    FACADE_STABLE_MEMBER(CPathNode[], m_pathNodes, 0x0);
    FACADE_MEMBER(CCarPathLink[], m_carPathLinks);
    FACADE_MEMBER(CTreadable* [], m_mapObjects);
    FACADE_MEMBER(short[], m_objectFlags);
    FACADE_MEMBER(short[], m_connections);
    //FACADE_MEMBER(short[], m_anDistances);
    FACADE_MEMBER(CConnectionFlags[], m_connectionFlags);
    FACADE_MEMBER(short[], m_carPathConnections);
    FACADE_MEMBER(int, m_numPathNodes);
    FACADE_MEMBER(int, m_numCarPathNodes);
    FACADE_MEMBER(int, m_numPedPathNodes);
    FACADE_MEMBER(short, m_numMapObjects);
    FACADE_MEMBER(short, m_numConnections);
    FACADE_MEMBER(int, m_numCarPathLinks);
    //FACADE_MEMBER(int, field_45BEC);
    //FACADE_MEMBER(unsigned char[], m_nNumGroups);
    //FACADE_MEMBER(CPathNode[], m_aSearchNodes);
};

export GameRef<CPathFind> ThePaths([]() -> CPathFind*
{
    auto pattern = hook::pattern("B9 ? ? ? ? 83 C0 ? ? ? ? ? ? ? ? ? ? 6A");
    if (!pattern.empty())
        return *pattern.get_first<CPathFind*>(1);
    return nullptr;
});

CPathFindFacade GetPaths()
{
    static std::once_flag s_once;
    std::call_once(s_once, []()
    {
        uintptr_t pathsAddress = (uintptr_t)ThePaths.get_ptr();
        //FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_pathNodes, pathsAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_carPathLinks, injector::ReadMemory<uintptr_t>(0x454FCE) - pathsAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_mapObjects, injector::ReadMemory<uintptr_t>(0x437044) - pathsAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_objectFlags, injector::ReadMemory<uintptr_t>(0x437358) - pathsAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_connections, injector::ReadMemory<uintptr_t>(0x42E2F6) - pathsAddress);
        //FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_anDistances, injector::ReadMemory<uintptr_t>(0x) - pathsAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_connectionFlags, injector::ReadMemory<uintptr_t>(0x455210) - pathsAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_carPathConnections, injector::ReadMemory<uintptr_t>(0x42E30F) - pathsAddress);

        uintptr_t numPathNodesAddress = injector::ReadMemory<uintptr_t>(0x4550B4) - pathsAddress;
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_numPathNodes, numPathNodesAddress);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_numCarPathNodes, numPathNodesAddress + 0x4);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_numPedPathNodes, numPathNodesAddress + 0x8);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_numMapObjects, numPathNodesAddress + 0xC);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_numConnections, numPathNodesAddress + 0xE);
        FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_numCarPathLinks, numPathNodesAddress + 0x10);
        //FACADE_SET_MEMBER_OFFSET(CPathFindFacade, field_45BEC, numPathNodesAddress + 0x8 + 0x4 + 0x8);
        //FACADE_SET_MEMBER_OFFSET(CPathFindFacade, m_nNumGroups, numPathNodesAddress + 0x8 + 0x4 + 0xC);
    });
    return CPathFindFacade(ThePaths.get_ptr());
}

namespace CCarCtrl
{
    export GameRef<float> CarDensityMultiplier([]() -> float*
    {
        auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? DB 05 ? ? ? ? DE D9");
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
        auto&& paths = GetPaths();
        return id < static_cast<unsigned>(paths.m_numCarPathNodes) && !paths.m_pathNodes[id].bDisabled && paths.m_pathNodes[id].GetPosition().z < 500.0f;
    }
    static bool Valid(const Edge& edge) { return NodeValid(edge.from) && NodeValid(edge.to); }
    static unsigned Degree(Node id)
    {
        auto&& paths = GetPaths();
        return NodeValid(id) ? paths.m_pathNodes[id].numLinks : 0;
    }
    static Node RandomNode(uint32_t random)
    {
        auto&& paths = GetPaths();
        int count = paths.m_numCarPathNodes;
        return count > 0 ? random % count : DistantTraffic::InvalidNode;
    }
    static size_t Outgoing(Node from, std::array<Edge, 16>& result)
    {
        if (!NodeValid(from))
            return 0;
        auto&& paths = GetPaths();
        auto& a = paths.m_pathNodes[from];
        size_t count = 0;
        for (int i = 0; i < a.numLinks && count < result.size(); ++i)
        {
            int connection = a.firstLink + i;
            if (connection < 0 || connection >= paths.m_numConnections)
                continue;
            Node to = paths.m_connections[connection];
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
            edge.water = false;
            // re3 stores the controlled direction in bit 7 of the type.
            if ((link.pathNodeIndex == to) != ((link.trafficLightType & 0x80) != 0))
                edge.signal = link.trafficLightType & 0x7F;
            edge.speed = edge.water ? 8.0f : 16.0f;
            result[count++] = edge;
        }
        return count;
    }
    static bool SpawnAllowed(const Edge& edge) { return !edge.water || bDistantMaritimeTraffic; }
    static float DensityAt(CVector position)
    {
        if (!CTheZones::GetZoneInfoForTimeOfDay)
            return 10.0f;
        CZoneInfo info{};
        CTheZones::GetZoneInfoForTimeOfDay(&position, &info);
        return static_cast<float>(info.carDensity);
    }
    static void Hide(uint32_t id)
    {
        DistantCarRenderer::HideLights(id);
        CLODLights::UnregisterCorona(id);
        CLODLights::UnregisterCorona(0x7E000000u + (id - 0x7F000000u));
        CLODLights::UnregisterCorona(0x7D000000u + (id - 0x7F000000u));
    }
};

using TrafficSimulation = DistantTraffic::Simulation<TrafficGraph, DistantCarRenderer::State, true>;
static TrafficSimulation traffic;
struct WaterTrafficGraph
{
    using Node = DistantTraffic::Node;
    using Edge = DistantTraffic::Edge;
    static bool Valid(const Edge& edge)
    {
        return bDistantMaritimeTraffic && edge.from < IIIWaterPaths::Nodes.size() && edge.to < IIIWaterPaths::Nodes.size();
    }
    static Node RandomNode(uint32_t random)
    {
        return bDistantMaritimeTraffic ? random % IIIWaterPaths::Nodes.size() : DistantTraffic::InvalidNode;
    }
    static size_t Outgoing(Node from, std::array<Edge, 16>& result)
    {
        if (!bDistantMaritimeTraffic || from >= IIIWaterPaths::Nodes.size()) return 0;
        const auto& a = IIIWaterPaths::Nodes[from];
        for (unsigned i = 0; i < a.count; ++i)
        {
            const auto& b = IIIWaterPaths::Nodes[a.links[i]];
            Edge edge;
            edge.from = from;
            edge.to = a.links[i];
            edge.position = { (a.x + b.x) * .5f, (a.y + b.y) * .5f, 0.0f };
            edge.direction = { b.x - a.x, b.y - a.y, 0.0f };
            edge.direction.Normalise();
            edge.lanes = 1;
            edge.laneWidth = 24.0f;
            edge.speed = 8.0f;
            edge.water = true;
            result[i] = edge;
        }
        return a.count;
    }
    static bool SpawnAllowed(const Edge&) { return bDistantMaritimeTraffic; }
    static void Hide(uint32_t id) { TrafficGraph::Hide(id); }
};
// Separate IDs and a small reserved share of the configured total prevent
// long-lived coastal traffic from gradually taking over the road-car pool.
static DistantTraffic::Simulation<WaterTrafficGraph, DistantCarRenderer::State> boats(0x7F010000u);
export class CMovingThings
{
  public:
    using CDistantCarImpostor = DistantTraffic::Car<DistantCarRenderer::State>;
    static std::vector<CDistantCarImpostor>& aDistantCarImpostors;
    static void InitDistantCarImpostors()
    {
        traffic.Clear();
        boats.Clear();
        traffic.Reserve(static_cast<size_t>((std::clamp)(nNumDistantCarImpostors, 0, 10000)));
        boats.Reserve(32);
    }
    static void ShutdownDistantCarImpostors() { traffic.Clear(); boats.Clear(); }
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
    size_t capacity = static_cast<size_t>((std::clamp)(nNumDistantCarImpostors, 0, 10000));
    size_t boatCapacity = bDistantMaritimeTraffic ? (std::min)(size_t(32), capacity / 20) : 0;
    float dt = CTimer::GetTimeStepInSeconds();
    traffic.SetSignals(CTrafficLights::StopForCars(1), CTrafficLights::StopForCars(2));
    traffic.Update(dt, capacity - boatCapacity, density, camera, farClip);
    boats.Update(dt, boatCapacity, density, camera, farClip);
}

template<class Simulation>
static void RenderTraffic(Simulation& simulation, DistantCarRenderer::Frame& models, const CVector& camPos, float maxDist)
{
    float nearScale = DistantTraffic::ViewDistanceScale(TheCamera->Cams[TheCamera->ActiveCam].FOV);
    for (size_t i = 0; i < simulation.cars.size(); i++)
    {
        auto& impostor = simulation.cars[i];
        if (!impostor.m_bActive)
            continue;

        if (impostor.m_bWaterNode && !bDistantMaritimeTraffic)
            continue;

        CVector toImpostor = simulation.RenderPosition(impostor) - camPos;
        float distSqr = toImpostor.MagnitudeSqr2D();
        if (distSqr < SQR(140.0f) || distSqr > SQR(maxDist))
            continue;

        float dist = Sqrt(distSqr);
        if (dist < 0.001f)
            continue;

        float dirDot = DotProduct(simulation.RenderDirection(impostor), camPos - simulation.RenderPosition(impostor));
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
        float fadeNear = Clamp((dist * nearScale - 140.0f) / 120.0f, 0.0f, 1.0f);
        float fade = fadeFar * fadeNear;

        if (models.Add(simulation.RenderPosition(impostor), simulation.RenderDirection(impostor), impostor.m_visual, impostor.m_nCoronaId, fade, impostor.m_bWaterNode))
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
            float toCamX = camPos.x - simulation.RenderPosition(impostor).x;
            float toCamY = camPos.y - simulation.RenderPosition(impostor).y;
            float toCamLen = Sqrt(toCamX * toCamX + toCamY * toCamY);
            if (toCamLen > 0.001f)
            {
                float fwd = (simulation.RenderDirection(impostor).x * toCamX + simulation.RenderDirection(impostor).y * toCamY) / toCamLen;
                boatCross = simulation.RenderDirection(impostor).x * toCamY - simulation.RenderDirection(impostor).y * toCamX;

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
                portPos = simulation.RenderPosition(impostor) +
                          CVector(-simulation.RenderDirection(impostor).y * beamHalf, simulation.RenderDirection(impostor).x * beamHalf, 0.0f);
                stbdPos = simulation.RenderPosition(impostor) +
                          CVector(simulation.RenderDirection(impostor).y * beamHalf, -simulation.RenderDirection(impostor).x * beamHalf, 0.0f);
            }
        }
        if (alpha == 0 && sideAlpha == 0)
            continue;

        if (alpha)
        {
            CLODLights::RegisterCorona(impostor.m_nCoronaId, nullptr, red, green, blue, alpha, simulation.RenderPosition(impostor), size, maxDist, 1, 0, false,
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
}

void CMovingThings::RenderDistantCarImpostors()
{
    if (nNumDistantCarImpostors <= 0) return;
    CVector camPos = TheCamera->GetPosition();
    float maxDist = CTimeCycle::m_fCurrentFarClip;
    DistantCarRenderer::Frame models(camPos, maxDist);
    RenderTraffic(traffic, models, camPos, maxDist);
    RenderTraffic(boats, models, camPos, maxDist);
    models.Flush();
}
