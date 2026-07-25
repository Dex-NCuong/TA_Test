# Dynamic Weather Material System

## Delivered assets

- `/Game/Materials/Master/M_SurfaceMaster` - default-lit weather master with BaseColor, Roughness, Normal, Height and `UseParallax` static switch.
- `/Game/Materials/Functions/MF_WetnessSurface` - darkens color, lowers roughness, flattens normals, and attenuates wetness using `dot(VertexNormalWS, WorldUp)`.
- `/Game/Materials/Functions/MF_SnowAccumulation` - upward-facing snow mask using `VertexNormalWS` and world-up dot product.
- `/Game/Materials/Functions/MF_TemperatureEffects` - ice roughness response below 0 C.
- `/Game/Materials/Functions/MF_ParallaxOcclusion` - BumpOffset parallax UV implementation. It is intentionally a lightweight approximation rather than ray-marched POM.
- `/Game/Materials/ParameterCollections/MPC_GlobalWeather` - scalars `WetnessAmount` (default 0), `SnowAmount` (default 0), `Temperature` (default 20); vectors `SunColor`, `SkyTint`.
- `/Game/Materials/WeatherSystem/Weather/BP_WeatherController` - level actor that initializes and interpolates the three MPC scalars every tick. The editable target values retarget transitions from their current state.
- `/Game/Materials/WeatherSystem/Weather/BP_WeatherMaterialPreviewRig` - side-by-side surface comparison rig.
- `/Game/Materials/Instances/MI_Weather_{Stone,Wood,Metal,Fabric,Dirt,Ice}` plus matching `/Mobile` children.

> **Mobile children are scaffolding.** Each `MI_Weather_*_Mobile` is parented to its desktop instance but currently has **no parameter overrides and no base-property overrides**, so it compiles to the same shader as its parent. There is no desktop/mobile cost difference in the project as it stands.
>
> `UseParallax` is also `false` on all 12 instances, so the Height sample and the BumpOffset branch are compiled out everywhere. The switch is wired and functional; nothing currently turns it on.

## Use

1. Place exactly one `BP_WeatherController` in the active level (one has been placed in `Lvl_ThirdPerson`).
2. Set `WetnessTarget`, `SnowTarget`, `TemperatureTargetC`, and a positive `TransitionDuration` on the controller.
3. Assign a surface material instance to a mesh. `UseParallax` is currently off on every instance, desktop and mobile alike — enable it per-instance to see the BumpOffset path.
4. Use the Material Editor preview or PIE to sweep MPC values for direct debugging.

## Time of day integration

The `BP_WeatherController` now includes a full time-of-day color temperature system:

- **4-keyframe SunColor**: Night → Dawn → Noon → Dusk with smooth lerp between each phase
- **4-keyframe SkyTint**: Complementary sky colors matching each time phase
- **Day/night intensity**: Sin-based daylight curve for light brightness (0-1)
- **Directional Light control**: Assign a `LightComponent` reference to the optional `DirectionalLight` variable — the controller will set its UseTemperature, Temperature, and Intensity.
- **Preview events**: Call `PreviewDawn`/`PreviewNoon`/`PreviewDusk`/`PreviewNight` to jump to specific times, or `PreviewTimeOfDay(Hour)` for a custom hour.
- **Toggle**: Set `EnableTimeOfDay` to false to disable the cycle and keep the current TimeOfDayHours value.
- **Speed**: Adjust `TimeSpeedHoursPerSec` — 1.0 = 1 real second = 1 in-game hour (a full day takes 24 seconds).

The MPC `SunColor` and `SkyTint` vector parameters are updated each tick when time-of-day is active. Scene materials can sample these from the MPC for ambient lighting effects.

### Weather transition animations (Smoothstep Easing)

Weather transitions now use a **Smoothstep** easing curve instead of raw `FInterpTo`:

```
easedAlpha = t * t * (3 - 2 * t)    where t = progress / duration
```

This provides a natural ease-in-out: the transition starts slowly, accelerates mid-way, and decelerates toward the target. The `TransitionDuration` parameter now exactly controls the time to completion (not an asymptotic approach).

When a new transition starts (via PreviewRain etc.), the current weather state is stored as the `Old*` values, and the progress counter resets. Rapidly switching weather mid-transition smoothly redirects from the current interpolated state.

### New Variables

| Variable | Type | Category | Description |
|---|---|---|---|
| `DirectionalLight` | LightComponent (obj) | WeatherTargets | Optional: assign level' s directional light for automatic temperature/intensity |
| `TransitionProgress` | float | Transition | Current transition progress (0-1) |
| `IsTransitioning` | bool | Transition | Whether a transition is in progress |
| `OldWetness` | float | Transition | Wetness value at transition start |
| `OldSnow` | float | Transition | Snow value at transition start |
| `OldTemperature` | float | Transition | Temperature value at transition start |

### New Custom Events

| Event | Description |
|---|---|
| `PreviewDawn` | Sets time to 6:00 and enables time-of-day |
| `PreviewNoon` | Sets time to 12:00 and enables time-of-day |
| `PreviewDusk` | Sets time to 18:00 and enables time-of-day |
| `PreviewNight` | Sets time to 0:00 and enables time-of-day |
| `PreviewTimeOfDay(Hour)` | Sets time to any hour (edit Hour in Details panel before calling) |
| `PreviewDry` | Transitions to dry weather with smoothstep |
| `PreviewRain` | Transitions to rainy weather with smoothstep |
| `PreviewSnow` | Transitions to snowy weather with smoothstep |
| `PreviewFrozen` | Transitions to freezing weather with smoothstep |
| `ApplyWeatherImmediate` | Jumps instantly to target weather, ends transition |

## Applied Surface Packs

The supplied project texture packs are now assigned to the instances:

- `MI_Weather_Stone` -> Rock050 (color, roughness, normal). **Note:** its `Height` slot points at `Rock050_..._NormalDX` rather than the displacement map. Harmless while `UseParallax` is off, but wrong if it is ever enabled.
- `MI_Weather_Wood` -> WoodFloor006 (color, roughness, normal, displacement)
- `MI_Weather_Metal` -> Metal049 (color, roughness, normal, displacement)
- `MI_Weather_Fabric` -> Fabric066 (color, roughness, normal, displacement)
- `MI_Weather_Dirt` -> Ground067 (color, roughness, normal, displacement)
- `MI_Weather_Ice` -> Ice003 (color, roughness, normal, displacement)
