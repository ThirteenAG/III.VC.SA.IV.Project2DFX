#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdint>

// Shared traffic geometry and simulation, independent of game memory layouts.
// See re3/reVC CarCtrl::UpdateCarOnRails and CCurves: cars travel between the
// lane-offset car-path LINKS, with a continuous tangent through each link.
namespace DistantTraffic
{
    using Node = uint32_t;
    constexpr Node InvalidNode = UINT32_MAX;

    struct Edge
    {
        Node from = InvalidNode, to = InvalidNode;
        CVector position = {0, 0, 0}, direction = {0, 1, 0};
        float laneOffset = .5f, laneWidth = 5.0f, speed = 16.0f;
        unsigned lanes = 0;
        unsigned spawnRate = 15;
        bool water = false;
        CVector Lane(unsigned index) const
        {
            CVector right(direction.y, -direction.x, 0);
            right.Normalise();
            return position + right * ((static_cast<float>(index) + laneOffset) * laneWidth);
        }
    };

    struct Curve
    {
        CVector start = {0, 0, 0}, finish = {0, 0, 0};
        CVector bendStart = {0, 0, 0}, bendFinish = {0, 0, 0};
        CVector tangent0 = {0, 1, 0}, tangent1 = {0, 1, 0};
        std::array<float, 33> arc{};
        float length = 0, leadLength = 0, trailLength = 0;

        CVector BendPoint(float t) const
        {
            float t2 = t * t, t3 = t2 * t;
            return bendStart * (2 * t3 - 3 * t2 + 1) + tangent0 * (t3 - 2 * t2 + t) + bendFinish * (-2 * t3 + 3 * t2) + tangent1 * (t3 - t2);
        }
        CVector Point(float t) const
        {
            float section = (std::clamp)(t, 0.0f, 1.0f) * length;
            if (leadLength > 0 && section < leadLength)
                return start + (bendStart - start) * (section / leadLength);
            if (trailLength > 0 && section > length - trailLength)
                return bendFinish + (finish - bendFinish) * ((section - length + trailLength) / trailLength);
            return BendPoint((std::clamp)((section - leadLength) / arc.back(), 0.0f, 1.0f));
        }
        CVector Direction(float t) const
        {
            float section = (std::clamp)(t, 0.0f, 1.0f) * length;
            CVector d;
            if (section < leadLength)
                d = bendStart - start;
            else if (section > length - trailLength)
                d = finish - bendFinish;
            else
            {
                float u = (std::clamp)((section - leadLength) / arc.back(), 0.0f, 1.0f);
                d = bendStart * (6 * u * u - 6 * u) + tangent0 * (3 * u * u - 4 * u + 1) + bendFinish * (-6 * u * u + 6 * u) + tangent1 * (3 * u * u - 2 * u);
            }
            if (d.MagnitudeSqr() < .0001f)
                d = tangent0;
            d.Normalise();
            return d;
        }
        bool Build(const Edge& a, unsigned laneA, const Edge& b, unsigned laneB)
        {
            start = a.Lane(laneA);
            finish = b.Lane(laneB);
            float chord = (finish - start).Magnitude();
            if (!std::isfinite(chord) || chord < 1.0f || chord > 600.0f)
                return false;
            bendStart = start;
            bendFinish = finish;
            leadLength = trailLength = 0;
            float dot = DotProduct(a.direction, b.direction);
            float cross = a.direction.x * b.direction.y - a.direction.y * b.direction.x;
            if (dot < .95f && std::abs(cross) > .05f)
            {
                CVector delta = finish - start;
                float approach = (delta.x * b.direction.y - delta.y * b.direction.x) / cross;
                float departure = (a.direction.x * delta.y - a.direction.y * delta.x) / cross;
                if (approach > 0 && departure > 0 && approach + departure < chord * 3.0f)
                {
                    // Keep a long road straight until the junction. A single
                    // spline spanning widely separated link midpoints cuts across
                    // entire blocks; only round the actual corner/lane intersection.
                    float radius = (std::min)(a.water ? 35.0f : 10.0f, (std::min)(approach, departure) * .5f);
                    bendStart = start + a.direction * (approach - radius);
                    bendFinish = finish - b.direction * (departure - radius);
                    leadLength = (bendStart - start).Magnitude();
                    trailLength = (finish - bendFinish).Magnitude();
                }
            }
            float scale = (bendFinish - bendStart).Magnitude() * (1.0f + .2f * (1.0f - dot));
            tangent0 = a.direction * scale;
            tangent1 = b.direction * scale;
            arc[0] = 0;
            CVector last = bendStart;
            for (size_t i = 1; i < arc.size(); ++i)
            {
                CVector p = BendPoint(static_cast<float>(i) / 32.0f);
                arc[i] = arc[i - 1] + (p - last).Magnitude();
                last = p;
            }
            length = leadLength + arc.back() + trailLength;
            return std::isfinite(length) && arc.back() > .01f;
        }
        float Parameter(float distance) const
        {
            distance = (std::clamp)(distance, 0.0f, length);
            if (distance <= leadLength || distance >= length - trailLength)
                return distance / length;
            float onBend = distance - leadLength;
            auto it = std::lower_bound(arc.begin() + 1, arc.end(), onBend);
            size_t i = static_cast<size_t>(it - arc.begin());
            if (i >= arc.size())
                return (length - trailLength) / length;
            float span = arc[i] - arc[i - 1];
            float t = (static_cast<float>(i - 1) + (span > .00001f ? (onBend - arc[i - 1]) / span : 0)) / 32.0f;
            return (leadLength + t * arc.back()) / length;
        }
    };

