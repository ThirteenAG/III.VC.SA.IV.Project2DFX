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
using uint64 = uint64_t;

#define Max(a, b)           ((a) > (b) ? (a) : (b))
#define Min(a, b)           ((a) < (b) ? (a) : (b))
#define Clamp(v, low, high) ((v) < (low) ? (low) : (v) > (high) ? (high) : (v))
constexpr auto SQR = [](auto x) { return x * x; };

class CompressedVector
{
public:
    short x, y, z;
};

class CompressedVector2D
{
public:
    short x, y;
};

class CPathNode
{
public:
    CPathNode* m_next, * m_prev;
    CompressedVector m_vPos;
    int16            m_totalDistFromOrigin;
    int16            m_wBaseLinkId;
    uint16           m_wAreaId;
    uint16           m_wNodeId;
    uint8            m_nPathWidth;
    uint8            m_nFloodFill;

    uint32 m_nNumLinks : 4;
    uint32 m_onDeadEnd : 1;
    uint32 bDisabled : 1;
    uint32 m_bRoadBlocks : 1;
    uint32 m_bWaterNode : 1;

    uint32 m_isSwitchedOffOriginal : 1;
    uint32 unk1 : 1;
    uint32 m_bDontWander : 1;
    uint32 unk2 : 1;
    uint32 m_bNotHighway : 1;
    uint32 m_bHighway : 1;
    uint32 unk3 : 1;
    uint32 unk4 : 1;

    uint32 m_nSpawnProbability : 4;
    uint32 m_nBehaviourType : 4;

    CVector GetPosition() const { return CVector(m_vPos.x / 8.0f, m_vPos.y / 8.0f, m_vPos.z / 8.0f); }
};

class CNodeAddress
{
public:
    short m_nAreaId;
    short m_nNodeId;

    inline CNodeAddress() { Clear(); }
    inline CNodeAddress(short areaId, short nodeId) { Set(areaId, nodeId); }
    inline void Set(short areaId, short nodeId) { m_nAreaId = areaId; m_nNodeId = nodeId; }
    inline bool IsEmpty() const { return m_nAreaId == -1 || m_nNodeId == -1; }
    inline void Clear() { m_nAreaId = -1; m_nNodeId = -1; }
    inline bool operator==(CNodeAddress const& rhs) const { return m_nAreaId == rhs.m_nAreaId && m_nNodeId == rhs.m_nNodeId; }
    inline bool operator!=(CNodeAddress const& rhs) const { return m_nAreaId != rhs.m_nAreaId || m_nNodeId != rhs.m_nNodeId; }
};

class CCarPathLink
{
public:
    CompressedVector2D m_vecPosn;
    CNodeAddress       m_address;
    char               m_nDirX;
    char               m_nDirY;
    char               m_nPathNodeWidth;

    unsigned char numLeftLanes : 3;
    unsigned char numRightLanes : 3;
    unsigned char m_bTrafficLightDirection : 1;
    unsigned char unk1 : 1;

    uint16 m_nTrafficLightState : 2; // must be uint16 — struct must be 14 bytes
    uint16 m_bTrainCrossing : 1;

    float GetNodePathWidth() const { return (float)m_nPathNodeWidth; }

    CVector2D GetPosition() { return CVector2D(m_vecPosn.x / 8.0f, m_vecPosn.y / 8.0f); }
    CVector2D GetDirection() { return CVector2D(m_nDirX / 100.0f, m_nDirY / 100.0f); }

    float OneWayLaneOffset() const
    {
        if (numLeftLanes)
            return 0.5f - (float)numRightLanes / 2.0f;
        if (numRightLanes)
            return 0.5f - GetNodePathWidth() / 5.4f / 2.0f;
        return 0.5f - (float)numLeftLanes / 2.0f;
    }
};

constexpr auto NUM_PATH_MAP_AREAS = 64;
constexpr auto NUM_PATH_INTERIOR_AREAS = 8;
constexpr auto NUM_PATH_TOTAL_AREAS = NUM_PATH_MAP_AREAS + NUM_PATH_INTERIOR_AREAS;

class CPathIntersectionInfo
{
public:
    unsigned char m_bRoadCross : 1;
    unsigned char m_bPedTrafficLight : 1;
};

class CCarPathLinkAddress
{
public:
    unsigned short m_nCarPathLinkId : 10;
    unsigned short m_nAreaId : 6;
};

class CForbiddenArea
{
public:
    float         m_fX1, m_fX2, m_fY1, m_fY2, m_fZ1, m_fZ2;
    bool          m_bEnable;
    unsigned char m_nType;
private:
    char _pad1A[2];
};

