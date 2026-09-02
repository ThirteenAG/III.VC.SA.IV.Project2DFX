<div align="center">

<img src="https://github.com/user-attachments/assets/ffb0d535-7125-4956-adac-510a6b1f8d41" width="760" alt="III.Project2DFX">

**III.Project2DFX** adds the Project2DFX LOD corona effect to Grand Theft Auto III, lighting up the entire city with thousands of distant lamppost and streetlight glows, and greatly extends the draw distance.

[Website](https://fusionfix.io/p2dfx) · [Source Code](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX) · [Default INI](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/blob/master/data/IIILodLights/IIILodLights.ini)

</div>

---

## Features

<table>
<tr>
<td width="58%" valign="middle">

- **LOD Coronas** - up to 25000 lamppost, streetlight and neon glows are rendered across the entire map, with a configurable corona radius and an alpha curve that boosts distant coronas
- **Dynamic Far Clip** - extended draw distance that adapts to your height and targets a smooth framerate
- **Distant Cars** - up to 2000 distant vehicles with visible headlight and taillight trails
- **Static Shadows** - stronger and further-reaching shadows for peds, cars and traffic lights
- **Volumetric Light Cones** - light beams are rendered as volumetric cones across the city at night

</td>
<td width="42%" valign="middle" align="center">

<img src="https://fusionfix.io/screens/2dfx/p2dfx_gta3a.jpg" width="360" alt="LOD coronas">

</td>
</tr>
</table>

---

## Showcase

<div align="center">

**LOD Coronas** - thousands of distant lamppost and streetlight glows

<img src="https://raw.githubusercontent.com/ThirteenAG/III.VC.SA.IV.Project2DFX/refs/heads/master/.github/docs/videos/iii_demo.webp" alt="LOD coronas">

**Volumetric Light Cones** - volumetric light beams at night

<img src="https://raw.githubusercontent.com/ThirteenAG/III.VC.SA.IV.Project2DFX/refs/heads/master/.github/docs/videos/iii_vol_lights.webp" alt="Volumetric lights">

</div>

---

## Screenshots

<div align="center">

| | |
|:---:|:---:|
| <img src="https://github.com/user-attachments/assets/ffb0d535-7125-4956-adac-510a6b1f8d41" width="360" alt="Night skyline"> | <img src="https://fusionfix.io/screens/2dfx/p2dfx_gta3a.jpg" width="360" alt="City lights"> |
| <img src="https://fusionfix.io/screens/2dfx/p2dfx_gta3b.jpg" width="360" alt="Distant coronas"> | <img src="https://github.com/user-attachments/assets/21b2d22e-7bb7-46db-8469-ab8749a548ab" width="360" alt="Draw distance"> |

</div>

---

## Installation

Game version 1.0 US is required.

1. Download and install [GTA III Widescreen Fix](http://thirteenag.github.io/wfp#gta3) - it includes the required [ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
2. Download [III.Project2DFX](https://github.com/ThirteenAG/III.VC.SA.IV.Project2DFX/releases/download/gta3/III.Project2DFX.zip).
3. Put the `.asi`, `.ini` and `.dat` files into the `scripts` folder.
4. Launch the game.

<details>
  <summary> Configuring III.Project2DFX - click to expand </summary>
<br>

All options are stored in `IIILodLights.ini` inside the `scripts` folder:

```
[LodLights]
RenderLodLights = 1                 ; enable distant LOD coronas (0|1)
MaxNumberOfLodLights = 25000        ; maximum number of coronas rendered at once
CoronaRadiusMultiplier = 1.0
SlightlyIncreaseRadiusWithDistance = 1

[StaticShadows]
IncreasePedsCarsShadowsDrawDistance = 1
StaticShadowsIntensity = 2.0
StaticShadowsDrawDistance = 300.0

[SearchLights]                        ; volumetric light cones
RenderSearchlightEffects = 1
RenderOnlyDuringFoggyWeather = 0

[FarClip]
FarClipMultiplier = 0.0             ; 0.0 = dynamic, <= 10.0 = fixed multiplier, > 10.0 = static farclip
HeightFactor = 0.05
MinMultiplier = 2.0
MaxMultiplier = 4.0
TargetFPS = 60

[IDETweaker]
DrawDistance = 5.0                  ; original maximum value is 1.8
MaxDrawDistanceForNormalObjects = 0.0

[Misc]
RandomExplosionEffects = 1
ReplaceSmokeTrailWithBulletTrail = 1
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
