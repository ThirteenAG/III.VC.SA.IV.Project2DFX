module;

#include <stdafx.h>

export module DistantCars;

import ComVars;
import Misc;
import Timer;
import Camera;
import Clock;
import Timecycle;
import LODLights;

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

export GameRef<CPathFind> ThePaths([]() -> CPathFind*
{
    auto pattern = hook::pattern("B9 ? ? ? ? 83 C0 ? ? ? ? ? ? ? ? ? ? 6A");
    if (!pattern.empty())
        return *pattern.get_first<CPathFind*>(1);
    return nullptr;
});

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

export class CMovingThings
{
public:
    struct CDistantCarImpostor
    {
        bool m_bActive;
        int16_t m_nPrevNode;
        int16_t m_nNextNode;
        float m_fProgress;
        float m_fSpeed;
        float m_fDesiredSpeed;
        float m_fLaneOffset;
        uint8_t m_nLaneSide;
        uint8_t m_nLaneCount;
        uint8_t m_nLaneIndex;
        CVector m_vecPos;
        CVector m_vecDir;
        uint16_t m_nStuckFrames;
        uint32_t m_nCoronaId;
    };

    static std::vector<CDistantCarImpostor> aDistantCarImpostors;

    static void InitDistantCarImpostors();
    static void ShutdownDistantCarImpostors();
    static void UpdateDistantCarImpostors();
    static void RenderDistantCarImpostors();

private:
    static bool InitDistantCarImpostor(CDistantCarImpostor& impostor, uint32_t coronaId);
    static bool PickNextNodeForImpostor(const CDistantCarImpostor& impostor, int16_t& nextNode, CCarPathLink& laneLink);
    static bool FindLaneLinkForSegment(int16_t fromNode, int16_t toNode, CCarPathLink& laneLink);
    static void EnsureDistantCarImpostorPoolSize();
};

std::vector<CMovingThings::CDistantCarImpostor> CMovingThings::aDistantCarImpostors;

static uint32 ImpostorCoronaId(int32 i)
{
    return 0x7F000000 + i;
}