class CPathFind
{
public:
    CNodeAddress            info;
    CPathNode* m_apNodesSearchLists[512];
    CPathNode* m_pPathNodes[NUM_PATH_TOTAL_AREAS];
    CCarPathLink* m_pNaviNodes[NUM_PATH_TOTAL_AREAS];
    CNodeAddress* m_pNodeLinks[NUM_PATH_TOTAL_AREAS];
    unsigned char* m_pLinkLengths[NUM_PATH_TOTAL_AREAS];
    CPathIntersectionInfo* m_pPathIntersections[NUM_PATH_TOTAL_AREAS];
    CCarPathLinkAddress* m_pNaviLinks[NUM_PATH_MAP_AREAS]; // only exterior areas
    void* field_EA4[NUM_PATH_MAP_AREAS];
    unsigned int            m_dwNumNodes[NUM_PATH_TOTAL_AREAS];
    unsigned int            m_dwNumVehicleNodes[NUM_PATH_TOTAL_AREAS];
    unsigned int            m_dwNumPedNodes[NUM_PATH_TOTAL_AREAS];
    unsigned int            m_dwNumCarPathLinks[NUM_PATH_TOTAL_AREAS];
    unsigned int            m_dwNumAddresses[NUM_PATH_TOTAL_AREAS];
    int                     field_1544[2048];
    unsigned int            m_dwTotalNumNodesInSearchList;
    CNodeAddress            char3548[8];
    unsigned int            m_dwNumForbiddenAreas;
    CForbiddenArea          m_aForbiddenAreas[64];
    bool                    m_bForbiddenForScriptedCarsEnabled;
    char                    _padding[3];
    float                   m_fForbiddenForScrCarsX1, m_fForbiddenForScrCarsX2;
    float                   m_fForbiddenForScrCarsY1, m_fForbiddenForScrCarsY2;

    // Sum vehicle nodes across all loaded areas
    int32 GetNumCarPathNodes() const
    {
        int32 total = 0;
        for (int i = 0; i < NUM_PATH_TOTAL_AREAS; i++)
            if (m_pPathNodes[i])
                total += static_cast<int32>(m_dwNumVehicleNodes[i]);
        return total;
    }

    // Returns the full CNodeAddress (area + node) for a link connection
    CNodeAddress GetConnectedAddress(int32 areaId, int32 linkId) const
    {
        return m_pNodeLinks[areaId][linkId];
    }

    // Lane link lookup — needs fromArea because m_pNaviLinks is per-area
    bool GetLaneLinkByConnection(int32 fromArea, int32 connection, CCarPathLink& laneLink) const
    {
        if (fromArea < 0 || fromArea >= NUM_PATH_MAP_AREAS)
            return false;
        auto* naviLinks = m_pNaviLinks[fromArea];
        if (!naviLinks)
            return false;
        const CCarPathLinkAddress& addr = naviLinks[connection];
        int32 areaId = (int32)addr.m_nAreaId;  // now correctly 0-63
        int32 linkId = (int32)addr.m_nCarPathLinkId;
        if (areaId >= NUM_PATH_TOTAL_AREAS)    // no longer need < 0 check
            return false;
        if (linkId >= static_cast<int32>(m_dwNumCarPathLinks[areaId]))
            return false;
        if (!m_pNaviNodes[areaId])
            return false;
        laneLink = m_pNaviNodes[areaId][linkId];
        return true;
    }

    bool IsAreaLoaded(int32 areaId) const
    {
        return areaId >= 0 && areaId < NUM_PATH_TOTAL_AREAS && m_pPathNodes[areaId] != nullptr;
    }
};

export GameRef<CPathFind> ThePaths([]() -> CPathFind*
{
    auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 85 C0 75");
    if (!pattern.empty())
        return *pattern.get_first<CPathFind*>(1);
    return nullptr;
});

namespace CCarCtrl
{
    export GameRef<float> CarDensityMultiplier([]() -> float*
    {
        auto pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? C6 05 ? ? ? ? ? A3");
        if (!pattern.empty())
            return *pattern.get_first<float*>(2);
        return nullptr;
    });
}

export void (__fastcall* MakeRequestForNodesToBeLoaded)(CPathFind* pf, void* edx, float minX, float maxX, float minY, float maxY) = nullptr;

