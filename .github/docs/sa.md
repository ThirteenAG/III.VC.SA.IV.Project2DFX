<div align="center">

<img src="https://github.com/user-attachments/assets/1787b919-3874-454e-a6f8-bf791589596f" width="760" alt="SA.Project2DFX">

**SA.Project2DFX** adds the Project2DFX LOD corona effect to Grand Theft Auto: San Andreas, lighting up the entire state with thousands of distant lamppost and streetlight glows, and greatly extends the draw distance.

[Website](https://fusionfix.io/p2dfx) · [Source Code](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX) · [Default INI](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/blob/master/data/SALodLights/SALodLights.ini)

</div>

---

## Features

<table>
<tr>
<td width="58%" valign="middle">

- **LOD Coronas** - up to 25000 lamppost and streetlight glows are rendered across the entire state, with a configurable corona radius and an alpha curve that boosts distant coronas
- **Dynamic Far Clip** - extended draw distance that adapts to your height and targets a smooth framerate
- **Distant Cars** - up to 2000 distant vehicles with visible headlight and taillight trails
- **Maritime Traffic** - distant boat lights along water paths
- **Static Shadows** - stronger shadows with a configurable draw distance and a bigger sun
- **Volumetric Light Cones** - light beams are rendered as volumetric cones across the map at night
- **IDE Tweaker** - preloaded LODs and boosted draw distances for LOD, generic, vegetation and timed objects

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

| Gameplay | In-Depth Guide (all INI settings) |
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

All options are stored in `SALodLights.ini` inside the `scripts` folder:

```
[LodLights]
RenderLodLights = 1                 ; enable distant LOD coronas (0|1)
MaxNumberOfLodLights = 25000        ; maximum number of coronas rendered at once
CoronaRadiusMultiplier = 0.5
SlightlyIncreaseRadiusWithDistance = 1
MaritimeTraffic = 1                 ; 1 = distant boat lights on water paths, 0 = disabled

[StaticShadows]
StaticShadowsIntensity = 2.0
StaticShadowsDrawDistance = 200.0

[SearchLights]                        ; volumetric light cones
RenderSearchlightEffects = 1
RenderOnlyDuringFoggyWeather = 0

[FarClip]
FarClipMultiplier = 0.0             ; 0.0 = dynamic, <= 10.0 = fixed multiplier, > 10.0 = static farclip
HeightFactor = 0.05
MinMultiplier = 2.0
MaxMultiplier = 4.0
TargetFPS = 60
StaticSunSize = 20.0

[IDETweaker]
TimedObjectsDrawDistance = 0.0
LODObjectsDrawDistance = 1300.0
GenericObjectsDrawDistance = 300.0
AllNormalObjectsDrawDistance = 0.0
VegetationDrawDistance = 800.0
PreloadLODs = 1
```

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
  <a href="https://discord.gg/2ckFCS572Z" target="_blank"><img width="200" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/discord.svg"></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://t.me/fusionfix" target="_blank"><img width="200" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/telegram.svg"></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://www.youtube.com/@FusionFix10" target="_blank"><img width="200" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/youtube.svg"></a>
  &nbsp;&nbsp;&nbsp;
  <a href="https://x.com/fusionfix10" target="_blank"><img width="200" src="https://raw.githubusercontent.com/ThirteenAG/GTAIV.EFLC.FusionFix/refs/heads/master/installer/x.svg"></a>
  &nbsp;&nbsp;&nbsp;
</p>