static void HideImpostorCorona(CMovingThings::CDistantCarImpostor& impostor)
{
    CLODLights::RegisterCorona(impostor.m_nCoronaId, nullptr,
        0, 0, 0, 0,
        CVector(0.0f, 0.0f, 0.0f),
        0.0f,
        0.0f,
        1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
}

static bool IsPathSegmentExcludedForImpostor(int16 fromNode, int16 toNode)
{
    //static const int16 excludedSegments[][2] = {
    //    // Optional manual exclusions:
    //    // { fromNode, toNode },
    //    // { 123, 456 },
    //    { -1, -1 },
    //};
    //
    //for (int32 i = 0; i < (sizeof(excludedSegments) / sizeof(excludedSegments[0])); i++)
    //{
    //    if ((excludedSegments[i][0] == fromNode && excludedSegments[i][1] == toNode) ||
    //        (excludedSegments[i][0] == toNode && excludedSegments[i][1] == fromNode))
    //        return true;
    //}

    CPathNode& fromPathNode = ThePaths->m_pathNodes[fromNode];
    CPathNode& toPathNode = ThePaths->m_pathNodes[toNode];

    if (fromPathNode.bDisabled || toPathNode.bDisabled)
        return true;

    CVector fromPos = fromPathNode.GetPosition();
    CVector toPos = toPathNode.GetPosition();
    CVector midPos = (fromPos + toPos) * 0.5f;

    CZoneInfo zoneFrom;
    CZoneInfo zoneMid;
    CZoneInfo zoneTo;
    CTheZones::GetZoneInfoForTimeOfDay(&fromPos, &zoneFrom);
    CTheZones::GetZoneInfoForTimeOfDay(&midPos, &zoneMid);
    CTheZones::GetZoneInfoForTimeOfDay(&toPos, &zoneTo);

    if (zoneFrom.carDensity <= 0 || zoneMid.carDensity <= 0 || zoneTo.carDensity <= 0)
        return true;

    return false;
}

static float ComputeDynamicImpostorDensityScale(float camTravelSpeed)
{
    float density = Clamp(CCarCtrl::CarDensityMultiplier, 0.25f, 1.4f);

    int32 hour = CClock::ms_nGameClockHours;
    if (hour <= 5)
        density *= 0.65f;
    else if (hour >= 7 && hour <= 9)
        density *= 1.12f;
    else if (hour >= 16 && hour <= 18)
        density *= 1.10f;

    density *= (1.0f - 0.22f * CWeather::Rain);
    density *= (1.0f - 0.16f * CWeather::Foggyness);
    density *= Clamp(0.9f + camTravelSpeed * 0.02f, 0.9f, 1.2f);

    return Clamp(density, 0.2f, 1.25f);
}

static float ComputeLaneOffset(bool useRightSide, int32 laneCount, int32 laneIndex, CCarPathLink link)
{
    laneCount = Max(1, laneCount);
    laneIndex = Clamp(laneIndex, 0, laneCount - 1);

    const float laneWidth = 5.0f;
    float sideSign = useRightSide ? -1.0f : 1.0f;
    return sideSign * (laneIndex + link.OneWayLaneOffset()) * laneWidth;
}

static bool IsTraversalAlongLinkDir(int16 fromNode, int16 toNode, CCarPathLink& link)
{
    CVector fromPos = ThePaths->m_pathNodes[fromNode].GetPosition();
    CVector toPos = ThePaths->m_pathNodes[toNode].GetPosition();
    CVector2D segDir = CVector2D(toPos.x - fromPos.x, toPos.y - fromPos.y);
    CVector2D linkDir = link.GetDirection();
    return DotProduct2D(segDir, linkDir) >= 0.0f;
}

static bool CanTraverseSegmentDirection(int16 fromNode, int16 toNode, CCarPathLink& link)
{
    bool alongLinkDir = IsTraversalAlongLinkDir(fromNode, toNode, link);
    return alongLinkDir ? (link.numRightLanes > 0) : (link.numLeftLanes > 0);
}

static bool UseRightLaneGroupForTraversal(int16 fromNode, int16 toNode, CCarPathLink& link)
{
    return IsTraversalAlongLinkDir(fromNode, toNode, link);
}

static bool ShouldKeepImpostorAliveNearCamera(const CMovingThings::CDistantCarImpostor& impostor, const CVector& camPos)
{
    return (impostor.m_vecPos - camPos).MagnitudeSqr2D() < SQR(380.0f);
}

static bool IsImpostorLaneEntryFree(int16 prevNode, int16 nextNode, uint8 laneSide, uint8 laneIndex, float minEntryGap, const CMovingThings::CDistantCarImpostor* self)
{
    for (size_t k = 0; k < CMovingThings::aDistantCarImpostors.size(); k++)
    {
        const CMovingThings::CDistantCarImpostor& other = CMovingThings::aDistantCarImpostors[k];
        if (!other.m_bActive || &other == self)
            continue;
        if (other.m_nPrevNode != prevNode || other.m_nNextNode != nextNode)
            continue;
        if (other.m_nLaneSide != laneSide || other.m_nLaneIndex != laneIndex)
            continue;
        if (other.m_fProgress < minEntryGap)
            return false;
    }
    return true;
}

static bool ComputeImpostorTransform(CMovingThings::CDistantCarImpostor& impostor)
{
    if (impostor.m_nPrevNode < 0 || impostor.m_nNextNode < 0 ||
        impostor.m_nPrevNode >= ThePaths->m_numCarPathNodes || impostor.m_nNextNode >= ThePaths->m_numCarPathNodes)
        return false;

    CVector fromPos = ThePaths->m_pathNodes[impostor.m_nPrevNode].GetPosition();
    CVector toPos = ThePaths->m_pathNodes[impostor.m_nNextNode].GetPosition();
    CVector segment = toPos - fromPos;
    float segmentLen = segment.Magnitude2D();
    if (segmentLen < 0.001f)
        return false;

    CVector dir = segment / segmentLen;
    CVector travelDir = dir;
    CVector right(-dir.y, dir.x, 0.0f);

    {
        CPathNode& node = ThePaths->m_pathNodes[impostor.m_nPrevNode];
        for (int32 li = 0; li < (int32)node.numLinks; li++)
        {
            int32 conn = node.firstLink + li;
            if (ThePaths->ConnectedNode(conn) != impostor.m_nNextNode) continue;
            CCarPathLink link = ThePaths->m_carPathLinks[ThePaths->m_carPathConnections[conn]];
            impostor.m_fLaneOffset = ComputeLaneOffset(true, impostor.m_nLaneCount, impostor.m_nLaneIndex, link);
            break;
        }
    }

    CVector pos = fromPos + segment * impostor.m_fProgress + right * impostor.m_fLaneOffset;
    pos.z += 0.55f;

    impostor.m_vecPos = pos;
    impostor.m_vecDir = travelDir;
    return true;
}

void CMovingThings::EnsureDistantCarImpostorPoolSize()
{
    int32 desired = nNumDistantCarImpostors;
    desired = Clamp(desired, 64, 10000);

    size_t oldSize = aDistantCarImpostors.size();
    size_t newSize = static_cast<size_t>(desired);

    if (newSize == oldSize)
        return;

    if (newSize < oldSize)
    {
        for (size_t i = newSize; i < oldSize; ++i)
        {
            auto& impostor = aDistantCarImpostors[i];
            if (impostor.m_bActive)
            {
                impostor.m_bActive = false;
                HideImpostorCorona(impostor);
            }
        }
    }

    aDistantCarImpostors.resize(newSize);

    if (newSize > oldSize)
    {
        for (size_t i = oldSize; i < newSize; ++i)
            InitDistantCarImpostor(aDistantCarImpostors[i], ImpostorCoronaId(static_cast<int32>(i)));
    }
}

bool CMovingThings::FindLaneLinkForSegment(int16 fromNode, int16 toNode, CCarPathLink& laneLink)
{
    if (fromNode < 0 || toNode < 0 || fromNode >= ThePaths->m_numCarPathNodes || toNode >= ThePaths->m_numCarPathNodes)
        return false;

    CPathNode& node = ThePaths->m_pathNodes[fromNode];
    for (int32 i = 0; i < node.numLinks; i++)
    {
        int32 connection = node.firstLink + i;
        if (ThePaths->ConnectedNode(connection) != toNode)
            continue;

        laneLink = ThePaths->m_carPathLinks[ThePaths->m_carPathConnections[connection]];
        return true;
    }
    return false;
}

bool CMovingThings::PickNextNodeForImpostor(const CDistantCarImpostor& impostor, int16& nextNode, CCarPathLink& laneLink)
{
    if (impostor.m_nNextNode < 0 || impostor.m_nNextNode >= ThePaths->m_numCarPathNodes)
        return false;

    CPathNode& node = ThePaths->m_pathNodes[impostor.m_nNextNode];
    if (node.numLinks == 0)
        return false;

    int16 nodeChoices[16];
    CCarPathLink linkChoices[16];
    int32 numChoices = 0;

    for (int32 i = 0; i < node.numLinks && numChoices < (sizeof(nodeChoices) / sizeof(nodeChoices[0])); i++)
    {
        int16 candidate = ThePaths->ConnectedNode(node.firstLink + i);
        if (candidate == impostor.m_nPrevNode && node.numLinks > 1)
            continue;

        CCarPathLink candidateLink;
        if (!FindLaneLinkForSegment(impostor.m_nNextNode, candidate, candidateLink))
            continue;
        if (!CanTraverseSegmentDirection(impostor.m_nNextNode, candidate, candidateLink))
            continue;
        if (IsPathSegmentExcludedForImpostor(impostor.m_nNextNode, candidate))
            continue;

        nodeChoices[numChoices] = candidate;
        linkChoices[numChoices] = candidateLink;
        numChoices++;
    }

    if (numChoices == 0)
    {
        if (impostor.m_nPrevNode < 0)
            return false;
        if (!FindLaneLinkForSegment(impostor.m_nNextNode, impostor.m_nPrevNode, laneLink))
            return false;
        if (!CanTraverseSegmentDirection(impostor.m_nNextNode, impostor.m_nPrevNode, laneLink))
            return false;
        if (IsPathSegmentExcludedForImpostor(impostor.m_nNextNode, impostor.m_nPrevNode))
            return false;

        nextNode = impostor.m_nPrevNode;
        return true;
    }

    int32 pick = CGeneral::GetRandomNumber() % numChoices;
    nextNode = nodeChoices[pick];
    laneLink = linkChoices[pick];
    return true;
}

bool CMovingThings::InitDistantCarImpostor(CDistantCarImpostor& impostor, uint32 coronaId)
{
    impostor.m_bActive = false;
    impostor.m_nCoronaId = coronaId;

    if (ThePaths->m_numCarPathNodes <= 0)
        return false;

    for (int32 attempts = 0; attempts < 128; attempts++)
    {
        int16 fromNode = CGeneral::GetRandomNumber() % ThePaths->m_numCarPathNodes;
        CPathNode& node = ThePaths->m_pathNodes[fromNode];
        if (node.numLinks == 0)
            continue;

        int16 toNode = ThePaths->ConnectedNode(node.firstLink + CGeneral::GetRandomNumber() % node.numLinks);
        CCarPathLink laneLink;
        if (!FindLaneLinkForSegment(fromNode, toNode, laneLink))
            continue;
        if (!CanTraverseSegmentDirection(fromNode, toNode, laneLink))
            continue;
        if (IsPathSegmentExcludedForImpostor(fromNode, toNode))
            continue;

        int8 leftLanes = Max((int8)1, laneLink.numLeftLanes);
        int8 rightLanes = Max((int8)1, laneLink.numRightLanes);
        bool useRightLaneGroup = UseRightLaneGroupForTraversal(fromNode, toNode, laneLink);

        int8 laneCount = useRightLaneGroup ? rightLanes : leftLanes;
        int8 lane = CGeneral::GetRandomNumber() % laneCount;
        uint8 laneSide = useRightLaneGroup ? 1 : 0;

        // Compute a safe spawn progress that doesn't overlap any existing impostor on this lane.
        // Collect progress values already occupied on this segment+lane, then find the largest gap.
        static std::vector<float> occupied;
        static std::vector<float> gapStart;
        static std::vector<float> gapEnd;

        occupied.clear();
        occupied.reserve(aDistantCarImpostors.size());

        CVector fromNodePos = node.GetPosition();
        CVector toNodePos = ThePaths->m_pathNodes[toNode].GetPosition();
        float segLen = (toNodePos - fromNodePos).Magnitude2D();
        float minGap = (segLen > 0.001f) ? Min(0.35f, 16.0f / segLen) : 0.35f;

        for (size_t k = 0; k < aDistantCarImpostors.size(); k++)
        {
            const CDistantCarImpostor& other = aDistantCarImpostors[k];
            if (!other.m_bActive)
                continue;
            if (other.m_nPrevNode != fromNode || other.m_nNextNode != toNode)
                continue;
            if (other.m_nLaneSide != laneSide || other.m_nLaneIndex != lane)
                continue;
            occupied.push_back(other.m_fProgress);
        }

        int32 numOccupied = static_cast<int32>(occupied.size());

        // Choose a random progress in [0,1] that is at least minGap from every occupied slot.
        // Sort occupied list, then check each inter-gap for a slot wide enough.
        // Simple insertion sort on the small occupied array.
        for (int32 a = 1; a < numOccupied; a++)
        {
            float v = occupied[a];
            int32 b = a - 1;
            while (b >= 0 && occupied[b] > v) { occupied[b + 1] = occupied[b]; b--; }
            occupied[b + 1] = v;
        }

        // Build candidate mid-points of each free gap, starting with a random offset.
        float spawnProgress = -1.0f;
        if (numOccupied == 0)
        {
            spawnProgress = (CGeneral::GetRandomNumber() & 0xFF) / 255.0f;
        }
        else
        {
            // Check gap before first, between each pair, and after last (wrapping is not needed
            // because progress is clamped to [0,1] and nodes are recycled at boundary anyway).
            int32 numGaps = numOccupied + 1;
            gapStart.resize(numGaps);
            gapEnd.resize(numGaps);
            int32 numFree = 0;
            float starts[2] = { 0.0f };
            float ends[2] = { 0.0f };
            // gap before first
            starts[0] = 0.0f;  ends[0] = occupied[0] - minGap;
            if (ends[0] > starts[0] + minGap) { gapStart[numFree] = starts[0]; gapEnd[numFree] = ends[0]; numFree++; }
            // gaps between pairs
            for (int32 g = 0; g < numOccupied - 1; g++)
            {
                float gs = occupied[g] + minGap;
                float ge = occupied[g + 1] - minGap;
                if (ge > gs) { gapStart[numFree] = gs; gapEnd[numFree] = ge; numFree++; }
            }
            // gap after last
            {
                float gs = occupied[numOccupied - 1] + minGap;
                float ge = 1.0f;
                if (ge > gs + minGap) { gapStart[numFree] = gs; gapEnd[numFree] = ge; numFree++; }
            }
            if (numFree > 0)
            {
                int32 pick2 = CGeneral::GetRandomNumber() % numFree;
                float t = (CGeneral::GetRandomNumber() & 0xFF) / 255.0f;
                spawnProgress = gapStart[pick2] + t * (gapEnd[pick2] - gapStart[pick2]);
            }
        }

        if (spawnProgress < 0.0f)
            continue; // no free slot on this segment+lane, try another

        impostor.m_bActive = true;
        impostor.m_nPrevNode = fromNode;
        impostor.m_nNextNode = toNode;
        impostor.m_fProgress = Clamp(spawnProgress, 0.0f, 1.0f);
        impostor.m_fDesiredSpeed = 9.0f + (CGeneral::GetRandomNumber() % 18);
        impostor.m_fSpeed = impostor.m_fDesiredSpeed;
        impostor.m_nLaneSide = laneSide;
        impostor.m_nLaneCount = laneCount;
        impostor.m_nLaneIndex = lane;
        impostor.m_fLaneOffset = 0.0f; // recomputed each frame in ComputeImpostorTransform
        impostor.m_vecPos = node.GetPosition();
        impostor.m_vecDir = CVector(1.0f, 0.0f, 0.0f);
        impostor.m_nStuckFrames = 0;
        return true;
    }

    return false;
}

void CMovingThings::InitDistantCarImpostors()
{
    EnsureDistantCarImpostorPoolSize();
    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
        InitDistantCarImpostor(aDistantCarImpostors[i], ImpostorCoronaId(static_cast<int32>(i)));
}

void CMovingThings::ShutdownDistantCarImpostors()
{
    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
    {
        CDistantCarImpostor& impostor = aDistantCarImpostors[i];
        impostor.m_bActive = false;
        HideImpostorCorona(impostor);
    }
}

void CMovingThings::UpdateDistantCarImpostors()
{
    if (ThePaths->m_numCarPathNodes <= 0)
        return;

    float dt = CTimer::GetTimeStepInSeconds();
    CVector camPos = TheCamera->GetPosition();

    static bool sHasPrevCamPos = false;
    static CVector sPrevCamPos;
    float camTravelSpeed = 0.0f;
    if (sHasPrevCamPos && dt > 0.0001f)
        camTravelSpeed = (camPos - sPrevCamPos).Magnitude2D() / dt;
    sPrevCamPos = camPos;
    sHasPrevCamPos = true;

    float densityScale = ComputeDynamicImpostorDensityScale(camTravelSpeed);
    int32 impostorCount = static_cast<int32>(aDistantCarImpostors.size());
    int32 desiredActive = (int32)Clamp((impostorCount * densityScale), 64, impostorCount);

    int32 activeCount = 0;
    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
    {
        if (aDistantCarImpostors[i].m_bActive)
            activeCount++;
    }

    int32 toDisable = Max(0, activeCount - desiredActive);
    if (toDisable > 0 && camTravelSpeed < 8.0f)
    {
        int32 disableBudget = Min(8, toDisable);
        for (size_t i = 0; i < aDistantCarImpostors.size() && disableBudget > 0; i++)
        {
            CDistantCarImpostor& impostor = aDistantCarImpostors[i];
            if (!impostor.m_bActive)
                continue;
            if (ShouldKeepImpostorAliveNearCamera(impostor, camPos))
                continue;
            if ((impostor.m_vecPos - camPos).MagnitudeSqr2D() < SQR(700.0f))
                continue;

            impostor.m_bActive = false;
            HideImpostorCorona(impostor);
            disableBudget--;
            activeCount--;
        }
    }

    int32 spawnBudget = Min(12, Max(0, desiredActive - activeCount));

    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
    {
        CDistantCarImpostor& impostor = aDistantCarImpostors[i];
        if (!impostor.m_bActive)
        {
            if (spawnBudget <= 0)
                continue;
            if ((CGeneral::GetRandomNumber() & 3) != 0)
                continue;

            if (InitDistantCarImpostor(impostor, ImpostorCoronaId(i)))
            {
                activeCount++;
                spawnBudget--;
            }
            continue;
        }

        if (impostor.m_nPrevNode < 0 || impostor.m_nNextNode < 0 ||
            impostor.m_nPrevNode >= ThePaths->m_numCarPathNodes || impostor.m_nNextNode >= ThePaths->m_numCarPathNodes)
        {
            if (ShouldKeepImpostorAliveNearCamera(impostor, camPos) && impostor.m_nStuckFrames < 120)
            {
                impostor.m_nStuckFrames++;
                continue;
            }
            InitDistantCarImpostor(impostor, impostor.m_nCoronaId);
            continue;
        }

        if (IsPathSegmentExcludedForImpostor(impostor.m_nPrevNode, impostor.m_nNextNode))
        {
            if (!InitDistantCarImpostor(impostor, impostor.m_nCoronaId))
            {
                impostor.m_bActive = false;
                HideImpostorCorona(impostor);
            }
            continue;
        }

        CVector fromPos = ThePaths->m_pathNodes[impostor.m_nPrevNode].GetPosition();
        CVector toPos = ThePaths->m_pathNodes[impostor.m_nNextNode].GetPosition();
        CVector segment = toPos - fromPos;
        float segmentLen = segment.Magnitude2D();
        if (segmentLen < 0.5f)
        {
            if (ShouldKeepImpostorAliveNearCamera(impostor, camPos) && impostor.m_nStuckFrames < 120)
            {
                impostor.m_nStuckFrames++;
                continue;
            }
            InitDistantCarImpostor(impostor, impostor.m_nCoronaId);
            continue;
        }

        float speedRecover = 3.0f * dt;
        impostor.m_fSpeed += Clamp(impostor.m_fDesiredSpeed - impostor.m_fSpeed, -6.0f * dt, speedRecover);
        impostor.m_fSpeed = Max(1.5f, impostor.m_fSpeed);

        float advance = impostor.m_fSpeed * dt / segmentLen;
        impostor.m_fProgress += advance;

        while (impostor.m_fProgress >= 1.0f)
        {
            impostor.m_fProgress -= 1.0f;

            CCarPathLink nextLink;
            int16 nextNode;
            if (!PickNextNodeForImpostor(impostor, nextNode, nextLink))
            {
                if (ShouldKeepImpostorAliveNearCamera(impostor, camPos) && impostor.m_nStuckFrames < 120)
                {
                    impostor.m_fProgress = 0.995f;
                    impostor.m_fSpeed = 0.0f;
                    impostor.m_nStuckFrames++;
                    break;
                }
                InitDistantCarImpostor(impostor, impostor.m_nCoronaId);
                break;
            }

            int16 targetPrevNode = impostor.m_nNextNode;
            int16 targetNextNode = nextNode;

            int8 leftLanes = Max((int8)1, nextLink.numLeftLanes);
            int8 rightLanes = Max((int8)1, nextLink.numRightLanes);
            bool useRightLaneGroup = UseRightLaneGroupForTraversal(targetPrevNode, targetNextNode, nextLink);

            int8 laneCount = useRightLaneGroup ? rightLanes : leftLanes;
            laneCount = Max((int8)1, laneCount);
            uint8 laneSide = useRightLaneGroup ? 1 : 0;
            uint8 laneIndex = Min((uint8)(laneCount - 1), impostor.m_nLaneIndex);

            CVector targetFromPos = ThePaths->m_pathNodes[targetPrevNode].GetPosition();
            CVector targetToPos = ThePaths->m_pathNodes[targetNextNode].GetPosition();
            float targetSegLen = (targetToPos - targetFromPos).Magnitude2D();
            float minEntryGap = (targetSegLen > 0.001f) ? Min(0.35f, 16.0f / targetSegLen) : 0.35f;

            bool canEnter = true;
            for (size_t k = 0; k < aDistantCarImpostors.size(); k++)
            {
                const CDistantCarImpostor& other = aDistantCarImpostors[k];
                if (!other.m_bActive || &other == &impostor)
                    continue;
                if (other.m_nPrevNode != targetPrevNode || other.m_nNextNode != targetNextNode)
                    continue;
                if (other.m_nLaneSide != laneSide || other.m_nLaneIndex != laneIndex)
                    continue;
                if (other.m_fProgress < minEntryGap)
                {
                    canEnter = false;
                    break;
                }
            }

            if (!canEnter)
            {
                impostor.m_fProgress = 0.999f;
                impostor.m_fSpeed = 0.0f;
                if (impostor.m_nStuckFrames < 200)
                    impostor.m_nStuckFrames++;

                // If this impostor has been blocked for a long time away from camera, recycle it
                // to avoid persistent artificial gridlock at awkward turns.
                if (impostor.m_nStuckFrames > 120 && !ShouldKeepImpostorAliveNearCamera(impostor, camPos))
                {
                    if (!InitDistantCarImpostor(impostor, impostor.m_nCoronaId))
                    {
                        impostor.m_bActive = false;
                        HideImpostorCorona(impostor);
                    }
                }
                break;
            }

            impostor.m_nPrevNode = targetPrevNode;
            impostor.m_nNextNode = targetNextNode;
            impostor.m_nLaneSide = laneSide;
            impostor.m_nLaneCount = laneCount;
            impostor.m_nLaneIndex = laneIndex;
            // m_fLaneOffset recomputed each frame in ComputeImpostorTransform
            impostor.m_nStuckFrames = 0;
        }

        if (!impostor.m_bActive)
            continue;

        if (!ComputeImpostorTransform(impostor))
            continue;
    }

    // Sort all active impostors by (prevNode, nextNode, laneSide, laneIndex) first, then by
    // descending progress within each group.  A single linear sweep of adjacent pairs is then
    // sufficient to guarantee zero collisions: every push cascades to the next car in the array
    // because it is always the immediate neighbour in the same lane segment.
    const float desiredHeadway = 18.0f;
    static std::vector<int32> sortedIdx;
    sortedIdx.clear();
    sortedIdx.reserve(aDistantCarImpostors.size());
    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
    {
        if (aDistantCarImpostors[i].m_bActive)
            sortedIdx.push_back(static_cast<int32>(i));
    }
    int32 sortedCount = static_cast<int32>(sortedIdx.size());

    // Insertion-sort: primary key = (prevNode, nextNode, laneSide, laneIndex) ascending,
    //                 secondary key = progress descending (front of lane comes first).
    for (int32 i = 1; i < sortedCount; i++)
    {
        int32 key = sortedIdx[i];
        const CDistantCarImpostor& keyImp = aDistantCarImpostors[key];
        int32 j = i - 1;
        while (j >= 0)
        {
            const CDistantCarImpostor& jImp = aDistantCarImpostors[sortedIdx[j]];
            bool keyBeforeJ;
            if (keyImp.m_nPrevNode != jImp.m_nPrevNode)
                keyBeforeJ = keyImp.m_nPrevNode < jImp.m_nPrevNode;
            else if (keyImp.m_nNextNode != jImp.m_nNextNode)
                keyBeforeJ = keyImp.m_nNextNode < jImp.m_nNextNode;
            else if (keyImp.m_nLaneSide != jImp.m_nLaneSide)
                keyBeforeJ = keyImp.m_nLaneSide < jImp.m_nLaneSide;
            else if (keyImp.m_nLaneIndex != jImp.m_nLaneIndex)
                keyBeforeJ = keyImp.m_nLaneIndex < jImp.m_nLaneIndex;
            else
                keyBeforeJ = keyImp.m_fProgress > jImp.m_fProgress; // higher progress = further ahead = comes first
            if (!keyBeforeJ)
                break;
            sortedIdx[j + 1] = sortedIdx[j];
            j--;
        }
        sortedIdx[j + 1] = key;
    }

    // Multi-pass linear sweep so corrections cascade fully through each lane stream.
    // Also cap follower speed from actual available gap so faster cars never tunnel into slower ones.
    for (int32 pass = 0; pass < 3; pass++)
    {
        for (int32 si = 1; si < sortedCount; si++)
        {
            CDistantCarImpostor& curr = aDistantCarImpostors[sortedIdx[si]];
            CDistantCarImpostor& prev = aDistantCarImpostors[sortedIdx[si - 1]];

            if (prev.m_nPrevNode != curr.m_nPrevNode || prev.m_nNextNode != curr.m_nNextNode)
                continue;
            if (prev.m_nLaneSide != curr.m_nLaneSide || prev.m_nLaneIndex != curr.m_nLaneIndex)
                continue;

            CVector fromPos = ThePaths->m_pathNodes[curr.m_nPrevNode].GetPosition();
            CVector toPos = ThePaths->m_pathNodes[curr.m_nNextNode].GetPosition();
            float segLen = (toPos - fromPos).Magnitude2D();
            if (segLen < 0.001f)
                continue;

            float minGap = Min(0.35f, desiredHeadway / segLen);
            float gap = prev.m_fProgress - curr.m_fProgress;
            float gapMeters = gap * segLen;

            if (gapMeters < desiredHeadway * 2.0f)
            {
                float followFactor = Clamp((gapMeters - 2.0f) / (desiredHeadway * 2.0f), 0.0f, 1.0f);
                float followSpeed = prev.m_fSpeed * (0.2f + 0.8f * followFactor);
                curr.m_fSpeed = Min(curr.m_fSpeed, followSpeed);
            }

            float maxGapProgressThisFrame = Max(0.0f, gap - minGap);
            float maxSafeSpeed = (dt > 0.0001f) ? (maxGapProgressThisFrame * segLen / dt) : 0.0f;
            curr.m_fSpeed = Min(curr.m_fSpeed, Max(0.0f, maxSafeSpeed));

            if (gap < minGap)
            {
                curr.m_fProgress = Max(0.0f, prev.m_fProgress - minGap);
                curr.m_fSpeed = Min(curr.m_fSpeed, prev.m_fSpeed);
                ComputeImpostorTransform(curr);
            }
        }
    }
}

void CMovingThings::RenderDistantCarImpostors()
{
    CVector camPos = TheCamera->GetPosition();

    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
    {
        CDistantCarImpostor& impostor = aDistantCarImpostors[i];
        if (!impostor.m_bActive)
            continue;

        auto maxDist = CTimeCycle::m_fCurrentFarClip;
        CVector toImpostor = impostor.m_vecPos - camPos;
        float distSqr = toImpostor.MagnitudeSqr2D();
        if (distSqr < SQR(140.0f) || distSqr > SQR(maxDist))
            continue;

        float dist = Sqrt(distSqr);
        if (dist < 0.001f)
            continue;

        float dirDot = DotProduct(impostor.m_vecDir, camPos - impostor.m_vecPos);
        bool approaching = dirDot > 0.0f;

        uint8 red = approaching ? 255 : 255;
        uint8 green = approaching ? 255 : 40;
        uint8 blue = approaching ? 230 : 40;
        uint8 alpha = 150;

        float fadeFar = Clamp((maxDist - dist) / 250.0f, 0.0f, 1.0f);
        float fadeNear = Clamp((dist - 140.0f) / 120.0f, 0.0f, 1.0f);
        float fade = fadeFar * fadeNear;
        alpha = (uint8)(alpha * fade);
        if (alpha == 0)
            continue;

        CLODLights::RegisterCorona(impostor.m_nCoronaId, nullptr,
            red, green, blue, alpha,
            impostor.m_vecPos,
            4.0f * fDistantCarsRadiusMultiplier,
            maxDist,
            1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
    }
}