export class CMovingThings
{
public:
    struct CDistantCarImpostor
    {
        bool     m_bActive;
        uint8_t  m_nPrevArea;  // area index for m_nPrevNode
        uint8_t  m_nNextArea;  // area index for m_nNextNode
        int16_t  m_nPrevNode;
        int16_t  m_nNextNode;
        float    m_fProgress;
        float    m_fSpeed;
        float    m_fDesiredSpeed;
        float    m_fLaneOffset;
        uint8_t  m_nLaneSide;
        uint8_t  m_nLaneCount;
        uint8_t  m_nLaneIndex;
        CVector  m_vecPos;
        CVector  m_vecDir;
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
    static bool PickNextNodeForImpostor(const CDistantCarImpostor& impostor, uint8_t& nextArea, int16_t& nextNode, CCarPathLink& laneLink);
    static bool FindLaneLinkForSegment(uint8_t fromArea, int16_t fromNode, uint8_t toArea, int16_t toNode, CCarPathLink& laneLink);
    static void EnsureDistantCarImpostorPoolSize();
};

std::vector<CMovingThings::CDistantCarImpostor> CMovingThings::aDistantCarImpostors;

static uint32 ImpostorCoronaId(int32 i) { return 0x7F000000 + i; }

static void HideImpostorCorona(CMovingThings::CDistantCarImpostor& impostor)
{
    CLODLights::RegisterCorona(impostor.m_nCoronaId, nullptr,
        0, 0, 0, 0,
        CVector(0.0f, 0.0f, 0.0f),
        0.0f, 0.0f,
        1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
}

export bool bExtendImpostorPathStreaming = true;
static void UpdateExtendedPathStreaming(const CVector& camPos, float farClip)
{
    if (!bExtendImpostorPathStreaming)
        return;

    MakeRequestForNodesToBeLoaded(ThePaths.get_ptr(), nullptr,
        camPos.x - farClip, camPos.x + farClip,
        camPos.y - farClip, camPos.y + farClip);
}

constexpr float PATH_WORLD_MIN = -3000.0f;
constexpr float PATH_WORLD_SIZE = 6000.0f;
constexpr int   PATH_GRID_W = 8;
constexpr int   PATH_GRID_H = 8;
constexpr float PATH_CELL_W = PATH_WORLD_SIZE / PATH_GRID_W;  // 750
constexpr float PATH_CELL_H = PATH_WORLD_SIZE / PATH_GRID_H;  // 750

static int  s_visibleAreas[NUM_PATH_MAP_AREAS];
static bool s_visibleAreaMask[NUM_PATH_MAP_AREAS];
static int  s_numVisibleAreas = 0;
static void RebuildVisibleAreaCache(const CVector& camPos, float maxDist)
{
    s_numVisibleAreas = 0;
    for (int i = 0; i < NUM_PATH_MAP_AREAS; i++)
        s_visibleAreaMask[i] = false;

    // AABB of the area that the camera can see (circle approximated as square)
    float camMinX = camPos.x - maxDist;
    float camMaxX = camPos.x + maxDist;
    float camMinY = camPos.y - maxDist;
    float camMaxY = camPos.y + maxDist;

    for (int i = 0; i < NUM_PATH_MAP_AREAS; i++)
    {
        if (!ThePaths->m_pPathNodes[i] || ThePaths->m_dwNumVehicleNodes[i] == 0)
            continue;

        int   gridX = i % PATH_GRID_W;
        int   gridY = i / PATH_GRID_W;
        float cellMinX = PATH_WORLD_MIN + gridX * PATH_CELL_W;
        float cellMinY = PATH_WORLD_MIN + gridY * PATH_CELL_H;
        float cellMaxX = cellMinX + PATH_CELL_W;
        float cellMaxY = cellMinY + PATH_CELL_H;

        // AABB overlap test between camera view rect and cell rect
        if (cellMaxX < camMinX || cellMinX > camMaxX) continue;
        if (cellMaxY < camMinY || cellMinY > camMaxY) continue;

        s_visibleAreas[s_numVisibleAreas++] = i;
        s_visibleAreaMask[i] = true;
    }

    // Fallback: any loaded area
    if (s_numVisibleAreas == 0)
    {
        for (int i = 0; i < NUM_PATH_MAP_AREAS; i++)
            if (ThePaths->m_pPathNodes[i] && ThePaths->m_dwNumVehicleNodes[i] > 0)
            {
                s_visibleAreas[s_numVisibleAreas++] = i;
                s_visibleAreaMask[i] = true;
            }
    }
}

static int32 GetRandomVisibleCarArea()
{
    if (s_numVisibleAreas == 0) return -1;
    return s_visibleAreas[CGeneral::GetRandomNumber() % s_numVisibleAreas];
}

static bool IsImpostorInVisibleArea(const CMovingThings::CDistantCarImpostor& imp)
{
    return imp.m_nPrevArea < NUM_PATH_MAP_AREAS && s_visibleAreaMask[imp.m_nPrevArea];
}

static uint64 MakeLaneKey(uint8 prevArea, int16 prevNode, uint8 nextArea, int16 nextNode, uint8 laneSide, uint8 laneIndex)
{
    return (uint64)prevArea |
        ((uint64)(uint16)prevNode << 8) |
        ((uint64)nextArea << 24) |
        ((uint64)(uint16)nextNode << 32) |
        ((uint64)laneSide << 48) |
        ((uint64)laneIndex << 56);
}

static thread_local std::unordered_map<uint64, std::vector<int32>> s_laneBuckets;
static thread_local bool s_laneBucketsDirty = true;

static void MarkLaneBucketsDirty()
{
    s_laneBucketsDirty = true;
}

static void RebuildLaneBuckets()
{
    s_laneBuckets.clear();
    s_laneBuckets.reserve(CMovingThings::aDistantCarImpostors.size());

    for (int32 i = 0; i < (int32)CMovingThings::aDistantCarImpostors.size(); i++)
    {
        const auto& imp = CMovingThings::aDistantCarImpostors[i];
        if (!imp.m_bActive)
            continue;

        uint64 key = MakeLaneKey(imp.m_nPrevArea, imp.m_nPrevNode, imp.m_nNextArea, imp.m_nNextNode, imp.m_nLaneSide, imp.m_nLaneIndex);
        s_laneBuckets[key].push_back(i);
    }

    s_laneBucketsDirty = false;
}

static const std::vector<int32>* GetLaneBucket(uint8 prevArea, int16 prevNode, uint8 nextArea, int16 nextNode, uint8 laneSide, uint8 laneIndex)
{
    if (s_laneBucketsDirty)
        RebuildLaneBuckets();

    uint64 key = MakeLaneKey(prevArea, prevNode, nextArea, nextNode, laneSide, laneIndex);
    auto it = s_laneBuckets.find(key);
    if (it == s_laneBuckets.end())
        return nullptr;
    return &it->second;
}

static bool IsPathSegmentExcludedForImpostor(uint8 fromArea, int16 fromNode, uint8 toArea, int16 toNode)
{
    if (!ThePaths->IsAreaLoaded(fromArea) || !ThePaths->IsAreaLoaded(toArea))
        return true;

    CPathNode& fromPathNode = ThePaths->m_pPathNodes[fromArea][fromNode];
    CPathNode& toPathNode = ThePaths->m_pPathNodes[toArea][toNode];

    if (fromPathNode.bDisabled || toPathNode.bDisabled)
        return true;

    return false;
}

static float ComputeDynamicImpostorDensityScale(float camTravelSpeed)
{
    float density = Clamp(CCarCtrl::CarDensityMultiplier, 0.25f, 1.4f);

    int32 hour = CClock::ms_nGameClockHours;
    if (hour <= 5)                  density *= 0.65f;
    else if (hour >= 7 && hour <= 9)    density *= 1.12f;
    else if (hour >= 16 && hour <= 18)   density *= 1.10f;

    density *= (1.0f - 0.22f * CWeather::Rain);
    density *= (1.0f - 0.16f * CWeather::Foggyness);
    density *= Clamp(0.9f + camTravelSpeed * 0.02f, 0.9f, 1.2f);

    return Clamp(density, 0.2f, 1.25f);
}

static float ComputeLaneOffset(bool useRightSide, int32 laneCount, int32 laneIndex, CCarPathLink link)
{
    laneCount = Max(1, laneCount);
    laneIndex = Clamp(laneIndex, 0, laneCount - 1);
    float sideSign = useRightSide ? -1.0f : 1.0f;
    return sideSign * (laneIndex + link.OneWayLaneOffset()) * 5.0f;
}

static bool IsTraversalAlongLinkDir(uint8 fromArea, int16 fromNode, uint8 toArea, int16 toNode, CCarPathLink& link)
{
    CVector fromPos = ThePaths->m_pPathNodes[fromArea][fromNode].GetPosition();
    CVector toPos = ThePaths->m_pPathNodes[toArea][toNode].GetPosition();
    CVector2D segDir(toPos.x - fromPos.x, toPos.y - fromPos.y);
    CVector2D linkDir = link.GetDirection();
    return DotProduct2D(segDir, linkDir) >= 0.0f;
}

static bool CanTraverseSegmentDirection(uint8 fromArea, int16 fromNode, uint8 toArea, int16 toNode, CCarPathLink& link)
{
    bool along = IsTraversalAlongLinkDir(fromArea, fromNode, toArea, toNode, link);
    return along ? (link.numRightLanes > 0) : (link.numLeftLanes > 0);
}

static bool UseRightLaneGroupForTraversal(uint8 fromArea, int16 fromNode, uint8 toArea, int16 toNode, CCarPathLink& link)
{
    return IsTraversalAlongLinkDir(fromArea, fromNode, toArea, toNode, link);
}

static bool ShouldKeepImpostorAliveNearCamera(const CMovingThings::CDistantCarImpostor& impostor, const CVector& camPos)
{
    return (impostor.m_vecPos - camPos).MagnitudeSqr2D() < SQR(380.0f);
}

static bool ComputeImpostorTransform(CMovingThings::CDistantCarImpostor& impostor)
{
    if (!ThePaths->IsAreaLoaded(impostor.m_nPrevArea) || !ThePaths->IsAreaLoaded(impostor.m_nNextArea))
        return false;
    if (impostor.m_nPrevNode < 0 || impostor.m_nPrevNode >= (int16)ThePaths->m_dwNumVehicleNodes[impostor.m_nPrevArea])
        return false;
    if (impostor.m_nNextNode < 0 || impostor.m_nNextNode >= (int16)ThePaths->m_dwNumVehicleNodes[impostor.m_nNextArea])
        return false;

    CVector fromPos = ThePaths->m_pPathNodes[impostor.m_nPrevArea][impostor.m_nPrevNode].GetPosition();
    CVector toPos = ThePaths->m_pPathNodes[impostor.m_nNextArea][impostor.m_nNextNode].GetPosition();
    CVector segment = toPos - fromPos;
    float segmentLen = segment.Magnitude2D();
    if (segmentLen < 0.001f)
        return false;

    CVector dir = segment / segmentLen;
    CVector right(-dir.y, dir.x, 0.0f);
    CVector pos = fromPos + segment * impostor.m_fProgress + right * impostor.m_fLaneOffset;
    pos.z += 0.55f;

    impostor.m_vecPos = pos;
    impostor.m_vecDir = dir;
    return true;
}

void CMovingThings::EnsureDistantCarImpostorPoolSize()
{
    int32 desired = Clamp((int32)nNumDistantCarImpostors, 64, 10000);

    size_t oldSize = aDistantCarImpostors.size();
    size_t newSize = static_cast<size_t>(desired);
    if (newSize == oldSize) return;

    if (newSize < oldSize)
    {
        for (size_t i = newSize; i < oldSize; ++i)
        {
            auto& imp = aDistantCarImpostors[i];
            if (imp.m_bActive) { imp.m_bActive = false; HideImpostorCorona(imp); }
        }
    }

    aDistantCarImpostors.resize(newSize);

    if (newSize > oldSize)
        for (size_t i = oldSize; i < newSize; ++i)
            InitDistantCarImpostor(aDistantCarImpostors[i], ImpostorCoronaId(static_cast<int32>(i)));
}

bool CMovingThings::FindLaneLinkForSegment(uint8 fromArea, int16 fromNode, uint8 toArea, int16 toNode, CCarPathLink& laneLink)
{
    if (!ThePaths->IsAreaLoaded(fromArea)) return false;
    if (fromNode < 0 || fromNode >= (int16)ThePaths->m_dwNumVehicleNodes[fromArea]) return false;

    CPathNode& node = ThePaths->m_pPathNodes[fromArea][fromNode];
    for (int32 i = 0; i < (int32)node.m_nNumLinks; i++)
    {
        int32 connection = node.m_wBaseLinkId + i;
        CNodeAddress connAddr = ThePaths->GetConnectedAddress(fromArea, connection);
        if (connAddr.m_nAreaId != toArea || connAddr.m_nNodeId != toNode)
            continue;
        if (!ThePaths->GetLaneLinkByConnection(fromArea, connection, laneLink))
            continue;
        return true;
    }
    return false;
}

bool CMovingThings::PickNextNodeForImpostor(const CDistantCarImpostor& impostor, uint8& nextArea, int16& nextNode, CCarPathLink& laneLink)
{
    uint8 curArea = impostor.m_nNextArea;
    if (!ThePaths->IsAreaLoaded(curArea)) return false;
    if (impostor.m_nNextNode < 0 || impostor.m_nNextNode >= (int16)ThePaths->m_dwNumVehicleNodes[curArea]) return false;

    CPathNode& node = ThePaths->m_pPathNodes[curArea][impostor.m_nNextNode];
    if (node.m_nNumLinks == 0) return false;

    uint8        areaChoices[16];
    int16        nodeChoices[16];
    CCarPathLink linkChoices[16];
    int32        numChoices = 0;

    for (int32 i = 0; i < (int32)node.m_nNumLinks && numChoices < 16; i++)
    {
        int32        connection = node.m_wBaseLinkId + i;
        CNodeAddress connAddr = ThePaths->GetConnectedAddress(curArea, connection);
        uint8        candArea = (uint8)connAddr.m_nAreaId;
        int16        candidate = (int16)connAddr.m_nNodeId;

        // Skip going back to previous node (unless it's the only option)
        if (candidate == impostor.m_nPrevNode && candArea == impostor.m_nPrevArea && node.m_nNumLinks > 1)
            continue;

        if (!ThePaths->IsAreaLoaded(candArea)) continue;

        CCarPathLink candidateLink;
        if (!FindLaneLinkForSegment(curArea, impostor.m_nNextNode, candArea, candidate, candidateLink)) continue;
        if (!CanTraverseSegmentDirection(curArea, impostor.m_nNextNode, candArea, candidate, candidateLink)) continue;
        if (IsPathSegmentExcludedForImpostor(curArea, impostor.m_nNextNode, candArea, candidate)) continue;

        areaChoices[numChoices] = candArea;
        nodeChoices[numChoices] = candidate;
        linkChoices[numChoices] = candidateLink;
        numChoices++;
    }

    if (numChoices == 0)
    {
        // Try reversing
        if (impostor.m_nPrevNode < 0) return false;
        if (!FindLaneLinkForSegment(curArea, impostor.m_nNextNode, impostor.m_nPrevArea, impostor.m_nPrevNode, laneLink)) return false;
        if (!CanTraverseSegmentDirection(curArea, impostor.m_nNextNode, impostor.m_nPrevArea, impostor.m_nPrevNode, laneLink)) return false;
        if (IsPathSegmentExcludedForImpostor(curArea, impostor.m_nNextNode, impostor.m_nPrevArea, impostor.m_nPrevNode)) return false;
        nextArea = impostor.m_nPrevArea;
        nextNode = impostor.m_nPrevNode;
        return true;
    }

    int32 pick = CGeneral::GetRandomNumber() % numChoices;
    nextArea = areaChoices[pick];
    nextNode = nodeChoices[pick];
    laneLink = linkChoices[pick];
    return true;
}

bool CMovingThings::InitDistantCarImpostor(CDistantCarImpostor& impostor, uint32 coronaId)
{
    impostor.m_bActive = false;
    impostor.m_nCoronaId = coronaId;
    MarkLaneBucketsDirty();

    for (int32 attempts = 0; attempts < 128; attempts++)
    {
        int32 areaIdx = GetRandomVisibleCarArea();
        if (areaIdx < 0) return false;
        uint8 fromArea = (uint8)areaIdx;

        int32 numNodes = (int32)ThePaths->m_dwNumVehicleNodes[fromArea];
        int16 fromNode = (int16)(CGeneral::GetRandomNumber() % numNodes);
        CPathNode& node = ThePaths->m_pPathNodes[fromArea][fromNode];
        if (node.m_nNumLinks == 0) continue;

        CNodeAddress connAddr = ThePaths->GetConnectedAddress(fromArea,
            node.m_wBaseLinkId + CGeneral::GetRandomNumber() % node.m_nNumLinks);
        uint8 toArea = (uint8)connAddr.m_nAreaId;
        int16 toNode = (int16)connAddr.m_nNodeId;

        if (!ThePaths->IsAreaLoaded(toArea)) continue;

        CCarPathLink laneLink;
        if (!FindLaneLinkForSegment(fromArea, fromNode, toArea, toNode, laneLink)) continue;
        if (!CanTraverseSegmentDirection(fromArea, fromNode, toArea, toNode, laneLink)) continue;
        if (IsPathSegmentExcludedForImpostor(fromArea, fromNode, toArea, toNode)) continue;

        bool useRightLaneGroup = UseRightLaneGroupForTraversal(fromArea, fromNode, toArea, toNode, laneLink);
        int8 leftLanes = Max((int8)1, (int8)laneLink.numLeftLanes);
        int8 rightLanes = Max((int8)1, (int8)laneLink.numRightLanes);
        int8 laneCount = useRightLaneGroup ? rightLanes : leftLanes;
        int8 lane = (int8)(CGeneral::GetRandomNumber() % laneCount);
        uint8 laneSide = useRightLaneGroup ? 1 : 0;

        static std::vector<float> occupied, gapStart, gapEnd;
        occupied.clear();

        CVector fromNodePos = node.GetPosition();
        CVector toNodePos = ThePaths->m_pPathNodes[toArea][toNode].GetPosition();
        float segLen = (toNodePos - fromNodePos).Magnitude2D();
        float minGap = (segLen > 0.001f) ? Min(0.35f, 16.0f / segLen) : 0.35f;

        if (const auto* laneBucket = GetLaneBucket(fromArea, fromNode, toArea, toNode, laneSide, (uint8)lane))
        {
            occupied.reserve(laneBucket->size());
            for (int32 idx : *laneBucket)
            {
                const auto& other = aDistantCarImpostors[idx];
                if (!other.m_bActive)
                    continue;
                occupied.push_back(other.m_fProgress);
            }
        }

        int32 numOccupied = (int32)occupied.size();
        for (int32 a = 1; a < numOccupied; a++)
        {
            float v = occupied[a]; int32 b = a - 1;
            while (b >= 0 && occupied[b] > v) { occupied[b + 1] = occupied[b]; b--; }
            occupied[b + 1] = v;
        }

        float spawnProgress = -1.0f;
        if (numOccupied == 0)
        {
            spawnProgress = (CGeneral::GetRandomNumber() & 0xFF) / 255.0f;
        }
        else
        {
            int32 numGaps = numOccupied + 1;
            gapStart.resize(numGaps);
            gapEnd.resize(numGaps);
            int32 numFree = 0;

            { float gs = 0.0f, ge = occupied[0] - minGap; if (ge > gs + minGap) { gapStart[numFree] = gs; gapEnd[numFree] = ge; numFree++; } }
            for (int32 g = 0; g < numOccupied - 1; g++)
            {
                float gs = occupied[g] + minGap, ge = occupied[g + 1] - minGap;
                if (ge > gs) { gapStart[numFree] = gs; gapEnd[numFree] = ge; numFree++; }
            }
            { float gs = occupied[numOccupied - 1] + minGap, ge = 1.0f; if (ge > gs + minGap) { gapStart[numFree] = gs; gapEnd[numFree] = ge; numFree++; } }

            if (numFree > 0)
            {
                int32 pick2 = CGeneral::GetRandomNumber() % numFree;
                float t = (CGeneral::GetRandomNumber() & 0xFF) / 255.0f;
                spawnProgress = gapStart[pick2] + t * (gapEnd[pick2] - gapStart[pick2]);
            }
        }

        if (spawnProgress < 0.0f) continue;

        impostor.m_bActive = true;
        impostor.m_nPrevArea = fromArea;
        impostor.m_nNextArea = toArea;
        impostor.m_nPrevNode = fromNode;
        impostor.m_nNextNode = toNode;
        impostor.m_fProgress = Clamp(spawnProgress, 0.0f, 1.0f);
        impostor.m_fDesiredSpeed = 9.0f + (float)(CGeneral::GetRandomNumber() % 18);
        impostor.m_fSpeed = impostor.m_fDesiredSpeed;
        impostor.m_nLaneSide = laneSide;
        impostor.m_nLaneCount = (uint8)laneCount;
        impostor.m_nLaneIndex = (uint8)lane;
        impostor.m_fLaneOffset = ComputeLaneOffset(true, laneCount, lane, laneLink);
        impostor.m_vecPos = node.GetPosition();
        impostor.m_vecDir = CVector(1.0f, 0.0f, 0.0f);
        impostor.m_nStuckFrames = 0;
        MarkLaneBucketsDirty();
        return true;
    }

    return false;
}

void CMovingThings::InitDistantCarImpostors()
{
    EnsureDistantCarImpostorPoolSize();
    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
        InitDistantCarImpostor(aDistantCarImpostors[i], ImpostorCoronaId((int32)i));
}

void CMovingThings::ShutdownDistantCarImpostors()
{
    for (auto& imp : aDistantCarImpostors)
    {
        imp.m_bActive = false;
        HideImpostorCorona(imp);
    }
    MarkLaneBucketsDirty();
}

void CMovingThings::UpdateDistantCarImpostors()
{
    float   dt = CTimer::GetTimeStepInSeconds();
    CVector camPos = TheCamera->GetCoords();
    float   maxDist = CTimeCycle::m_fCurrentFarClip;

    UpdateExtendedPathStreaming(camPos, maxDist);

    int32 totalCarNodes = ThePaths->GetNumCarPathNodes();
    if (totalCarNodes <= 0)
        return;

    RebuildVisibleAreaCache(camPos, maxDist);

    float evictDistSqr = (maxDist * 1.1f) * (maxDist * 1.1f);
    for (auto& imp : aDistantCarImpostors)
    {
        if (!imp.m_bActive) continue;
        if (ShouldKeepImpostorAliveNearCamera(imp, camPos)) continue; // always keep close ones

        bool beyondFarClip = (imp.m_vecPos - camPos).MagnitudeSqr2D() > evictDistSqr;
        bool areaUnloaded = !ThePaths->IsAreaLoaded(imp.m_nPrevArea);
        bool areaNotVisible = !IsImpostorInVisibleArea(imp);

        if (beyondFarClip || areaUnloaded || areaNotVisible)
        {
            imp.m_bActive = false;
            HideImpostorCorona(imp);
            MarkLaneBucketsDirty();
        }
    }

    static bool    sHasPrevCamPos = false;
    static CVector sPrevCamPos;
    float camTravelSpeed = 0.0f;
    if (sHasPrevCamPos && dt > 0.0001f)
        camTravelSpeed = (camPos - sPrevCamPos).Magnitude2D() / dt;
    sPrevCamPos = camPos;
    sHasPrevCamPos = true;

    float densityScale = ComputeDynamicImpostorDensityScale(camTravelSpeed);
    int32 impostorCount = (int32)aDistantCarImpostors.size();
    int32 desiredActive = (int32)Clamp(impostorCount * densityScale, 64.0f, (float)impostorCount);

    int32 activeCount = 0;
    for (const auto& imp : aDistantCarImpostors)
        if (imp.m_bActive) activeCount++;

    int32 toDisable = Max(0, activeCount - desiredActive);
    if (toDisable > 0 && camTravelSpeed < 8.0f)
    {
        int32 disableBudget = Min(8, toDisable);
        for (auto& imp : aDistantCarImpostors)
        {
            if (disableBudget <= 0) break;
            if (!imp.m_bActive) continue;
            if (ShouldKeepImpostorAliveNearCamera(imp, camPos)) continue;
            if ((imp.m_vecPos - camPos).MagnitudeSqr2D() < SQR(700.0f)) continue;
            imp.m_bActive = false;
            HideImpostorCorona(imp);
            MarkLaneBucketsDirty();
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
            if (spawnBudget <= 0) continue;
            if ((CGeneral::GetRandomNumber() & 3) != 0) continue;
            if (InitDistantCarImpostor(impostor, ImpostorCoronaId((int32)i)))
            {
                activeCount++;
                spawnBudget--;
            }
            continue;
        }

        // Validate current nodes are still accessible
        bool prevOk = ThePaths->IsAreaLoaded(impostor.m_nPrevArea) &&
            impostor.m_nPrevNode >= 0 &&
            impostor.m_nPrevNode < (int16)ThePaths->m_dwNumVehicleNodes[impostor.m_nPrevArea];
        bool nextOk = ThePaths->IsAreaLoaded(impostor.m_nNextArea) &&
            impostor.m_nNextNode >= 0 &&
            impostor.m_nNextNode < (int16)ThePaths->m_dwNumVehicleNodes[impostor.m_nNextArea];

        if (!prevOk || !nextOk)
        {
            if (ShouldKeepImpostorAliveNearCamera(impostor, camPos) && impostor.m_nStuckFrames < 120)
            {
                impostor.m_nStuckFrames++;
                continue;
            }
            InitDistantCarImpostor(impostor, impostor.m_nCoronaId);
            continue;
        }

        if (IsPathSegmentExcludedForImpostor(impostor.m_nPrevArea, impostor.m_nPrevNode, impostor.m_nNextArea, impostor.m_nNextNode))
        {
            if (!InitDistantCarImpostor(impostor, impostor.m_nCoronaId))
            {
                impostor.m_bActive = false;
                HideImpostorCorona(impostor);
                MarkLaneBucketsDirty();
            }
            continue;
        }

        CVector fromPos = ThePaths->m_pPathNodes[impostor.m_nPrevArea][impostor.m_nPrevNode].GetPosition();
        CVector toPos = ThePaths->m_pPathNodes[impostor.m_nNextArea][impostor.m_nNextNode].GetPosition();
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
        impostor.m_fProgress += impostor.m_fSpeed * dt / segmentLen;

        while (impostor.m_fProgress >= 1.0f)
        {
            impostor.m_fProgress -= 1.0f;

            CCarPathLink nextLink;
            uint8 nextArea; int16 nextNode;
            if (!PickNextNodeForImpostor(impostor, nextArea, nextNode, nextLink))
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

            uint8 targetPrevArea = impostor.m_nNextArea;
            int16 targetPrevNode = impostor.m_nNextNode;
            uint8 targetNextArea = nextArea;
            int16 targetNextNode = nextNode;

            bool useRight = UseRightLaneGroupForTraversal(targetPrevArea, targetPrevNode, targetNextArea, targetNextNode, nextLink);
            int8 leftLanes = Max((int8)1, (int8)nextLink.numLeftLanes);
            int8 rightLanes = Max((int8)1, (int8)nextLink.numRightLanes);
            int8 laneCount = Max((int8)1, useRight ? rightLanes : leftLanes);
            uint8 laneSide = useRight ? 1 : 0;
            uint8 laneIndex = Min((uint8)(laneCount - 1), impostor.m_nLaneIndex);

            CVector tgFrom = ThePaths->m_pPathNodes[targetPrevArea][targetPrevNode].GetPosition();
            CVector tgTo = ThePaths->m_pPathNodes[targetNextArea][targetNextNode].GetPosition();
            float tgSegLen = (tgTo - tgFrom).Magnitude2D();
            float minEntryGap = (tgSegLen > 0.001f) ? Min(0.35f, 16.0f / tgSegLen) : 0.35f;

            bool canEnter = true;
            if (const auto* laneBucket = GetLaneBucket(targetPrevArea, targetPrevNode, targetNextArea, targetNextNode, laneSide, laneIndex))
            {
                for (int32 idx : *laneBucket)
                {
                    const auto& other = aDistantCarImpostors[idx];
                    if (!other.m_bActive || &other == &impostor)
                        continue;
                    if (other.m_fProgress < minEntryGap)
                    {
                        canEnter = false;
                        break;
                    }
                }
            }

            if (!canEnter)
            {
                impostor.m_fProgress = 0.999f;
                impostor.m_fSpeed = 0.0f;
                if (impostor.m_nStuckFrames < 200) impostor.m_nStuckFrames++;
                if (impostor.m_nStuckFrames > 120 && !ShouldKeepImpostorAliveNearCamera(impostor, camPos))
                {
                    if (!InitDistantCarImpostor(impostor, impostor.m_nCoronaId))
                    {
                        impostor.m_bActive = false;
                        HideImpostorCorona(impostor);
                        MarkLaneBucketsDirty();
                    }
                }
                break;
            }

            impostor.m_nPrevArea = targetPrevArea;
            impostor.m_nNextArea = targetNextArea;
            impostor.m_nPrevNode = targetPrevNode;
            impostor.m_nNextNode = targetNextNode;
            impostor.m_nLaneSide = laneSide;
            impostor.m_nLaneCount = (uint8)laneCount;
            impostor.m_nLaneIndex = laneIndex;
            impostor.m_fLaneOffset = ComputeLaneOffset(true, laneCount, laneIndex, nextLink);
            impostor.m_nStuckFrames = 0;
            MarkLaneBucketsDirty();
        }

        if (!impostor.m_bActive) continue;
        if (!ComputeImpostorTransform(impostor)) continue;
    }

    const float desiredHeadway = 18.0f;
    static std::vector<int32> sortedIdx;
    sortedIdx.clear();
    sortedIdx.reserve(aDistantCarImpostors.size());
    for (size_t i = 0; i < aDistantCarImpostors.size(); i++)
        if (aDistantCarImpostors[i].m_bActive)
            sortedIdx.push_back((int32)i);

    int32 sortedCount = (int32)sortedIdx.size();

    std::sort(sortedIdx.begin(), sortedIdx.end(), [](int32 a, int32 b)
    {
        const CDistantCarImpostor& lhs = CMovingThings::aDistantCarImpostors[a];
        const CDistantCarImpostor& rhs = CMovingThings::aDistantCarImpostors[b];

        if (lhs.m_nPrevArea != rhs.m_nPrevArea) return lhs.m_nPrevArea < rhs.m_nPrevArea;
        if (lhs.m_nPrevNode != rhs.m_nPrevNode) return lhs.m_nPrevNode < rhs.m_nPrevNode;
        if (lhs.m_nNextArea != rhs.m_nNextArea) return lhs.m_nNextArea < rhs.m_nNextArea;
        if (lhs.m_nNextNode != rhs.m_nNextNode) return lhs.m_nNextNode < rhs.m_nNextNode;
        if (lhs.m_nLaneSide != rhs.m_nLaneSide) return lhs.m_nLaneSide < rhs.m_nLaneSide;
        if (lhs.m_nLaneIndex != rhs.m_nLaneIndex) return lhs.m_nLaneIndex < rhs.m_nLaneIndex;
        return lhs.m_fProgress > rhs.m_fProgress;
    });

    for (int32 pass = 0; pass < 3; pass++)
    {
        for (int32 si = 1; si < sortedCount; si++)
        {
            CDistantCarImpostor& curr = aDistantCarImpostors[sortedIdx[si]];
            CDistantCarImpostor& prev = aDistantCarImpostors[sortedIdx[si - 1]];

            if (prev.m_nPrevArea != curr.m_nPrevArea || prev.m_nPrevNode != curr.m_nPrevNode)  continue;
            if (prev.m_nNextArea != curr.m_nNextArea || prev.m_nNextNode != curr.m_nNextNode)  continue;
            if (prev.m_nLaneSide != curr.m_nLaneSide || prev.m_nLaneIndex != curr.m_nLaneIndex) continue;

            CVector fp = ThePaths->m_pPathNodes[curr.m_nPrevArea][curr.m_nPrevNode].GetPosition();
            CVector tp = ThePaths->m_pPathNodes[curr.m_nNextArea][curr.m_nNextNode].GetPosition();
            float segLen = (tp - fp).Magnitude2D();
            if (segLen < 0.001f) continue;

            float minGap = Min(0.35f, desiredHeadway / segLen);
            float gap = prev.m_fProgress - curr.m_fProgress;
            float gapMeters = gap * segLen;

            if (gapMeters < desiredHeadway * 2.0f)
            {
                float followFactor = Clamp((gapMeters - 2.0f) / (desiredHeadway * 2.0f), 0.0f, 1.0f);
                curr.m_fSpeed = Min(curr.m_fSpeed, prev.m_fSpeed * (0.2f + 0.8f * followFactor));
            }

            float maxSafeSpeed = (dt > 0.0001f) ? (Max(0.0f, gap - minGap) * segLen / dt) : 0.0f;
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
    CVector camPos = TheCamera->GetCoords();

    for (auto& impostor : aDistantCarImpostors)
    {
        if (!impostor.m_bActive) continue;

        float   maxDist = CTimeCycle::m_fCurrentFarClip;
        float   distSqr = (impostor.m_vecPos - camPos).MagnitudeSqr2D();
        if (distSqr < SQR(140.0f) || distSqr > SQR(maxDist)) continue;

        float dist = Sqrt(distSqr);
        if (dist < 0.001f) continue;

        bool approaching = DotProduct(impostor.m_vecDir, camPos - impostor.m_vecPos) > 0.0f;

        uint8 red = 255;
        uint8 green = approaching ? 255 : 40;
        uint8 blue = approaching ? 230 : 40;
        float fade = Clamp((maxDist - dist) / 250.0f, 0.0f, 1.0f)
            * Clamp((dist - 140.0f) / 120.0f, 0.0f, 1.0f);
        uint8 alpha = (uint8)(150 * fade);
        if (alpha == 0) continue;

        CLODLights::RegisterCorona(impostor.m_nCoronaId, nullptr,
            red, green, blue, alpha,
            impostor.m_vecPos,
            4.0f * fDistantCarsRadiusMultiplier,
            maxDist,
            1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
    }
}