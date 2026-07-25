# Technical Artist Test — D/E/F Combo Submission

**Candidate:** Đào Nhật Cường
**Tests:** D (Shader Tech) + E (Gameplay Tech) + F (Pipeline)
**Completion Time:** 3 day (cumulative)
**UE Version:** 5.8

---

## 🎯 Test Overview

This repository demonstrates three distinct Technical Artist competency areas — **Dynamic Weather Materials**, a **Modular Ability System**, and an **Enemy Configuration Pipeline Tool** — all built in Unreal Engine 5.8.

| Test | Focus | What Was Built |
|------|-------|----------------|
| **D — Shader Tech** | Master Material + Weather System | One master material driven by a global weather parameter collection (wetness, snow accumulation, temperature), with 6 surface instances. A parallax path exists behind a static switch but is currently off. Mobile child instances are in place as scaffolding — see the note below. |
| **E — Gameplay Tech** | Blueprint Architecture + Data-Driven Abilities | 3 ability blueprints (Fireball, Heal, Teleport) using a modular ActorComponent-based system with Gameplay Tags, Data Assets, Enhanced Input, and a shared ability interface. |
| **F — Pipeline** | Editor Utility Widget + C++ plugin | A 5-step wizard that guides designers through enemy creation — naming, skeletal mesh/material assignment, stat configuration, AI behavior tree binding, and automated Blueprint generation. The EUW owns layout; a two-module C++ editor plugin owns behaviour — rationale below. |

---

## 🚀 Key Features

### Test D — Dynamic Weather Material System

- **Master Material** (`M_SurfaceMaster`) — 5 exposed parameters: 4 texture params (BaseColor, Roughness, Normal, Height) plus a `UseParallax` static switch. Weather is **not** exposed as material parameters; it arrives through 4 `CollectionParameter` nodes reading `MPC_GlobalWeather`, so weather changes never recompile shaders or spawn instances.
- **Material Function Library** — 4 reusable functions, all called from the master graph:
  - `MF_WetnessSurface` — surface-normal-aware wetness lerp
  - `MF_SnowAccumulation` — world-position-based z-up snow masking
  - `MF_TemperatureEffects` — ice formation below 0°C
  - `MF_ParallaxOcclusion` — BumpOffset-based offset mapping (not ray-marched POM)
- **6 Surface Material Instances** — Stone, Wood, Metal, Fabric, Ice, Dirt — all sharing the same weather-driven master, each with its own PBR texture set
- **MPC_GlobalWeather** — Material Parameter Collection with `WetnessAmount` (0–1), `SnowAmount` (0–1), `Temperature` (default 20°C), plus `SunColor` / `SkyTint` vectors for time-of-day shifts
- **BP_WeatherController** (`Content/Materials/WeatherSystem/Weather/`) — drives the MPC values and interpolates weather transitions; also a `BP_WeatherMaterialPreviewRig` for side-by-side surface comparison

