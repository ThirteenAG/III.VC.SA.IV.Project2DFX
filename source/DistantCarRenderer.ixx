module;

#include <stdafx.h>
#include <cmath>
#include <type_traits>
#include "DistantCarMesh.hpp"
#include "DistantBoatMesh.hpp"

export module DistantCarRenderer;

import Game;
import Misc;
import ComVars;
import Clock;
import Timer;
import WaterLevel;
import LODLights;
import Sprite;

export namespace DistantCarRenderer
{
    constexpr float CarScale = 1.2f;

    // GTA III uses the older immediate-mode pipeline. Keep its submissions small
    // and include UVs even with a null texture raster to use the full vertex path.
    bool bLegacyIm3D = false;
    float fNightAmbient = .14f;
    // Per simulated car; reset on respawn, not on each path segment.
    struct State
    {
        bool sampled = false;
        bool grounded = false;
        uint32_t sampleTime = 0;
        CVector samplePosition = { 0,0,0 };
        CVector groundNormal = { 0,0,1 };
        float groundHeight = 0;
        // Appearance and the spawn/despawn envelope. The paint is rolled once
        // per spawned car and kept for its whole life, so a car driving along
        // the road never changes colour; only a respawn picks a new one.
        bool appearanceSet = false;
        uint8_t paint = 0;
        float fade = 1.0f;
        bool dying = false;
    };

    // Spawn/despawn envelope. Traffic must never appear or disappear in one
    // frame: a slot that has to be reused fades out first (and fades back in at
    // its new place), instead of teleporting. Returns true only when the caller
    // may move the car now, i.e. when a requested fade-out has finished.
    // 'dt' is the game frame time in seconds.
    inline bool UpdateEnvelope(State& state, float dt, bool wantRecycle)
    {
        constexpr float fadeInSeconds = 1.2f;
        constexpr float fadeOutSeconds = 0.6f;

        if (state.dying)
        {
            state.fade -= dt / fadeOutSeconds;
            if (state.fade > 0.0f)
                return false;

            state.fade = 0.0f;
            state.dying = false;
            return true;
        }

        if (wantRecycle)
        {
            // Faded out already (out of sight respawn); the caller may move it now.
            if (state.fade <= 0.0f)
                return true;

            state.dying = true;
            return false;
        }

        if (state.fade < 1.0f)
            state.fade = (std::min)(1.0f, state.fade + dt / fadeInSeconds);

        return false;
    }

    void HideLights(uint32_t id)
    {
        for (uint32_t lamp = 0; lamp < 4; ++lamp)
            CLODLights::UnregisterCorona(0x7C000000u + (id - 0x7F000000u) * 4 + lamp);
    }

    class Frame
    {
        struct Car
        {
            CVector base, direction;
            State* state;
            uint32_t id;
            float fade, distance;
            bool boat;
        };
        CVector camera;
        float farClip;
        bool enabled;
        static constexpr float PROBE_MAX_DISTANCE = 600.0f;
        // Shared by successive frames on the game's render thread. The traffic
        // pool is capped at 10000; no per-frame heap allocations are necessary.
        static inline DistantCarMesh::Buffer<Car, 10000> cars;
        static inline DistantCarMesh::Buffer<uint32_t, 10000> previousLights;

        template<class Query>
        static bool SampleWaterLevel(Query query, const CVector& position, float* level)
        {
            if (!query) return false;
            if constexpr (std::is_invocable_v<Query, float, float, float, float*>)
                return query(position.x, position.y, position.z, level);
            else
                return query(position.x, position.y, position.z, level, nullptr, nullptr);
        }

        static float FogVisibility(const CVector& position)
        {
            const auto* rwCamera = Scene->m_pRwCamera;
            if (!rwCamera || rwCamera->farPlane <= rwCamera->fogPlane) return 1.0f;
            const auto& view = rwCamera->viewMatrix;
            float depth = position.x * view.right.z + position.y * view.up.z + position.z * view.at.z + view.pos.z;
            // Match the linear camera fog applied to the mesh. Additive sprites
            // otherwise remain visible after the body has blended into the fog.
            return Saturate((rwCamera->farPlane - depth) / (rwCamera->farPlane - rwCamera->fogPlane));
        }

        static float Saturate(float v) { return (std::clamp)(v, 0.0f, 1.0f); }