    template <class Visual> struct Car
    {
        bool m_bActive = false, m_bWaterNode = false;
        CVector m_vecPos = {0, 0, 0}, m_vecDir = {0, 1, 0};
        CVector previousPosition = {0, 0, 0}, previousDirection = {0, 1, 0};
        uint32_t m_nCoronaId = 0;
        Visual m_visual;
        Edge entry, exit;
        unsigned lane = 0, exitLane = 0;
        Curve curve;
        float distance = 0, speed = 0, cruise = 0, waiting = 0;
        bool retiring = false;
        uint32_t random = 1;
    };

    // Graph supplies validated outgoing edges; no game-memory pointers survive a
    // frame. SA can unload a path area without invalidating a cached curve.
    template <class Graph, class Visual, bool AvoidCongestion = false> class Simulation
    {
        using Vehicle = Car<Visual>;
        float accumulator = 0;
        std::vector<float> advances, speeds;
        std::vector<CVector> lookPositions, lookDirections, nextPositions, nextDirections;
        std::vector<std::array<CVector, 6>> yieldPositions, yieldDirections;
        std::vector<size_t> yieldCount;
        // Intrusive bucket chains reuse one link per pool slot. Exact cell keys
        // distinguish hash collisions without allocating a list for each cell.
        static constexpr size_t NoCar = size_t(-1);
        std::vector<size_t> cells, nextCell;
        std::vector<int64_t> cellKeys;

        size_t Bucket(int64_t key) const
        {
            uint64_t hash = static_cast<uint64_t>(key);
            hash ^= hash >> 33;
            hash *= 0xff51afd7ed558ccdULL;
            hash ^= hash >> 33;
            return static_cast<size_t>(hash) & (cells.size() - 1);
        }
        void InsertCell(size_t i)
        {
            const auto& p = cars[i].m_vecPos;
            int64_t key = Cell(static_cast<int>(std::floor(p.x / 40.0f)), static_cast<int>(std::floor(p.y / 40.0f)));
            size_t bucket = Bucket(key);
            cellKeys[i] = key;
            nextCell[i] = cells[bucket];
            cells[bucket] = i;
        }
        void BuildCells()
        {
            size_t buckets = 1;
            while (buckets < cars.size() * 2)
                buckets *= 2;
            cells.resize(buckets);
            std::fill(cells.begin(), cells.end(), NoCar);
            nextCell.resize(cars.size());
            cellKeys.resize(cars.size());
            // Keep neighbours in pool order, as in the original cell lists.
            for (size_t i = cars.size(); i-- > 0;)
                if (cars[i].m_bActive)
                    InsertCell(i);
        }
        size_t spawnCursor = 0;

        static uint32_t Random(uint32_t& seed)
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            return seed;
        }
        static int64_t Cell(int x, int y) { return static_cast<int64_t>((uint64_t(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y)); }
        static void Position(Vehicle& car)
        {
            float t = car.curve.Parameter(car.distance);
            car.m_vecPos = car.curve.Point(t) + CVector(0, 0, .55f);
            car.m_vecDir = car.curve.Direction(t);
        }
        static bool Choose(Vehicle& car, const Edge& entry, Edge& next, unsigned& lane, Curve& curve, bool respectLanes = AvoidCongestion)
        {
            std::array<Edge, 16> options;
            size_t count = Graph::Outgoing(entry.to, options);
            float total = 0;
            bool chosen = false;
            for (size_t i = 0; i < count; ++i)
            {
                auto& edge = options[i];
                if (edge.to == entry.from || edge.water != entry.water || !edge.lanes)
                    continue;
                float dot = DotProduct(entry.direction, edge.direction);
                if (dot < -.75f)
                    continue; // No instantaneous U-turns at dead ends.
                if constexpr (AvoidCongestion)
                {
                    // re3 PickNextNodeRandomly: left turns use the innermost
                    // lane, right turns the outermost. Crossing several lanes
                    // at once creates mutually blocked cars at small junctions.
                    float cross = entry.direction.x * edge.direction.y - entry.direction.y * edge.direction.x;
                    if (respectLanes && ((cross > .77f && car.exitLane != 0) ||
                        (cross < -.77f && car.exitLane + 1 != entry.lanes)))
                        continue;
                }
                unsigned candidateLane = (std::min)(car.exitLane, edge.lanes - 1);
                Curve trial;
                if (!trial.Build(entry, car.exitLane, edge, candidateLane))
                    continue;
                // As in PickNextNodeRandomly, prefer continuing along the road;
                // choose once per transition, never reroll a blocked turn.
                float weight = dot > .85f ? 5.0f : 1.0f;
                total += weight;
                if (static_cast<float>(Random(car.random) % 65536) / 65536.0f < weight / total)
                {
                    next = edge;
                    lane = candidateLane;
                    curve = trial;
                    chosen = true;
                }
            }
            // Like re3, relax the lane restriction if the road has no legal
            // continuation (for example, every lane follows a sharp bend).
            if constexpr (AvoidCongestion)
                if (!chosen && respectLanes) return Choose(car, entry, next, lane, curve, false);
            return chosen;
        }
        static bool AdvanceRoute(Vehicle& car)
        {
            Edge next;
            unsigned nextLane;
            Curve curve;
            if (!Choose(car, car.exit, next, nextLane, curve))
                return false;
            car.entry = car.exit;
            car.lane = car.exitLane;
            car.exit = next;
            car.exitLane = nextLane;
            car.curve = curve;
            return true;
        }
        bool Spawn(Vehicle& car, const CVector& camera, float farClip)
        {
            for (int attempt = 0; attempt < 24; ++attempt)
            {
                Node node = Graph::RandomNode(Random(car.random));
                if (node == InvalidNode)
                    continue;
                std::array<Edge, 16> edges;
                size_t count = Graph::Outgoing(node, edges);
                if (!count)
                    continue;
                Edge entry = edges[Random(car.random) % count];
                if (entry.spawnRate < 15 && (Random(car.random) & 15u) > entry.spawnRate)
                    continue;
                if (!Graph::SpawnAllowed(entry))
                    continue;
                car.exitLane = Random(car.random) % entry.lanes;
                Edge exit;
                unsigned exitLane;
                Curve curve;
                if (!Choose(car, entry, exit, exitLane, curve))
                    continue;
                float distance = curve.length * (static_cast<float>(Random(car.random) % 65536) / 65536.0f);
                CVector position = curve.Point(curve.Parameter(distance));
                float d2 = (position - camera).MagnitudeSqr2D();
                if (d2 < 260.0f * 260.0f || d2 > farClip * farClip)
                    continue;
                bool occupied = false;
                float spawnGap = entry.water ? 90.0f : (std::max)(28.0f, entry.speed * 2.0f);
                unsigned nearby = 0;
                float radius = (std::max)(80.0f, spawnGap);
                int minX = static_cast<int>(std::floor((position.x - radius) / 40.0f));
                int maxX = static_cast<int>(std::floor((position.x + radius) / 40.0f));
                int minY = static_cast<int>(std::floor((position.y - radius) / 40.0f));
                int maxY = static_cast<int>(std::floor((position.y + radius) / 40.0f));
                for (int x = minX; x <= maxX && !occupied; ++x)
                    for (int y = minY; y <= maxY && !occupied; ++y)
                    {
                        int64_t key = Cell(x, y);
                        for (size_t i = cells[Bucket(key)]; i != NoCar; i = nextCell[i])
                        {
                            if (cellKeys[i] != key)
                                continue;
                            const auto& other = cars[i];
                            CVector delta = other.m_vecPos - (position + CVector(0, 0, .55f));
                            if (std::abs(delta.z) >= 4.0f)
                                continue;
                            float spacing = delta.MagnitudeSqr2D();
                            if (spacing < 80.0f * 80.0f)
                                ++nearby;
                            if constexpr (AvoidCongestion)
                            {
                                // Do not refill a queue as its tail creeps forward.
                                // Reuse the spawn query; no additional world scan.
                                if (!entry.water && spacing < 80.0f * 80.0f && other.waiting > .75f)
                                {
                                    occupied = true;
                                    break;
                                }
                            }
                            if (spacing < spawnGap * spawnGap || (!entry.water && nearby >= 8))
                            {
                                occupied = true;
                                break;
                            }
                        }
                    }
                if (occupied)
                    continue;
                car.entry = entry;
                car.exit = exit;
                car.lane = car.exitLane;
                car.exitLane = exitLane;
                car.curve = curve;
                car.distance = distance;
                car.m_bWaterNode = entry.water;
                car.cruise = (entry.water ? 8.0f : entry.speed) * (.85f + static_cast<float>(Random(car.random) % 300) / 1000.0f);
                car.speed = car.cruise;
                car.waiting = 0;
                car.retiring = false;
                car.m_visual = {};
                car.m_visual.fade = 0;
                // Stable identity even if the pool slot eventually gets reused.
                car.m_visual.paint = static_cast<uint8_t>((car.m_nCoronaId * 2654435761u) % 10);
                car.m_visual.appearanceSet = true;
                car.m_bActive = true;
                Position(car);
                car.previousPosition = car.m_vecPos;
                car.previousDirection = car.m_vecDir;
                return true;
            }
            return false;
        }
        static bool Overlap(const CVector& a, const CVector& directionA, const CVector& b, const CVector& directionB)
        {
            CVector delta = b - a;
            if (std::abs(delta.z) > 3.0f || delta.MagnitudeSqr2D() > 40.0f) return false;
            CVector rightA(directionA.y, -directionA.x, 0), rightB(directionB.y, -directionB.x, 0);
            // The 1.2x sedan is 5.57m long and about 2.2m wide. Include a small
            // clearance, and test all four separating axes, not just centre lines.
            const CVector axes[] = { directionA, rightA, directionB, rightB };
            for (const auto& axis : axes)
            {
                float separation = std::abs(delta.x * axis.x + delta.y * axis.y);
                auto radius = [&](const CVector& forward, const CVector& right)
                {
                    return 2.9f * std::abs(forward.x * axis.x + forward.y * axis.y) +
                        1.2f * std::abs(right.x * axis.x + right.y * axis.y);
                };
                if (separation >= radius(directionA, rightA) + radius(directionB, rightB)) return false;
            }
            return true;
        }
        void PreventOverlaps()
        {
            nextPositions.resize(cars.size());
            nextDirections.resize(cars.size());
            for (size_t i = 0; i < cars.size(); ++i)
            {
                const auto& car = cars[i];
                if (!car.m_bActive || car.m_bWaterNode) continue;
                float distance = car.distance + advances[i];
                if (distance > car.curve.length)
                {
                    // Predict the same route transition/RNG choice without changing
                    // the driver. Merely clamping to the link would miss collisions
                    // in the overshoot applied by the movement pass below.
                    Vehicle projected = car;
                    projected.distance = distance;
                    for (int transitions = 0; projected.distance >= projected.curve.length && transitions < 8; ++transitions)
                    {
                        float remaining = projected.distance - projected.curve.length;
                        projected.distance = projected.curve.length;
                        if (!AdvanceRoute(projected)) break;
                        projected.distance = remaining;
                    }
                    Position(projected);
                    nextPositions[i] = projected.m_vecPos;
                    nextDirections[i] = projected.m_vecDir;
                }
                else
                {
                    float t = car.curve.Parameter(distance);
                    nextPositions[i] = car.curve.Point(t) + CVector(0, 0, .55f);
                    nextDirections[i] = car.curve.Direction(t);
                }
            }
            if constexpr (AvoidCongestion)
            {
                // Resolve against simultaneous end positions. Testing every
                // moving car against its neighbour's OLD position can freeze
                // a whole queue when the leader's tail swings around a bend.
                // Recheck after a stop so followers cannot move into that car.
                for (int pass = 0; pass < 8; ++pass)
                {
                    bool changed = false;
                    auto stop = [&](size_t i)
                    {
                        advances[i] = speeds[i] = 0.0f;
                        nextPositions[i] = cars[i].m_vecPos;
                        nextDirections[i] = cars[i].m_vecDir;
                        changed = true;
                    };
                    for (size_t i = 0; i < cars.size(); ++i)
                    {
                        const auto& car = cars[i];
                        if (!car.m_bActive || car.m_bWaterNode || advances[i] == 0) continue;
                        int x = static_cast<int>(std::floor(car.m_vecPos.x / 40.0f));
                        int y = static_cast<int>(std::floor(car.m_vecPos.y / 40.0f));
                        for (int dx = -1; dx <= 1 && advances[i] > 0; ++dx)
                            for (int dy = -1; dy <= 1 && advances[i] > 0; ++dy)
                            {
                                int64_t key = Cell(x + dx, y + dy);
                                for (size_t j = cells[Bucket(key)]; j != NoCar && advances[i] > 0; j = nextCell[j])
                                {
                                    const auto& other = cars[j];
                                    if (i == j || cellKeys[j] != key || other.m_bWaterNode ||
                                        (other.m_vecPos - car.m_vecPos).MagnitudeSqr2D() > 144.0f ||
                                        Overlap(car.m_vecPos, car.m_vecDir, other.m_vecPos, other.m_vecDir)) continue;
                                    if (!Overlap(nextPositions[i], nextDirections[i], nextPositions[j], nextDirections[j])) continue;
                                    if (advances[j] == 0 || Overlap(nextPositions[i], nextDirections[i], other.m_vecPos, other.m_vecDir))
                                        stop(i);
                                    else
                                        stop(j);
                                }
                            }
                    }
                    if (!changed) return;
                }
                // Unusually long stop chains use the conservative fallback below.
            }
            for (size_t i = 0; i < cars.size(); ++i)
            {
                const auto& car = cars[i];
                if (!car.m_bActive || car.m_bWaterNode || advances[i] == 0) continue;
                int x = static_cast<int>(std::floor(car.m_vecPos.x / 40.0f));
                int y = static_cast<int>(std::floor(car.m_vecPos.y / 40.0f));
                bool blocked = false;
                for (int dx = -1; dx <= 1 && !blocked; ++dx)
                    for (int dy = -1; dy <= 1 && !blocked; ++dy)
                    {
                        int64_t key = Cell(x + dx, y + dy);
                        for (size_t j = cells[Bucket(key)]; j != NoCar; j = nextCell[j])
                        {
                            const auto& other = cars[j];
                            if (i == j || cellKeys[j] != key || !other.m_bActive || other.m_bWaterNode ||
                                (other.m_vecPos - car.m_vecPos).MagnitudeSqr2D() > 144.0f) continue;
                            // Let pre-existing overlaps separate instead of freezing
                            // both cars. Spawn spacing prevents new overlaps at birth.
                            if (Overlap(car.m_vecPos, car.m_vecDir, other.m_vecPos, other.m_vecDir)) continue;
                            // Check both ends of the other driver's move. Therefore
                            // either car can yield without invalidating this result.
                            if (Overlap(nextPositions[i], nextDirections[i], other.m_vecPos, other.m_vecDir) ||
                                Overlap(nextPositions[i], nextDirections[i], nextPositions[j], nextDirections[j]))
                            {
                                blocked = true;
                                break;
                            }
                        }
                    }
                if (blocked) advances[i] = speeds[i] = 0.0f;
            }
        }
        void Step(float dt)
        {
            BuildCells();
            advances.assign(cars.size(), 0);
            speeds.assign(cars.size(), 0);
            if constexpr (AvoidCongestion)
            {
                yieldPositions.resize(cars.size());
                yieldDirections.resize(cars.size());
                yieldCount.assign(cars.size(), 6);
            }
            else
            {
                lookPositions.resize(cars.size());
                lookDirections.resize(cars.size());
            }
            for (size_t i = 0; i < cars.size(); ++i)
            {
                auto& car = cars[i];
                if (!car.m_bActive)
                    continue;
                car.previousPosition = car.m_vecPos;
                car.previousDirection = car.m_vecDir;
                if (!car.m_bWaterNode)
                {
                    if constexpr (AvoidCongestion)
                    {
                        Vehicle projected = car;
                        for (size_t n = 0; n < yieldPositions[i].size(); ++n)
                        {
                            if (n) projected.distance += 5.0f;
                            // III has short internal road links. Continue across
                            // them using the same route/RNG choices as movement;
                            // clamping here hides the next junction until too late.
                            for (int transitions = 0; projected.distance >= projected.curve.length && transitions < 8; ++transitions)
                            {
                                float remaining = projected.distance - projected.curve.length;
                                projected.distance = projected.curve.length;
                                if (!AdvanceRoute(projected)) break;
                                projected.distance = remaining;
                            }
                            float ahead = projected.curve.Parameter(projected.distance);
                            yieldPositions[i][n] = projected.curve.Point(ahead) + CVector(0, 0, .55f);
                            yieldDirections[i][n] = projected.curve.Direction(ahead);
                        }
                    }
                    else
                    {
                        float t = car.curve.Parameter((std::min)(car.distance + car.speed * 1.5f, car.curve.length));
                        lookPositions[i] = car.curve.Point(t) + CVector(0, 0, .55f);
                        lookDirections[i] = car.curve.Direction(t);
                    }
                }
            }
            if constexpr (AvoidCongestion)
                for (size_t i = 0; i < cars.size(); ++i)
                {
                    const auto& car = cars[i];
                    if (!car.m_bActive || car.m_bWaterNode) continue;
                    int x = static_cast<int>(std::floor(car.m_vecPos.x / 40.0f));
                    int y = static_cast<int>(std::floor(car.m_vecPos.y / 40.0f));
                    for (int dx = -1; dx <= 1; ++dx)
                        for (int dy = -1; dy <= 1; ++dy)
                        {
                            int64_t key = Cell(x + dx, y + dy);
                            for (size_t j = cells[Bucket(key)]; j != NoCar; j = nextCell[j])
                            {
                                const auto& other = cars[j];
                                if (i == j || cellKeys[j] != key || other.m_bWaterNode ||
                                    DotProduct(car.m_vecDir, other.m_vecDir) < .5f) continue;
                                bool sameEntry = car.entry.from == other.entry.from && car.entry.to == other.entry.to && car.lane == other.lane;
                                bool nextLink = car.exit.from == other.entry.from && car.exit.to == other.entry.to && car.exitLane == other.lane;
                                bool previousLink = car.entry.from == other.exit.from && car.entry.to == other.exit.to && car.lane == other.exitLane;
                                if (!sameEntry && !nextLink && !previousLink) continue;
                                // A queued driver cannot reserve the crossing beyond
                                // the car in front. Otherwise the cross traffic yields
                                // to the tail while the head yields to cross traffic.
                                for (size_t n = 1; n < yieldCount[i]; ++n)
                                    if (Overlap(yieldPositions[i][n], yieldDirections[i][n], other.m_vecPos, other.m_vecDir))
                                    {
                                        yieldCount[i] = n;
                                        break;
                                    }
                            }
                        }
                }
            for (size_t i = 0; i < cars.size(); ++i)
            {
                auto& car = cars[i];
                if (!car.m_bActive)
                    continue;
                if (car.retiring)
                {
                    car.m_visual.fade = (std::max)(0.0f, car.m_visual.fade - dt / (AvoidCongestion ? .6f : .8f));
                    if (car.m_visual.fade == 0)
                    {
                        car.m_bActive = false;
                        Graph::Hide(car.m_nCoronaId);
                    }
                    continue;
                }
                car.m_visual.fade = (std::min)(1.0f, car.m_visual.fade + dt / 1.2f);
                if (!Graph::Valid(car.entry) || !Graph::Valid(car.exit))
                {
                    car.retiring = true;
                    continue;
                }
                float desired = (std::min)(car.cruise, (std::min)(car.entry.speed, car.exit.speed));
                float bend = DotProduct(car.entry.direction, car.exit.direction);
                if (bend < .8f && car.distance > car.curve.leadLength - 25.0f && car.distance < car.curve.length - car.curve.trailLength + 25.0f)
                    desired = (std::min)(desired, 8.0f);
                float allowed = 1000.0f;
                unsigned queuedAhead = 0;
                // Read one snapshot and apply all movement afterwards. Followers
                // slow BEFORE moving; they are never pushed backwards to make room.
                int x = static_cast<int>(std::floor(car.m_vecPos.x / 40.0f));
                int y = static_cast<int>(std::floor(car.m_vecPos.y / 40.0f));
                for (int dx = -1; dx <= 1; ++dx)
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        int64_t key = Cell(x + dx, y + dy);
                        for (size_t j = cells[Bucket(key)]; j != NoCar; j = nextCell[j])
                        {
                            if (cellKeys[j] != key || i == j)
                                continue;
                            const auto& other = cars[j];
                            CVector delta = other.m_vecPos - car.m_vecPos;
                            if (std::abs(delta.z) > 3.0f || car.m_bWaterNode != other.m_bWaterNode)
                                continue;
                            auto sameLane = [](const Edge& a, unsigned laneA, const Edge& b, unsigned laneB)
                            { return a.from == b.from && a.to == b.to && laneA == laneB; };
                            bool sameEntry = sameLane(car.entry, car.lane, other.entry, other.lane);
                            bool sameExit = sameLane(car.exit, car.exitLane, other.exit, other.exitLane);
                            bool following = sameEntry || sameExit || sameLane(car.entry, car.lane, other.exit, other.exitLane) ||
                                             sameLane(car.exit, car.exitLane, other.entry, other.lane);
                            // At a merge both approaching cars can project ahead
                            // of each other. Give one approach priority instead of
                            // letting two followers brake each other to a standstill.
                            if (sameExit && !sameEntry && DotProduct(delta, car.m_vecDir) > 0 && DotProduct(delta, other.m_vecDir) < 0)
                            {
                                bool otherFirst = other.entry.from < car.entry.from ||
                                    (other.entry.from == car.entry.from && other.m_nCoronaId < car.m_nCoronaId);
                                if constexpr (AvoidCongestion)
                                    otherFirst = other.curve.length - other.distance < car.curve.length - car.distance ||
                                        (other.curve.length - other.distance == car.curve.length - car.distance && other.m_nCoronaId < car.m_nCoronaId);
                                following = otherFirst;
                            }
                            float heading = DotProduct(car.m_vecDir, other.m_vecDir);
                            if constexpr (AvoidCongestion)
                            {
                                float along = DotProduct(delta, car.m_vecDir);
                                float lateral = std::abs(delta.x * car.m_vecDir.y - delta.y * car.m_vecDir.x);
                                if (!other.retiring && heading > .5f && along > 0 && along < 40.0f && lateral < 3.0f &&
                                    other.speed < (std::max)(1.0f, other.cruise * .25f))
                                    ++queuedAhead;
                            }
                            if (!AvoidCongestion && !(sameEntry && sameExit) && !car.m_bWaterNode && delta.MagnitudeSqr2D() < 400.0f &&
                                other.m_nCoronaId < car.m_nCoronaId &&
                                Overlap(lookPositions[i], lookDirections[i], lookPositions[j], lookDirections[j]))
                                desired = 0.0f;
                            if (following)
                            {
                                float along = sameEntry && sameExit ? other.distance - car.distance : DotProduct(delta, car.m_vecDir);
                                float lateral = sameEntry && sameExit ? 0.0f : std::abs(delta.x * car.m_vecDir.y - delta.y * car.m_vecDir.x);
                                float gap = car.m_bWaterNode ? 35.0f : 6.0f;
                                if (along > 0 && along <= 70 && lateral <= (car.m_bWaterNode ? 5.0f : sameExit && !sameEntry ? 6.0f : 1.8f) && ((sameEntry && sameExit) || heading > .7f))
                                {
                                    float space = (std::max)(0.0f, along - gap);
                                    allowed = (std::min)(allowed, space);
                                    float headway = AvoidCongestion ? .55f : .8f;
                                    float response = AvoidCongestion ? .8f : .5f;
                                    float followingSpeed = (std::max)(0.0f, other.speed + (space - car.speed * headway) * response);
                                    desired = (std::min)(desired, followingSpeed);
                                }
                            }
                            // Local crossing priority, like the games' nearby-car
                            // scans. Never lock an entire node/link, and never stop
                            // a vehicle that is already clearing the crossing.
                            if ((!AvoidCongestion && (car.entry.to != other.entry.to || heading > .7f)) || car.entry.from == other.entry.from)
                                continue;
                            if constexpr (AvoidCongestion)
                            {
                                // Opposing turns can intersect even when the current
                                // headings are parallel. Check the upcoming corridor,
                                // not just a single speed-dependent future position.
                                // Fixed distances keep the yielding car stopped until
                                // the other driver actually clears its path.
                                if (sameEntry || sameLane(car.entry, car.lane, other.exit, other.exitLane) ||
                                    sameLane(car.exit, car.exitLane, other.entry, other.lane))
                                    continue; // A car clearing the link never yields to its own queue.
                                if (!car.m_bWaterNode && delta.MagnitudeSqr2D() < 3600.0f)
                                {
                                    size_t first = 6, otherFirst = 6;
                                    for (size_t a = 0; a < yieldCount[i]; ++a)
                                        for (size_t b = 0; b < yieldCount[j]; ++b)
                                            if (Overlap(yieldPositions[i][a], yieldDirections[i][a], yieldPositions[j][b], yieldDirections[j][b]))
                                            {
                                                first = (std::min)(first, a);
                                                otherFirst = (std::min)(otherFirst, b);
                                            }
                                    // The driver nearest the actual conflict clears
                                    // first. Node IDs are not arrival order, especially
                                    // when a curve crosses several short road links.
                                    bool yield = first > otherFirst || (first == otherFirst && other.m_nCoronaId < car.m_nCoronaId);
                                    if (sameExit)
                                        yield = other.curve.length - other.distance < car.curve.length - car.distance ||
                                            (other.curve.length - other.distance == car.curve.length - car.distance && other.m_nCoronaId < car.m_nCoronaId);
                                    if (first < 6 && yield)
                                    {
                                        allowed = (std::min)(allowed, (std::max)(0.0f, (static_cast<float>(first) - 1.0f) * 5.0f));
                                        desired = 0.0f;
                                    }
                                }
                                continue;
                            }
                            float cross = car.m_vecDir.x * other.m_vecDir.y - car.m_vecDir.y * other.m_vecDir.x;
                            if (std::abs(cross) < .25f || (other.speed < .5f && other.waiting > 3.0f))
                                continue;
                            float approach = (delta.x * other.m_vecDir.y - delta.y * other.m_vecDir.x) / cross;
                            float otherApproach = (delta.x * car.m_vecDir.y - delta.y * car.m_vecDir.x) / cross;
                            if (approach < 6.0f || approach > 30.0f || otherApproach < -4.0f || otherApproach > 30.0f)
                                continue;
                            float arrival = approach / (std::max)(car.speed, 2.0f);
                            float otherArrival = (std::max)(0.0f, otherApproach) / (std::max)(other.speed, 2.0f);
                            bool occupied = otherApproach < 6.0f;
                            bool priority = std::abs(arrival - otherArrival) < 1.5f &&
                                            (otherArrival < arrival - .75f || (std::abs(arrival - otherArrival) <= .75f && other.entry.from < car.entry.from));
                            if (occupied || priority)
                            {
                                float space = approach - 6.0f;
                                allowed = (std::min)(allowed, space);
                                desired = (std::min)(desired, std::sqrt(6.0f * space));
                            }
                        }
                    }
                if constexpr (AvoidCongestion)
                    if (queuedAhead >= 3 && car.waiting > .75f)
                    {
                        // Ambient traffic must not fill a street with a standing
                        // queue. Keep its front cars and fade excess tails using
                        // the existing envelope; never teleport through the jam.
                        car.retiring = true;
                        continue;
                    }
                // III's queue should pull away promptly once its leader moves.
                // Keep the same braking and physical clearance checks.
                float acceleration = AvoidCongestion ? 3.5f : 2.0f;
                float speed = car.speed + (std::clamp)(desired - car.speed, -4.0f * dt, acceleration * dt);
                float advance = (std::min)((std::max)(0.0f, speed) * dt, allowed);
                speeds[i] = advance / dt;
                advances[i] = advance;
            }
            PreventOverlaps();
            for (size_t i = 0; i < cars.size(); ++i)
            {
                auto& car = cars[i];
                if (!car.m_bActive || car.retiring)
                    continue;
                car.speed = speeds[i];
                if constexpr (AvoidCongestion)
                {
                    // A creeping queue is still congested. Short bursts of
                    // movement must not restart its recovery timer every time.
                    car.waiting = car.speed < car.cruise * .25f && car.cruise > 1.0f ?
                        car.waiting + dt : (std::max)(0.0f, car.waiting - dt * 2.0f);
                }
                else
                    car.waiting = car.speed < .5f && car.cruise > 1.0f ? car.waiting + dt : 0.0f;
                // Recycle a genuinely stuck distant driver gradually. Stagger the
                // timeout so a whole queue does not disappear on the same frame.
                float stuckTime = AvoidCongestion ? 8.0f + static_cast<float>(car.m_nCoronaId % 4) :
                    24.0f + static_cast<float>(car.m_nCoronaId % 12);
                if (car.waiting > stuckTime)
                {
                    car.retiring = true;
                    continue;
                }
                car.distance += advances[i];
                // Preserve METRES of overshoot across unequal-length curves.
                for (int transitions = 0; car.distance >= car.curve.length && transitions < 8; ++transitions)
                {
                    float remaining = car.distance - car.curve.length;
                    car.distance = car.curve.length;
                    Position(car);
                    if (!AdvanceRoute(car))
                    {
                        car.retiring = true;
                        break;
                    }
                    car.distance = remaining;
                }
                Position(car);
            }
        }