> **Known gap — mobile variants are scaffolding, not an optimization.**
> The 6 `MI_Weather_*_Mobile` assets exist and are parented to their desktop counterparts, but they currently carry **zero parameter overrides** — same textures, no base-property overrides. They compile to the same shader as the desktop instances, so there is no measured mobile saving to report yet.
> Relatedly, `UseParallax` is `false` on all 12 instances, so the Height sample and the BumpOffset branch are statically compiled out everywhere. The parallax path is wired and switchable, but nothing in the project currently enables it. (Minor: `MI_Weather_Stone` has its Height slot pointing at the Rock normal map rather than the displacement map — harmless while the switch is off, but wrong if it's ever turned on.)
> Making the mobile path genuinely cheaper — stripping the weather functions and dropping a texture sample behind static switches — is the clear next step and is deliberately left undone rather than overstated.

### Test E — Modular Ability System

- **Component-Based Architecture** — `BP_AbilitySystemComponent` (ActorComponent) manages active abilities, cooldown timers, and input bindings
- **Object-Based Abilities** — `BP_AbilityBase` (Object Blueprint) with virtual functions `CanActivate`, `Execute`, `OnEnd` and data slots for Cooldown, Cost, Duration
- **3 Ability Implementations**:
  - **Fireball** — projectile spawn with Niagara VFX (`NS_Free_Magic_Fire2`), sound cue, forward velocity
  - **Heal** — instant health restore with placeholder VFX and healing sound
  - **Teleport** — line-trace movement with placeholder VFX and teleport sound
- **Gameplay Tags** — hierarchical tags (`Ability.Magic.Fire.Fireball`, etc.) for ability filtering and cooldown categories; configured via `DefaultGameplayTags.ini`
- **Data Assets** — `DA_AbilityData` primary data asset with `DA_Fireball`, `DA_Heal`, `DA_Teleport` entries; blueprint-callable data queries
- **Blueprint Interface** — `BPI_Ability` for standardized ability communication
- **Enhanced Input** — input action bindings with ability triggering through the AbilitySystemComponent
- **Zero Tick usage** — fully event-driven architecture

### Test F — Enemy Configuration Tool

- **5-Step Editor Utility Wizard** (`EUW_EnemyCreationWizard`) with Widget Switcher navigation:
  1. **Identity & Naming** — text input with `BP_` prefix enforcement, optional folder structure creation
  2. **Visual Configuration** — Skeletal Mesh asset picker → dynamic material-slot list with per-slot Material Interface assignment
  3. **Core Gameplay Stats** — sliders/spin boxes for Health, Base Damage, Movement Speed; Combo Box for Enemy Class (Grunt/Scout/Brute/Boss)
  4. **AI & Behavior** — Behavior Tree asset picker (mandatory field)
  5. **Review & Generate** — read-only summary + Progress Bar + Log Output during generation
- **Validation** — "Next" button enabled state bound to per-step validation functions; all material slots must be filled, BT field is mandatory
- **Automated Generation** — creates a Blueprint asset from `BP_BaseEnemy` parent, sets skeletal mesh, assigns materials by slot, configures public variables (Health, Damage, MoveSpeed, BehaviorTreeAsset, EnemyClass)
- **Editor Notification** on success/failure
- **Detailed comments** throughout the Blueprint graph explaining UI interaction and generation logic

#### Why Test F uses C++ (and where the line is drawn)

The wizard is **not** pure Blueprint. Layout and widget naming live in the `EUW_EnemyCreationWizard` asset; behaviour lives in the `EnemyCreationWizardEditor` plugin (`Plugins/EnemyCreationWizardEditor/`). The EUW is auto-reparented to `UEnemyCreationWizardWidget` on editor startup, so the contract is: **the designer owns the layout, C++ owns the logic.**

Only three functions are exposed to Blueprint — `CreateEnemyFromWizard`, `OpenEnemyCreationWizard`, and `ToggleEnemyCreationWizardOverlay`. Everything else is native by necessity:

| Requirement | Why Blueprint can't do it |
|---|---|
| **Filtered asset pickers** for Skeletal Mesh, Behavior Tree, and each material slot | `SObjectPropertyEntryBox` is a Slate widget from the `PropertyEditor` module with no UMG wrapper. It's injected into the widget tree via `UNativeWidgetHost`. |
| **Content-path picker** for the output directory | `FContentBrowserModule::CreatePathPicker` has no Blueprint equivalent. |
| **Arbitrary-N material slot list** | Slot names come from `FSkeletalMaterial::ImportedMaterialSlotName`, which is not a Blueprint-readable property. This is what allows *any* slot count instead of a hardcoded four. |
| **Creating the Blueprint asset** | `FKismetEditorUtilities::CreateBlueprint` + `FAssetRegistryModule::AssetCreated` + `CompileBlueprint`. |
| **Writing defaults onto the generated class** | Defaults are written into the CDO by reflection (`FindFProperty<>` → `SetPropertyValue_InContainer`, incl. `FEnumProperty` and `FClassProperty`). Blueprint has no "set arbitrary named property on a CDO" node. The same reflection runs as a pre-flight contract check on `BP_BaseEnemy` before anything is created, and a failed generation is rolled back (`AssetDeleted` + `MarkAsGarbage`). |
| **Saving / provenance** | `UEditorAssetSubsystem::SaveLoadedAssets` with checkout suppressed; ~20 provenance keys via `UPackage::GetMetaData().SetValue`. |
| **Toast notifications + modal progress** | `FSlateNotificationManager` and `FScopedSlowTask` are not Blueprint-exposed. |
| **Tools menu + toolbar entry, PIE hotkey** | `UToolMenus` extension, and an `IInputProcessor` registered via `FSlateApplication::RegisterInputPreProcessor` for the in-PIE hotkey. |
| **Unit tests** | 4 automation tests (`IMPLEMENT_SIMPLE_AUTOMATION_TEST`) over the name/path/slot-count/summary helpers. |

**Not claimed as C++-only.** Name normalization, numeric range validation, step-validation logic, reading UMG control values, status/log/progress text, `WidgetSwitcher` index and button enable-state, the stat sliders, and the whole runtime `OpponentComponent` are all things Blueprint does fine. They sit in C++ so the helpers are unit-testable and the logic stays in one place, not because Blueprint couldn't express them. A pure-Blueprint wizard is feasible if you accept dropdowns populated from an asset-registry query instead of native pickers, a fixed maximum slot count, and no CDO default-writing — the generated Blueprint would need its values set by hand afterwards.

---

## 🎮 How to Test

### Test D — Weather Materials

1. Open `TA_Test.uproject` in UE 5.8
2. Navigate to the **Materials** content directory:
   - `Content/Materials/Master/M_SurfaceMaster` — master material
   - `Content/Materials/Functions/MF_*` — 4 material functions
   - `Content/Materials/Instances/MI_Weather_*` — surface instances
   - `Content/Materials/Instances/Mobile/MI_Weather_*_Mobile` — mobile children (currently no overrides)
   - `Content/Materials/ParameterCollections/MPC_GlobalWeather` — weather parameter collection
3. Open `Content/Materials/WeatherSystem/Weather/BP_WeatherController` to see the driving Blueprint
4. Open `Lvl_ThirdPerson` — it already contains a preview row of six cubes wearing the six desktop instances, plus a placed `BP_WeatherController`
5. Adjust `WetnessAmount`, `SnowAmount`, and `Temperature` in the MPC, or set `WetnessTarget` / `SnowTarget` / `TemperatureTargetC` on the controller — observe real-time surface transitions

### Test E — Ability System

1. Open any level with a player character
2. Ensure `BP_AbilitySystemComponent` is added to the character
3. Assign `DA_AbilityData` in the component's data asset slot
4. Default **Enhanced Input** bindings:
   - **Left Mouse** — Cast Fireball
   - **Q** — Cast Heal
   - **E** — Cast Teleport
5. Observe: Fireball projectile with Niagara VFX, instant health restore (Heal), and line-trace teleport with VFX/SFX

### Test F — Enemy Creation Wizard

1. Open UE 5.8 Editor with the project loaded
2. In Content Browser, browse to `Content/GeneratedEnemiesDemo/Tools/EUW_EnemyCreationWizard`
3. Right-click → **Run Editor Utility Widget**
4. Follow the 5-step workflow:
   - **Step 1:** Enter enemy name (automatically prefixed with `BP_`), choose output directory, optionally enable sub-folder creation
   - **Step 2:** Select a Skeletal Mesh → fill each material slot that appears
   - **Step 3:** Set Health, Damage, Movement Speed, pick Enemy Class
   - **Step 4:** Select a Behavior Tree
   - **Step 5:** Review summary → Click **Generate Enemy**
5. Check the output directory — a new child Blueprint of `BP_BaseEnemy` is created with all properties configured

---

## 📊 Performance Metrics

### Test D — Material System

**No measured numbers are claimed here.** The material graph compiles and the weather system runs, but no Shader Complexity capture or Material Editor instruction count was recorded, and — because the mobile instances currently carry no overrides — a desktop-vs-mobile comparison would show two identical shaders. Publishing estimated counts as if they were measurements would be misleading, so they have been removed.

What can be stated from the asset graph itself:

| Property | Value (static inspection) |
|---|---|
| **Texture samples, all instances** | 3 — BaseColor, Roughness, Normal. The Height sample is compiled out because `UseParallax=false` everywhere. |
| **Weather cost** | 4 `CollectionParameter` reads from `MPC_GlobalWeather` — no per-instance cost, no recompile on weather change |
| **Material instances** | 12 assets (6 desktop + 6 mobile children), but 6 distinct shaders |
| **Desktop vs mobile delta** | None yet — see the Known gap under Test D |

Remaining sign-off: enable Shader Complexity in the viewport at worst-case weather (`WetnessAmount=1`, `SnowAmount=1`, `Temperature=-10`), read sampler and instruction counts from the Statistics panel, and run a target-device pass. `Lvl_ThirdPerson` already contains a preview row of six cubes wearing the six desktop instances plus a `BP_WeatherController`, so the capture is ready to take once the mobile path is actually differentiated.

### Test E — Ability System

| Metric | Value |
|--------|-------|
| **Tick Usage** | 0 (fully event-driven) |
| **Active Ability Limit** | Configurable (default 10) |
| **Cooldown Granularity** | Per-ability and per-tag-category |
| **Data-Driven Configuration** | All ability parameters in Data Assets |

---

## 🛠 Tools & Techniques Used

### UE5 Systems

| System | Test D | Test E | Test F |
|--------|--------|--------|--------|
| **Material Editor** | ✅ Master, Functions, Instances | — | — |
| **Material Parameter Collection** | ✅ MPC_GlobalWeather | — | — |
| **Material Instances** | ✅ 12 assets (6 desktop + 6 mobile children) | — | — |
| **Static Switch (`UseParallax`)** | ✅ Wired, currently off on all instances | — | — |
| **Shader Complexity View** | ⬜ Not captured — see Performance Metrics | — | — |
| **Mobile Preview Mode** | ⬜ Not run | — | — |
| **Live Preview (test mesh)** | ✅ 6-cube preview row in `Lvl_ThirdPerson` | — | — |
| **Blueprint ActorComponent** | — | ✅ BP_AbilitySystemComponent | — |
| **Blueprint Object** | — | ✅ BP_AbilityBase | — |
| **Blueprint Interface** | — | ✅ BPI_Ability | — |
| **Gameplay Tags** | — | ✅ Ability.Magic.* | ✅ EIN_EnemyClass |
| **Data Assets** | — | ✅ DA_AbilityData chain | — |
| **Enhanced Input** | — | ✅ IA_Fireball / IA_Heal / IA_Teleport | — |
| **Editor Utility Widget** | — | — | ✅ EUW_EnemyCreationWizard |
| **Widget Switcher** | — | — | ✅ 5-step navigation |
| **Asset Picker (filtered)** | — | — | ✅ Native `SObjectPropertyEntryBox` — mesh, BT, per-slot materials |
| **Blueprint generation** | — | — | ✅ `FKismetEditorUtilities` + CDO writes by reflection (C++) |
| **Package saving** | — | — | ✅ `UEditorAssetSubsystem::SaveLoadedAssets` |
| **Editor Notification** | — | — | ✅ `FSlateNotificationManager` success/failure |
| **C++ automation tests** | — | — | ✅ 4 tests over the pure helpers |
| **Niagara VFX** | — | ✅ NS_Free_Magic_Fire2 | — |
| **Behavior Tree** | — | — | ✅ BT_TestEnemy |

### External Resources

| Resource | Usage |
|----------|-------|
| **Free Magic VFX Pack** | Niagara systems and materials for Fireball, Heal, Teleport SFX/VFX |
| **Polyhaven / ambientCG** | PBR texture sets — Fabric, Ground, Ice, Metal, Rock, Wood |
| **Git** | Version control and commit history |

---

## 🎨 Media Preview

> **Not yet recorded.** There is no `Media/` directory in this repository. The only capture present is `Saved/Demos/_recorder_smoke_test.mp4`, a smoke test of the recording setup rather than a demo.
>
> The Test F walkthrough is *planned* — the shot list and step-by-step demo script live in [EnemyCreationWizardDemo.md](Docs/EnemyCreationWizardDemo.md) — but the final video (and the required screenshots) have not been captured yet.

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [DynamicWeatherMaterialSystem.md](Docs/DynamicWeatherMaterialSystem.md) | Test D — master material structure, weather functions, and MPC wiring |
| [DynamicWeatherOptimizationReport.md](Docs/DynamicWeatherOptimizationReport.md) | Test D — optimization decisions and the outstanding visual sign-off |
| [EnemyCreationWizardDemo.md](Docs/EnemyCreationWizardDemo.md) | Test F — 12-minute end-to-end demo script and expected outputs |

---

## 📁 Repository Structure

The repository root *is* the UE project root — there is no submission wrapper directory.

```
TA_Test/
├── README.md                                    # This file
├── TA_Test.uproject                             # UE 5.8 project file
├── Docs/
│   ├── DynamicWeatherMaterialSystem.md          # Test D: material system notes
│   ├── DynamicWeatherOptimizationReport.md      # Test D: optimization decisions
│   └── EnemyCreationWizardDemo.md               # Test F: demo script
├── Scripts/
│   └── RecordEnemyWizardDemo.ps1                # Test F: screen-recording helper
├── Plugins/
│   └── EnemyCreationWizardEditor/               # Test F: C++ plugin (2 modules)
│       └── Source/
│           ├── EnemyCreationWizardEditor/       # Editor module — widget, generator, tests
│           └── EnemyCreationWizardRuntime/      # Runtime module — bridge, opponent component
├── Config/
│   ├── DefaultEditor.ini
│   ├── DefaultGame.ini
│   └── DefaultGameplayTags.ini                  # Tag hierarchy for abilities
└── Content/
    ├── Blueprints/
    │   ├── Ability/                             # Test E: BP_AbilityBase, Fireball, Heal, Teleport
    │   ├── Abilities/                           # Test E: earlier iteration (BP_Fireball)
    │   ├── Components/                          # Test E: BP_AbilitySystemComponent
    │   ├── Data/                                # Test E: DA_AbilityData, DA_Fireball, etc.
    │   ├── Enemies/                             # Test F: BP_BaseEnemy, BP_TestEnemy, BT_TestEnemy
    │   ├── Enum/                                # Test F: EIN_EnemyClass
    │   ├── Interfaces/                          # Test E: BPI_Ability
    │   ├── Image/                               # Ability icons (Fire, Heal, Teleport)
    │   ├── Sound/                               # Ability SFX
    │   ├── Struct/                              # Shared data structures
    │   └── UI/                                  # WBP_SkillBar HUD
    ├── Materials/
    │   ├── Master/M_SurfaceMaster               # Test D: master material
    │   ├── Functions/MF_*                       # Test D: 4 material functions
    │   ├── Instances/MI_Weather_*               # Test D: 6 surface MIs
    │   ├── Instances/Mobile/MI_*_Mobile         # Test D: 6 mobile children
    │   ├── ParameterCollections/                # Test D: MPC_GlobalWeather
    │   ├── WeatherSystem/Weather/               # Test D: BP_WeatherController + preview rig
    │   └── ImageMaterials/                      # PBR texture source sets
    ├── GeneratedEnemiesDemo/                    # Test F: wizard + generated output
    │   ├── Editor/Widgets/EUW_EnemyCreationWizard   # Canonical widget (auto-reparented to C++)
    │   ├── Tools/EUW_EnemyCreationWizard        # Runnable entry point
    │   └── Blueprints/BP_DemoGoblin             # Sample generated enemy
    ├── Free_Magic/                              # VFX pack (Niagara systems, materials, meshes)
    ├── LevelPrototyping/                        # Test environment interactables
    ├── ThirdPerson/Lvl_ThirdPerson              # Main test level + weather preview row
    └── Characters/                              # Mannequin characters
```

No `Source/` directory at the project root — all C++ lives in the plugin under `Plugins/`.

---

## 🔍 Review Highlights

### Test D — Shader Tech
- Modular master material — 4 texture parameters plus a `UseParallax` static switch, with weather kept out of the parameter list entirely
- Reusable material functions that can be dropped into any project independently
- Weather driven globally through an MPC — changing weather never recompiles a shader or spawns a material instance
- Real-time wetness / snow / temperature blending on the six surface instances
- **Incomplete:** the mobile instances carry no overrides yet, and no Shader Complexity or Mobile Preview capture has been taken. Both are documented above rather than claimed as done.

### Test E — Gameplay Tech
- Clean, scalable Blueprint architecture — ActorComponent + Object pattern with interface binding
- Proper interface-based communication, not direct class coupling (BPI_Ability)
- Data-driven design — all ability parameters from Data Assets, zero hard-coded values
- No Tick — fully event-driven, nativization-ready, memory-efficient

### Test F — Pipeline
- Editor Utility Widget for layout, C++ plugin for behaviour, with the EUW auto-reparented to the native class on startup
- Native filtered asset pickers (`SObjectPropertyEntryBox`) hosted inside UMG — not approximated with dropdowns
- Dynamic material slot population from the mesh's real imported slot names, for any slot count
- Automated generation with a pre-flight contract check on `BP_BaseEnemy`, CDO defaults written by reflection, provenance metadata, and rollback on failure
- 4 C++ automation tests covering the name / path / slot-count / summary helpers
- **Untested by automation:** `CreateEnemyFromWizard` itself, the CDO reflection writes, and the widget all need a live editor and are only verified manually

---

## 📤 Submission Notes

- **Repository type:** Private GitHub — access granted to designated reviewers
- **Contact:** daonguyenhoang.dex@gmail.com
- **Email to:** hai.huynh@atherlabs.com
- **Subject line:** "TA Test Submission — Đào Nhật Cường — Test D-E-F"

---

*Built with Unreal Engine 5.8 • Technical Artist Candidate — Đào Nhật Cường*