        static void Probe(Car& car, uint32_t now)
        {
            auto& state = *car.state;
            state.sampled = true;
            state.sampleTime = now;
            state.samplePosition = car.base;
            state.grounded = false;
            if (car.boat) return;
            if (!CWorld::FindGroundZFor3DCoordCR && !CWorld::FindGroundZFor3DCoordCRGO) return;
            // Collision is only streamed around the camera; probing far away
            // either fails or returns a lower deck. Those cars keep the path's
            // own height and slope, which is what the game's traffic follows.
            if (car.distance > PROBE_MAX_DISTANCE * PROBE_MAX_DISTANCE) return;

            CVector forward = car.direction;
            forward.Normalise();
            CVector right(forward.y, -forward.x, 0); right.Normalise();
            const CVector offsets[] = { forward * (1.48f * CarScale),forward * (-1.48f * CarScale),right * (.8f * CarScale),right * (-.8f * CarScale) };
            float heights[4];
            for (int i = 0; i < 4; ++i)
            {
                CVector p = car.base + offsets[i];
                bool found = false;
                // A short allowance above the expected road avoids selecting a
                // bridge/roof over this path. Never accept the sea/floor far below.
                heights[i] = CWorld::FindGroundZFor3DCoord(p.x, p.y, p.z + 2.0f, &found, nullptr);
                if (!found || !std::isfinite(heights[i]) || std::abs(heights[i] - p.z) > 2.0f) return;
            }
            CVector f = offsets[0] - offsets[1]; f.z = heights[0] - heights[1];
            CVector r = offsets[2] - offsets[3]; r.z = heights[2] - heights[3];
            CVector normal = CrossProduct(r, f); normal.Normalise();
            if (normal.z < .7f) return;
            state.grounded = true;
            state.groundNormal = normal;
            state.groundHeight = (heights[0] + heights[1] + heights[2] + heights[3]) * .25f;
        }

        // Restore exactly the state we touched, including texture and fog. The
        // moving-things hook is shared with coronas/searchlights and other mods.
        struct RenderStates
        {
            const RwRenderState states[10] = { rwRENDERSTATETEXTURERASTER,rwRENDERSTATEZTESTENABLE,
                rwRENDERSTATEZWRITEENABLE,rwRENDERSTATEVERTEXALPHAENABLE,
                rwRENDERSTATESRCBLEND,rwRENDERSTATEDESTBLEND,rwRENDERSTATECULLMODE,
                rwRENDERSTATEFOGENABLE,rwRENDERSTATESHADEMODE,rwRENDERSTATEALPHATESTFUNCTION };
            uintptr_t saved[10] = {};

            RenderStates()
            {
                // ZWRITE stays off on purpose. Everything that draws after the
                // models and z-tests will be rejected inside their pixels if the
                // models write depth, so cars would cut holes into the moving fog
                // and the volumetric clouds (both only test, they never occlude
                // anything themselves). The test stays on, so buildings occlude
                // the cars, and the cars are sorted back to front, which is what
                // the alpha blending needs.
                const uintptr_t values[10] = { 0,1,0,1,5,6,2,1,2,8 };
                for (int i = 0; i < 10; ++i)
                {
                    RwRenderStateGet(states[i], &saved[i]);
                    RwRenderStateSet(states[i], reinterpret_cast<void*>(values[i]));
                }
            }
            ~RenderStates()
            {
                for (int i = 9; i >= 0; --i)
                    RwRenderStateSet(states[i], reinterpret_cast<void*>(saved[i]));
            }
        };

