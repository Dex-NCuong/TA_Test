# Dynamic Weather Material Optimization Report

## Scope

Static review of `/Game/Materials/Master/M_SurfaceMaster` and its instances, verified by inspecting the compiled assets in the editor.

## Current state — read this first

An earlier revision of this report described a desktop-vs-mobile split that **does not exist in the project**. Corrected findings:

| Claim previously made | Actual state |
|---|---|
| Desktop runs `UseParallax=true` with a Height sample | `UseParallax` is `false` on **all 12** instances. No instance samples Height. |
| Mobile children strip parallax for a saving | Mobile children have **zero** parameter and base-property overrides. They compile to the same shader as their desktop parents. |
| Mobile variants use fewer texture samples | Identical texture assignments to their parents. |

So there is currently **one shader path**, not three:

| Path | Texture samples | Work performed | Instances |
|---|---:|---|---|
| Effective (all instances) | BaseColor, Roughness, Normal | Wetness, snow, temperature | All 12 (6 desktop + 6 mobile children) |
| Parallax (wired, unused) | + Height | + BumpOffset UV | None — switch off everywhere |

## Optimization decisions that do hold

- Weather state comes from an MPC; changing weather does not create material instances or trigger shader recompilation. This is the main structural win and it is real.
- Wetness and snow reuse the base normal and texture path instead of adding separate weather textures.
- `MF_ParallaxOcclusion` is BumpOffset, not expensive ray-marched POM — so the parallax path, when enabled, stays cheap.
- Parallax is behind a static switch, so instances that leave it off genuinely compile the branch out. The mechanism works; it is just not being used to differentiate platforms yet.

## Outstanding work

The mobile tier is unimplemented, not optimized. To make it real:

1. Add static switches to `M_SurfaceMaster` gating the wetness / snow / temperature function calls.
2. Turn those off on the six `MI_Weather_*_Mobile` children, and enable `UseParallax` on the desktop parents so the two tiers actually diverge.
3. Fix `MI_Weather_Stone`'s `Height` slot — it currently points at the Rock050 normal map instead of the displacement map.
4. Only then record instruction and sampler counts, since before this the two tiers are the same shader.

## Required visual validation

Worth doing once the tiers actually differ — capturing now would produce two identical images with different captions.

1. In the viewport enable **Shader Complexity** and inspect dry, wet, snow, and frozen states.
2. Record screenshots with `WetnessAmount=1`, `SnowAmount=1`, `Temperature=-10` as the worst weather state.
3. Run each `MI_Weather_*_Mobile` in **Mobile Preview** and compare against its desktop parent.
4. Confirm texture sampler and instruction counts in the Material Editor Statistics panel before shipping.

`Lvl_ThirdPerson` already holds a preview row of six cubes wearing the six desktop instances plus a placed `BP_WeatherController`, so the capture itself is quick.

## Limitation

No Shader Complexity image, Statistics-panel export, or target-device run has been recorded. No measured performance number in this repository has been verified — any figure quoted before this revision was an estimate presented as a measurement, and has been removed rather than restated.

## Applied Texture Packs

The desktop material instances use Rock050, WoodFloor006, Metal049, Fabric066, Ground067, and Ice003 color/roughness/normal maps. Displacement maps are assigned to the `Height` slot but go unsampled while `UseParallax` is off (and `MI_Weather_Stone`'s `Height` slot points at its normal map rather than displacement). Mobile children inherit every texture unchanged.
