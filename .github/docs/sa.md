<div align="center">

<img src="https://github.com/user-attachments/assets/1787b919-3874-454e-a6f8-bf791589596f" width="760" alt="SA.Project2DFX">

**SA.Project2DFX** adds the Project2DFX LOD corona effect to Grand Theft Auto: San Andreas, lighting up the entire state with thousands of distant lamppost and streetlight glows, and greatly extends the draw distance.

[Website](https://fusionfix.io/p2dfx) · [Source Code](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX)

</div>

---

## Features

<table>
<tr>
<td width="58%" valign="middle">

- **LOD Coronas** - lamppost and streetlight glows are rendered across the entire state
- **Draw Distance** - a farther view with LOD buildings out to the fog, and a separate setting for small objects and trees
- **Distant Traffic** - distant cars and boats, as lights or as 3D vehicles
- **Shadows** - shadows under street lamps and traffic lights, and of cars and people, drawn much farther
- **Volumetric Light Cones** - light beams are rendered as volumetric cones across the map at night

</td>
<td width="42%" valign="middle" align="center">

<img src="https://fusionfix.io/screens/2dfx/p2dfx_gtasaa.jpg" width="360" alt="LOD coronas">

</td>
</tr>
</table>

---

## Showcase

<div align="center">

**LOD Coronas** - thousands of distant lamppost and streetlight glows
<img src="https://raw.githubusercontent.com/ThirteenAG/III.VC.SA.IV.Project2DFX/refs/heads/master/.github/docs/videos/sa_demo.webp" alt="LOD coronas">

**Volumetric Light Cones** - volumetric light beams at night
<img src="https://raw.githubusercontent.com/ThirteenAG/III.VC.SA.IV.Project2DFX/refs/heads/master/.github/docs/videos/sa_vol_lights.webp" alt="Volumetric lights">

</div>

---

## Screenshots

<div align="center">

| | |
|:---:|:---:|
| <img src="https://github.com/user-attachments/assets/1787b919-3874-454e-a6f8-bf791589596f" width="360" alt="Night skyline"> | <img src="https://fusionfix.io/screens/2dfx/p2dfx_gtasaa.jpg" width="360" alt="Statewide lights"> |
| <img src="https://fusionfix.io/screens/2dfx/p2dfx_gtasab.jpg" width="360" alt="Distant coronas"> | <img src="https://github.com/user-attachments/assets/2597905a-089e-457d-a9c0-e80e77d1e955" width="360" alt="Draw distance"> |

</div>

---

## Video

<div align="center">

| Gameplay | In-Depth Guide |
|:---:|:---:|
| <a href="https://www.youtube.com/watch?v=o8j6PzFjgbA" target="_blank"><img src="https://i.ytimg.com/vi/o8j6PzFjgbA/hqdefault.jpg" width="360" alt="SA.Project2DFX gameplay"></a> | <a href="https://youtu.be/kNiOpksYVOs" target="_blank"><img src="https://i.ytimg.com/vi/kNiOpksYVOs/hqdefault.jpg" width="360" alt="Project2DFX SA in-depth guide"></a> |

</div>

---

## Installation

Game version 1.0 US is required.

1. Download and install [GTASA Widescreen Fix](http://thirteenag.github.io/wfp#gtasa) - it includes the required [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
2. Download [SA.Project2DFX](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/download/gtasa/SA.Project2DFX.zip).
3. Put all the files into the `scripts` folder.
4. Launch the game.

<details>
  <summary> Configuring SA.Project2DFX - click to expand </summary>
<br>

Grass and procedural objects keep their original distances at 1. Longer procedural ranges require Open Limit Adjuster with enlarged ProcObjects, PlantTriangles and PlantCollisionEntries pools. The multiplier is limited by the smallest pool and capped at 3. Grass keeps its gradual fade; higher detail costs more performance.

The settings are in the game's menu with an ASI Settings menu host, in FusionFixHub, or in `SALodLights.cfg` next to the plugin. Detail Distance changes model distances after a restart in San Andreas. Grass and procedural objects follow the setting while playing.

Distant Light Pools use the street lamps’ own colours and take over beyond the game’s nearby pools. Warning lights and traffic signals do not receive distant pools. Pools keep defined centres and soft edges, with restrained growth at longer distances. Distant pools require a compatible Direct3D 9 renderer and the game’s anti-aliasing (MSAA) off. They use a smooth depth-based falloff; with MSAA on or unsupported depth sampling, only the game’s nearby pools remain. The effect follows visible geometry; it does not trace shadows through walls.

| Setting | What it does | Vanilla · Recommended · Enhanced |
|---|---|---|
| View Distance (m) | Extra metres added to the weather’s view distance and fog start | 0 (game) · +1000 · +2000 |
| Draw Distance | How far objects and LOD models stay visible; raise it for more scenery, or lower it for better performance | 1 (game) · 2.5 · 4 |
| Detail Distance | How far small objects, trees, grass and procedural rocks and bushes are drawn | 1 (game) · 2 · 3 |
| Shadow Distance | How far shadows under lamps and traffic lights, and of cars and people, are drawn (after restart) | 1 (game) · 4 · 7.5 |
| Distant Lights | Street lamps and other lights glow far across the map at night | off · on · on |
| Distant Light Pools | Distant street lamps light roads, slopes and nearby walls, following the visible surfaces; requires MSAA off | off · on · on |
| Distant Traffic | Cars and boats far away: off, only their lights, or the vehicles too | off · vehicles · vehicles |
| Searchlights | Light cones of searchlights and street lamps | off · on · on |

The bundled [Open Limit Adjuster](https://github.com/GTAmodding/III.VC.SA.LimitAdjuster) lets the game's lists grow with the draw distance. Without it View Distance is capped at +500 m, and Draw Distance and Detail Distance at 1.5.

View Distance adds the same number of metres to the current weather’s far clip and fog start. For example, +1000 m turns a 600 m view into 1600 m and a 1200 m view into 2200 m. The weather’s own differences and fog ramp remain, with normal transitions as the weather and time change. Interiors, cutscenes, and San Andreas underwater and tunnel views keep the game’s distance. Draw Distance controls objects and LOD models separately; more view distance cannot create scenery beyond their range.

View Distance and Draw Distance apply live. Detail Distance requires a restart in San Andreas; Shadow Distance requires a restart. Start with Recommended and lower Draw Distance or Detail Distance if performance suffers.

</details>

---

## Support the project

<p align="center">
  <a href="https://patreon.fusionfix.io/" target="_blank"><picture><source media="(max-width: 768px) and (prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-p2dfx-mobile-dark.svg"><source media="(max-width: 768px)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-p2dfx-mobile.svg"><source media="(prefers-color-scheme: dark)" srcset="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-p2dfx-dark.svg"><img width="100%" src="https://fusionlegacyinitiative.com/sponsors-progress/sponsors-progress-p2dfx.svg"></picture></a>
  <br />
  <a href="https://github.com/sponsors/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/github-dark.svg"><img src="https://thirteenag.github.io/img/buttons/github.svg" width="250"></picture></a>
  <a href="https://ko-fi.com/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/kofi-dark.svg"><img src="https://thirteenag.github.io/img/buttons/kofi.svg" width="250"></picture></a>
  <a href="https://paypal.me/SergeyP13"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/paypal-dark.svg"><img src="https://thirteenag.github.io/img/buttons/paypal.svg" width="250"></picture></a>
  <a href="https://www.patreon.com/ThirteenAG"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/patreon-dark.svg"><img src="https://thirteenag.github.io/img/buttons/patreon.svg" width="250"></picture></a>
  <a href="https://boosty.to/thirteenag"><picture><source media="(prefers-color-scheme: dark)" srcset="https://thirteenag.github.io/img/buttons/boosty-dark.svg"><img src="https://thirteenag.github.io/img/buttons/boosty.svg" width="250"></picture></a><br><br>
  <a href="https://discord.gg/2ckFCS572Z" target="_blank"><img width="50" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/discord.svg"></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://t.me/fusionfix" target="_blank"><img width="50" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/telegram.svg"></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.youtube.com/@FusionFix10" target="_blank"><img width="50" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/youtube.svg"></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://x.com/fusionfix10" target="_blank"><img width="50" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/x.svg"></a>
  &nbsp;&nbsp;&nbsp;
</p>