        void Render()
        {
            if (cars.empty()) return;
            // All three dllmains resolve PauseMode; the regular millisecond
            // GameRef is declared but unbound. Cache age can use pause time.
            const uint32_t now = CTimer::m_snTimeInMillisecondsPauseMode;
            // Round-robin sampling prevents distant entries from starving. At
            // most 128 vertical queries per frame, independent of pool size.
            static size_t probeCursor = 0;
            size_t start = probeCursor % cars.size();
            int budget = 32;
            for (size_t n = 0; n < cars.size() && budget>0; ++n)
            {
                size_t i = (start + n) % cars.size();
                auto& car = cars[i];
                auto& state = *car.state;
                if (!state.sampled || now - state.sampleTime >= 350u ||
                    (car.base - state.samplePosition).MagnitudeSqr() > 36.0f)
                {
                    Probe(car, now);
                    --budget;
                }
                probeCursor = i + 1;
            }
            std::sort(cars.begin(), cars.end(), [](const Car& a, const Car& b) { return a.distance > b.distance; });
            static const DistantCarMesh::Mesh carMesh;
            static const DistantBoatMesh::Mesh boatMesh;
            static DistantCarMesh::Buffer<RxObjSpace3dVertex, 16000> vertices;
            static DistantCarMesh::Buffer<short, 24000> indices;
            vertices.clear(); indices.clear();
            const size_t vertexLimit = bLegacyIm3D ? 2048 : 16000;
            const unsigned int transformFlags = bLegacyIm3D ? 0x19u : 0x18u;
            RenderStates states;
            static DistantCarMesh::Buffer<uint32_t, 10000> batchIds;
            batchIds.clear();
            auto flush = [&]()
            {
                bool drawn = false;
                if (!vertices.empty() && RwIm3DTransform(vertices.data(), static_cast<unsigned int>(vertices.size()), nullptr, transformFlags))
                {
                    drawn = RwIm3DRenderIndexedPrimitive(3, indices.data(), static_cast<int>(indices.size())) != 0;
                    RwIm3DEnd();
                }
                if (!drawn)
                    for (uint32_t id : batchIds) HideLights(id);
                batchIds.clear();
                vertices.clear(); indices.clear();
            };

            const float hour = static_cast<float>(CClock::ms_nGameClockHours) + static_cast<float>(CClock::ms_nGameClockMinutes) / 60.0f;
            const float daylight = Saturate((hour - 5.5f) / 1.5f) * Saturate((20.5f - hour) / 2.0f);
            const float lamps = (std::max)(1.0f - daylight, static_cast<float>(CWeather::Foggyness) * .8f);
            static constexpr uint8_t paints[][3] = { {175,181,180},{53,61,66},{134,29,25},{30,58,99},
                {189,178,143},{32,70,53},{203,202,190},{87,82,76},{109,118,134},{172,139,65} };
            for (const auto& car : cars)
            {
                CVector position = car.base;
                const DistantCarMesh::Geometry& mesh = car.boat ?
                    static_cast<const DistantCarMesh::Geometry&>(boatMesh) : carMesh;
                CVector forward = car.direction;
                if (car.boat)
                {
                    forward.z = 0;
                    float water;
                    if (SampleWaterLevel(CWaterLevel::GetWaterLevelNoWaves, position, &water) && std::isfinite(water))
                        position.z = water;
                }
                forward.Normalise();
                CVector right(forward.y, -forward.x, 0); right.Normalise();
                CVector up = CrossProduct(right, forward); up.Normalise();
                const auto& ground = *car.state;
                // Expire cached planes across teleports, unloaded collision, and
                // junctions. Path elevation/pitch is always the safe fallback.
                if (!car.boat && ground.grounded && now - ground.sampleTime < 1000u &&
                   (car.base - ground.samplePosition).MagnitudeSqr() < 144.0f)
                {
                    up = ground.groundNormal;
                    CVector delta = car.base - ground.samplePosition;
                    float height = ground.groundHeight - (up.x * delta.x + up.y * delta.y) / up.z;
                    if (std::abs(height - car.base.z) < 2.0f)
                    {
                        position.z = height;
                        forward = forward - up * DotProduct(forward, up); forward.Normalise();
                        right = CrossProduct(forward, up); right.Normalise();
                    }
                    else up = CrossProduct(right, forward);
                }
                if (!car.boat) position.z += fDistantCarsGroundOffset;
                const float scale = car.boat ? 1.0f : CarScale;
                auto world = [&](const CVector& p) { return position + (right * p.x + forward * p.y + up * p.z) * scale; };
                auto normal = [&](const CVector& n) { return right * n.x + forward * n.y + up * n.z; };
                if (vertices.size() + mesh.vertices.size() > vertexLimit || indices.size() + mesh.indices.size() > 24000) flush();
                batchIds.push_back(car.id);
                short base = static_cast<short>(vertices.size());
                // A large additive glow outlives a nearly transparent body even
                // with the same alpha. Fade its intensity and footprint together.
                float lightFade = car.fade * Saturate((car.fade - .10f) / .90f) * FogVisibility(position);
                float lightSize = std::sqrt(lightFade);
                // One paint per car, rolled on spawn and then kept: the colour of
                // a car that is already driving never changes.
                if (!car.state->appearanceSet)
                {
                    car.state->paint = static_cast<uint8_t>(CGeneral::GetRandomNumber() % std::size(paints));
                    car.state->appearanceSet = true;
                }
                const auto& paint = paints[car.state->paint % std::size(paints)];
                const float ambient = fNightAmbient * (1.0f - daylight) + .14f * daylight + daylight * .42f;
                const float diffuse = daylight * .34f;
                const uint32_t alpha = static_cast<uint32_t>(static_cast<int>(255 * car.fade)) << 24;
                // Components are bounded to [0,255]. A signed conversion avoids
                // the x86 unsigned float-conversion runtime helper per channel.
                auto rgb = [](float r, float g, float b, float light)
                {
                    return (static_cast<uint32_t>(static_cast<int>(r * light)) << 16) |
                        (static_cast<uint32_t>(static_cast<int>(g * light)) << 8) |
                        static_cast<uint32_t>(static_cast<int>(b * light));
                };
                const uint32_t nightColors[] = {
                    rgb(paint[0], paint[1], paint[2], ambient), rgb(44, 62, 73, ambient),
                    rgb(23, 24, 25, ambient), rgb(135, 139, 140, ambient),
                    rgb(245, 239, 206, (std::max)(ambient, lamps)), rgb(175, 16, 10, (std::max)(ambient, lamps))
                };
                auto output = vertices.append(mesh.vertices.size());
                auto outputIndices = indices.append(mesh.indices.size());
                if (!output || !outputIndices) { HideLights(car.id); continue; }
                for (const auto& v : mesh.vertices)
                {
                    CVector p = world(v.position), n = normal(v.normal);
                    auto& vertex = *output++;
                    vertex.objVertex = { p.x,p.y,p.z }; vertex.objNormal = { n.x,n.y,n.z };
                    vertex.u = vertex.v = 0.0f;
                    if (diffuse == 0.0f && v.material != DistantCarMesh::BoatHull)
                    {
                        vertex.color = alpha | nightColors[v.material];
                        continue;
                    }
                    float light = ambient + diffuse * (std::max)(0.0f, DotProduct(n, CVector(-.35f, -.45f, .82f)));
                    float r = paint[0], g = paint[1], b = paint[2];
                    switch (v.material)
                    {
                        case DistantCarMesh::BoatHull:
                            r = static_cast<float>((v.color >> 16) & 255);
                            g = static_cast<float>((v.color >> 8) & 255);
                            b = static_cast<float>(v.color & 255);
                            light = (std::max)(light, .30f);
                            break;
                        case DistantCarMesh::Glass: r = 44; g = 62; b = 73; break;
                        case DistantCarMesh::Rubber: r = 23; g = 24; b = 25; break;
                        case DistantCarMesh::Chrome: r = 135; g = 139; b = 140; break;
                        case DistantCarMesh::Headlight: r = 245; g = 239; b = 206; light = (std::max)(light, lamps); break;
                        case DistantCarMesh::Taillight: r = 175; g = 16; b = 10; light = (std::max)(light, lamps); break;
                        default: break;
                    }
                    vertex.color = alpha | rgb(r, g, b, light);
                }
                for (short index : mesh.indices) *outputIndices++ = base + index;

                // Lamps are always on (dimmed by day) and are readable from the
                // side too: a 3D car whose lights only show when it drives
                // straight at the camera reads as dead.
                if (car.boat)
                {
                    if (bDistantCars3DLights)
                    {
                        CVector view = camera - position; view.Normalise();
                        float fwd = DotProduct(forward, view), side = DotProduct(right, view);
                        float sector = Saturate((fwd + .38f) / .15f);
                        for (uint32_t lamp = 0; lamp < 3; ++lamp)
                        {
                            bool port = lamp == 0, starboard = lamp == 1;
                            float strength = lamp == 2 ? 1.0f : sector * Saturate(.5f + (port ? -side : side) * 4.0f);
                            auto alpha = static_cast<uint8_t>(220 * lightFade * strength * (.4f + .6f * lamps));
                            if (alpha < 4) continue;
                            CVector p = world({ port ? -1.6f : starboard ? 1.6f : 0.0f, lamp == 2 ? -3.0f : 2.0f, 1.4f });
                            CLODLights::RegisterCorona(0x7C000000u + (car.id - 0x7F000000u) * 4 + lamp, nullptr,
                                starboard ? 30 : 255, port ? 25 : 240, lamp == 2 ? 220 : 20, alpha, p,
                                .8f * lightSize * fDistantCarsRadiusMultiplier, farClip,
                                1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                        }
                    }
                    continue;
                }
                if (bDistantCars3DLights)
                {
                    CVector view = camera - position; view.Normalise();
                    float facing = DotProduct(view, forward);
                    // Front/rear sectors and a vertical cutoff stop the glow from
                    // shining through the car's own body from above or behind.
                    float vertical = Saturate((.95f - std::abs(DotProduct(view, up))) / .45f);
                    const float glow = .55f + .45f * lamps;
                    for (uint32_t lamp = 0; lamp < 4; ++lamp)
                    {
                        bool front = lamp < 2;
                        // The bias keeps a passing car's lamps lit while the other
                        // end still goes dark when it faces away from us.
                        float directional = Saturate(((front ? facing : -facing) + .45f) / .90f);
                        float strength = directional * vertical * glow;
                        auto alpha = static_cast<uint8_t>(255 * lightFade * strength);
                        if (alpha < 4) continue;
                        // A little outside the body, so the car's own depth never
                        // clips the glow. The size is generous on purpose: a small
                        // sprite is invisible next to the model.
                        CVector p = world({ (lamp % 2) ? .70f : -.70f,front ? 2.42f : -2.42f,.74f });
                        CLODLights::RegisterCorona(0x7C000000u + (car.id - 0x7F000000u) * 4 + lamp, nullptr,
                            255, front ? 246 : 28, front ? 220 : 18, alpha, p,
                            1.6f * lightSize * fDistantCarsRadiusMultiplier, farClip,
                            1, 0, false, false, 0, 0.0f, false, 0.0f, 0, 255.0f, false, false);
                    }
                }
            }
            flush();
        }

    public:
        Frame(const CVector& cam, float farDistance) :camera(cam), farClip(farDistance),
            enabled(bDistantCars3D&& RwIm3DTransform&& RwIm3DRenderIndexedPrimitive&& RwIm3DEnd&& CGame::currArea == 0)
        {
            // This also removes lamps for cars rejected by the caller's distance
            // cull, so no previous-frame glow survives without its model.
            for (uint32_t id : previousLights) HideLights(id);
            previousLights.clear();
            cars.clear();
        }
        void Flush()
        {
            Render();
            if (!cars.empty()) CLODLights::RenderBuffered(true);
            cars.clear();
        }
        bool Add(const CVector& coronaPosition, const CVector& direction, State& state, uint32_t id, float fade, bool boat = false)
        {
            HideLights(id);
            if (!enabled || (boat && !bDistantBoats3D)) return false;
            CLODLights::UnregisterCorona(id);
            if (boat)
            {
                CLODLights::UnregisterCorona(0x7E000000u + (id - 0x7F000000u));
                CLODLights::UnregisterCorona(0x7D000000u + (id - 0x7F000000u));
            }
            // The envelope belongs to the simulated car and is driven by the
            // game module; a car that is fading in or out is drawn dimmed.
            fade = Saturate(fade * state.fade);
            if (fade <= .01f || direction.MagnitudeSqr() < .001f) return true;
            // The existing simulation raises its single corona by 0.55m. The
            // mesh uses a tyre-contact origin, so undo that visual-only offset.
            CVector base = coronaPosition - CVector(0, 0, .55f);
            // Conservative bounds in screen space, allowing the entire model
            // around screen edges. Avoid building/probing cars behind the view.
            RwV3d center = { base.x,base.y,base.z + .8f * (boat ? 1.0f : CarScale) }, screen;
            float width, height;
            if (!CSprite::CalcScreenCoors(&center, &screen, &width, &height, true, true)) return true;
            float radius = boat ? 8.0f : 4.0f * CarScale;
            float marginX = std::abs(width) * radius, marginY = std::abs(height) * radius;
            if (screen.x < -marginX || screen.y < -marginY ||
                screen.x > RsGlobal->width + marginX || screen.y > RsGlobal->height + marginY) return true;
            previousLights.push_back(id);
            cars.push_back({ base,direction,&state,id,fade,(base - camera).MagnitudeSqr(),boat });
            return true;
        }
    };
}