      public:
        std::vector<Vehicle> cars;
        CVector RenderPosition(const Vehicle& car) const
        {
            float t = (std::clamp)(accumulator * 30.0f, 0.0f, 1.0f);
            return car.previousPosition + (car.m_vecPos - car.previousPosition) * t;
        }
        CVector RenderDirection(const Vehicle& car) const
        {
            float t = (std::clamp)(accumulator * 30.0f, 0.0f, 1.0f);
            CVector direction = car.previousDirection + (car.m_vecDir - car.previousDirection) * t;
            direction.Normalise();
            return direction;
        }
        void Reserve(size_t capacity)
        {
            capacity = (std::min)(capacity, size_t(10000));
            cars.reserve(capacity);
            advances.reserve(capacity);
            speeds.reserve(capacity);
            nextPositions.reserve(capacity);
            nextDirections.reserve(capacity);
            if constexpr (AvoidCongestion)
            {
                yieldPositions.reserve(capacity);
                yieldDirections.reserve(capacity);
                yieldCount.reserve(capacity);
            }
            else
            {
                lookPositions.reserve(capacity);
                lookDirections.reserve(capacity);
            }
            nextCell.reserve(capacity);
            cellKeys.reserve(capacity);
            size_t buckets = 1;
            while (buckets < capacity * 2)
                buckets *= 2;
            cells.reserve(buckets);
        }
        void Clear()
        {
            for (auto& car : cars)
                if (car.m_bActive)
                    Graph::Hide(car.m_nCoronaId);
            cars.clear();
            cells.clear();
            accumulator = 0;
            spawnCursor = 0;
        }
        void Update(float dt, size_t capacity, float density, const CVector& camera, float farClip)
        {
            capacity = (std::min)(capacity, size_t(10000));
            if (capacity < cars.size())
                for (size_t i = capacity; i < cars.size(); ++i)
                    Graph::Hide(cars[i].m_nCoronaId);
            size_t old = cars.size();
            cars.resize(capacity);
            for (size_t i = old; i < cars.size(); ++i)
            {
                cars[i].m_nCoronaId = 0x7F000000u + static_cast<uint32_t>(i);
                cars[i].random = 0x9E3779B9u ^ ((static_cast<uint32_t>(i) + 1) * 747796405u);
                if (!cars[i].random)
                    cars[i].random = 1;
            }
            if (cars.empty())
                return;
            if (!std::isfinite(dt) || dt <= 0)
                return;
            size_t active = 0;
            for (const auto& car : cars)
                if (car.m_bActive && !car.retiring)
                    ++active;
            size_t desired = static_cast<size_t>(cars.size() * (std::clamp)(density, 0.0f, 1.0f));
            for (auto& car : cars)
            {
                if (!car.m_bActive || car.retiring)
                    continue;
                float dist = (car.m_vecPos - camera).MagnitudeSqr2D();
                // Density changes only retire hidden traffic; visible queues keep
                // their identities. Off-map cars eventually free slots for new areas.
                if (dist > (farClip + 300) * (farClip + 300) || (active > desired && (dist < 120 * 120 || dist > farClip * farClip)))
                {
                    car.retiring = true;
                    --active;
                }
            }
            if (active < desired)
                BuildCells();
            size_t budget = 12;
            for (size_t visited = 0; visited < cars.size() && budget && active < desired; ++visited)
            {
                auto& car = cars[spawnCursor++ % cars.size()];
                if (car.m_bActive)
                    continue;
                --budget;
                if (Spawn(car, camera, farClip))
                {
                    InsertCell(static_cast<size_t>(&car - cars.data()));
                    ++active;
                }
            }
            accumulator += (std::min)(dt, .25f);
            constexpr float step = 1.0f / 30.0f;
            while (accumulator >= step)
            {
                Step(step);
                accumulator -= step;
            }
        }
    };
}
