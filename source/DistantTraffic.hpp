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
    template <class Graph, class Visual> class Simulation
    {
        using Vehicle = Car<Visual>;
        float accumulator = 0;
        std::vector<float> advances, speeds;
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
        static bool Choose(Vehicle& car, const Edge& entry, Edge& next, unsigned& lane, Curve& curve)
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
        void Step(float dt)
        {
            BuildCells();
            advances.assign(cars.size(), 0);
            speeds.assign(cars.size(), 0);
            for (size_t i = 0; i < cars.size(); ++i)
            {
                auto& car = cars[i];
                if (!car.m_bActive)
                    continue;
                car.previousPosition = car.m_vecPos;
                car.previousDirection = car.m_vecDir;
            }
            for (size_t i = 0; i < cars.size(); ++i)
            {
                auto& car = cars[i];
                if (!car.m_bActive)
                    continue;
                if (car.retiring)
                {
                    car.m_visual.fade = (std::max)(0.0f, car.m_visual.fade - dt / .8f);
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
                                following = otherFirst;
                            }
                            float heading = DotProduct(car.m_vecDir, other.m_vecDir);
                            if (following)
                            {
                                float along = sameEntry && sameExit ? other.distance - car.distance : DotProduct(delta, car.m_vecDir);
                                float lateral = sameEntry && sameExit ? 0.0f : std::abs(delta.x * car.m_vecDir.y - delta.y * car.m_vecDir.x);
                                float gap = car.m_bWaterNode ? 35.0f : 6.0f;
                                if (along > 0 && along <= 70 && lateral <= (car.m_bWaterNode ? 5.0f : 1.8f) && ((sameEntry && sameExit) || heading > .7f))
                                {
                                    float space = (std::max)(0.0f, along - gap);
                                    allowed = (std::min)(allowed, space);
                                    desired = (std::min)(desired, (std::max)(0.0f, other.speed + (space - car.speed * .8f) * .5f));
                                }
                            }
                            // Local crossing priority, like the games' nearby-car
                            // scans. Never lock an entire node/link, and never stop
                            // a vehicle that is already clearing the crossing.
                            if (car.entry.to != other.entry.to || car.entry.from == other.entry.from || heading > .7f)
                                continue;
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
                float speed = car.speed + (std::clamp)(desired - car.speed, -4.0f * dt, 2.0f * dt);
                float advance = (std::min)((std::max)(0.0f, speed) * dt, allowed);
                speeds[i] = advance / dt;
                advances[i] = advance;
            }
            for (size_t i = 0; i < cars.size(); ++i)
            {
                auto& car = cars[i];
                if (!car.m_bActive || car.retiring)
                    continue;
                car.speed = speeds[i];
                car.waiting = car.speed < .5f && car.cruise > 1.0f ? car.waiting + dt : 0.0f;
                // Recycle a genuinely stuck distant driver gradually. Stagger the
                // timeout so a whole queue does not disappear on the same frame.
                if (car.waiting > 24.0f + static_cast<float>(car.m_nCoronaId % 12))
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
