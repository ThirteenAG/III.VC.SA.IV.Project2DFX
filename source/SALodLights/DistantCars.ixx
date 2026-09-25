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

    float GetNodePathWidth() const { return m_nPathNodeWidth / 16.0f; }

    CVector2D GetPosition() { return CVector2D(m_vecPosn.x / 8.0f, m_vecPosn.y / 8.0f); }
    CVector2D GetDirection() { return CVector2D(m_nDirX / 100.0f, m_nDirY / 100.0f); }

    float OneWayLaneOffset() const
    {
        if (numLeftLanes == 0)
            return 0.5f - (float)numRightLanes / 2.0f;
        if (numRightLanes == 0)
            return 0.5f - (float)numLeftLanes / 2.0f;
        return 0.5f;
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


export bool bExtendImpostorPathStreaming = true;
struct TrafficGraph
{
    using Node = DistantTraffic::Node;
    using Edge = DistantTraffic::Edge;
    static unsigned Area(Node id) { return id >> 16; }
    static unsigned Index(Node id) { return id & 0xFFFF; }
    static bool NodeValid(Node id)
    {
        unsigned area = Area(id), index = Index(id);
        return area < NUM_PATH_MAP_AREAS && ThePaths->IsAreaLoaded(area) && index < ThePaths->m_dwNumVehicleNodes[area] &&
               !ThePaths->m_pPathNodes[area][index].bDisabled && ThePaths->m_pPathNodes[area][index].GetPosition().z < 500.0f;
    }
    static bool Valid(const Edge& edge) { return NodeValid(edge.from) && NodeValid(edge.to) && (!edge.water || bDistantMaritimeTraffic); }
    static unsigned Degree(Node id) { return NodeValid(id) ? ThePaths->m_pPathNodes[Area(id)][Index(id)].m_nNumLinks : 0; }
    static Node RandomNode(uint32_t random)
    {
        unsigned total = 0;
        for (unsigned area = 0; area < NUM_PATH_MAP_AREAS; ++area)
            if (ThePaths->IsAreaLoaded(area))
                total += ThePaths->m_dwNumVehicleNodes[area];
        if (!total)
            return DistantTraffic::InvalidNode;
        unsigned offset = random % total;
        for (unsigned area = 0; area < NUM_PATH_MAP_AREAS; ++area)
        {
            if (!ThePaths->IsAreaLoaded(area))
                continue;
            unsigned count = ThePaths->m_dwNumVehicleNodes[area];
            if (offset < count)
                return (area << 16) | offset;
            offset -= count;
        }
        return DistantTraffic::InvalidNode;
    }
    static size_t Outgoing(Node from, std::array<Edge, 16>& result)
    {
        if (!NodeValid(from))
            return 0;
        unsigned area = Area(from);
        auto& a = ThePaths->m_pPathNodes[area][Index(from)];
        if (!ThePaths->m_pNodeLinks[area])
            return 0;
        size_t count = 0;
        for (unsigned i = 0; i < a.m_nNumLinks && count < result.size(); ++i)
        {
            int connection = a.m_wBaseLinkId + i;
            if (connection < 0 || static_cast<unsigned>(connection) >= ThePaths->m_dwNumAddresses[area])
                continue;
            auto address = ThePaths->GetConnectedAddress(area, connection);
            Node to = (static_cast<uint32_t>(static_cast<uint16_t>(address.m_nAreaId)) << 16) | static_cast<uint16_t>(address.m_nNodeId);
            if (!NodeValid(to) || to == from)
                continue;
            auto& b = ThePaths->m_pPathNodes[Area(to)][Index(to)];
            if (a.m_bWaterNode != b.m_bWaterNode || (a.m_bWaterNode && !bDistantMaritimeTraffic))
                continue;
            CCarPathLink link;
            if (!ThePaths->GetLaneLinkByConnection(area, connection, link))
                continue;
            // PathFind::DoPathSearch(sameLaneOnly) uses the attached address to
            // choose opposite/same-direction lanes. Node IDs alone are not unique.
            bool attached = link.m_address.m_nAreaId == address.m_nAreaId && link.m_address.m_nNodeId == address.m_nNodeId;
            unsigned lanes = attached ? link.numLeftLanes : link.numRightLanes;
            if (!lanes)
                continue;
            CVector start = a.GetPosition(), end = b.GetPosition(), segment = end - start;
            float length = segment.Magnitude2D();
            if (length < 1.0f)
                continue;
            CVector2D p = link.GetPosition(), d = link.GetDirection();
            float sign = (d.x * segment.x + d.y * segment.y) >= 0 ? 1.0f : -1.0f;
            CVector direction(d.x * sign, d.y * sign, segment.z / length);
            direction.Normalise();
            float t = (std::clamp)(((p.x - start.x) * segment.x + (p.y - start.y) * segment.y) / (length * length), 0.0f, 1.0f);
            Edge edge;
            edge.spawnRate = (std::min)(unsigned(a.m_nSpawnProbability), unsigned(b.m_nSpawnProbability));
            edge.from = from;
            edge.to = to;
            edge.lanes = lanes;
            edge.position = {p.x, p.y, start.z + segment.z * t};
            edge.direction = direction;
            // SA's lane spacing is 5.4m; the signed width byte is fixed-point
            // road width, NOT an independently varying lane spacing.
            edge.laneWidth = 5.4f;
            edge.laneOffset = (link.numLeftLanes == 0 || link.numRightLanes == 0) ? (.5f - .5f * lanes) : .5f;
            edge.water = a.m_bWaterNode;
            // SA's direction bit has the opposite sense to III/VC.
            if (!edge.water && attached == bool(link.m_bTrafficLightDirection))
                edge.signal = link.m_nTrafficLightState;
            edge.speed = edge.water ? 8.0f : (a.m_bHighway ? 23.0f : 16.0f);
            result[count++] = edge;
        }
        return count;
    }
    static bool SpawnAllowed(const Edge& edge) { return !edge.water || bDistantMaritimeTraffic; }
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
    CVector camera = TheCamera->GetCoords();
    float farClip = CTimeCycle::m_fCurrentFarClip;
    if (bExtendImpostorPathStreaming && MakeRequestForNodesToBeLoaded)
        MakeRequestForNodesToBeLoaded(ThePaths.get_ptr(), nullptr, camera.x - farClip, camera.x + farClip, camera.y - farClip, camera.y + farClip);
    float density = (std::clamp)(static_cast<float>(CCarCtrl::CarDensityMultiplier), 0.0f, 1.0f);
    int hour = CClock::ms_nGameClockHours;
    if (hour <= 5)
        density *= .65f;
    density *= 1.0f - .22f * static_cast<float>(CWeather::Rain);
    density *= 1.0f - .16f * static_cast<float>(CWeather::Foggyness);
    traffic.SetSignals(CTrafficLights::StopForCars(1), CTrafficLights::StopForCars(2));
    traffic.Update(CTimer::GetTimeStepInSeconds(), static_cast<size_t>((std::clamp)(nNumDistantCarImpostors, 0, 10000)), density, camera, farClip);
}

void CMovingThings::RenderDistantCarImpostors()
{
    if (nNumDistantCarImpostors <= 0 || aDistantCarImpostors.empty())
        return;

    CVector camPos = TheCamera->GetCoords();
    float maxDist = CTimeCycle::m_fCurrentFarClip;
    DistantCarRenderer::Frame models(camPos, maxDist);

    float nearScale = DistantTraffic::ViewDistanceScale(TheCamera->Cams[TheCamera->ActiveCam].FOV);
    for (auto& impostor : aDistantCarImpostors)
    {
        if (!impostor.m_bActive)
            continue;

        if (impostor.m_bWaterNode && !bDistantMaritimeTraffic)
            continue;

        float distSqr = (traffic.RenderPosition(impostor) - camPos).MagnitudeSqr2D();
        if (distSqr < SQR(140.0f) || distSqr > SQR(maxDist))
            continue;

        float dist = Sqrt(distSqr);
        if (dist < 0.001f)
            continue;

        bool approaching = DotProduct(traffic.RenderDirection(impostor), camPos - traffic.RenderPosition(impostor)) > 0.0f;

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
        float fade = Clamp((maxDist - dist) / 250.0f, 0.0f, 1.0f) * Clamp((dist * nearScale - 140.0f) / 120.0f, 0.0f, 1.0f);

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
