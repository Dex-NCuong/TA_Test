# Enemy Creation Wizard — 12-minute end-to-end demo

## Preconditions

- Save all open assets before beginning.
- The updated `EnemyCreationWizardEditor` plugin binary must already be loaded by Unreal Editor.
- Canonical widget: `/Game/GeneratedEnemiesDemo/Editor/Widgets/EUW_EnemyCreationWizard`.
- Demo Behavior Tree: `/Game/Blueprints/Enemies/BT_TestEnemy`.
- Do not run PIE while generating editor assets.

## Demo data

- Enemy name: `DemoGoblin` (the UI must normalize this immediately to `BP_DemoGoblin`).
- Base directory: `/Game/GeneratedEnemiesDemo`.
- Create sub-folders: enabled.
- Skeletal Mesh: `/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple`.
- Materials: keep the defaults discovered from the selected mesh, or choose compatible Material Interface assets with the native slot pickers.
- Health: `150`.
- Damage: `20`.
- Movement Speed: `450`.
- Enemy Class: `Grunt`.
- Behavior Tree: `/Game/Blueprints/Enemies/BT_TestEnemy`.

## Timeline (12:00)

| Time | Action |
|---|---|
| 00:00–00:40 | Introduce the tool, identify the canonical Editor Utility Widget, and show the five-step workflow. |
| 00:40–01:50 | Step 1: enter `DemoGoblin`; show immediate `BP_` normalization. Open the native content-path picker and choose `/Game/GeneratedEnemiesDemo`. |
| 01:50–04:20 | Step 2: use the filtered native Skeletal Mesh picker. Select Quinn. Show that the material list is generated from the mesh's real slots and that every row is a filtered Material Interface picker. |
| 04:20–05:30 | Optionally change one material, then restore/confirm the desired materials. Explain support for meshes with more than four slots. |
| 05:30–06:40 | Step 3: enter Health, Damage, Movement Speed, and Enemy Class. |
| 06:40–07:50 | Step 4: use the filtered native Behavior Tree picker and select `BT_TestEnemy`. |
| 07:50–09:20 | Step 5: review every concrete value, resolved output path, and material slot assignment. |
| 09:20–10:20 | Click **Generate Enemy** and show progress, status, log, and success notification. |
| 10:20–11:20 | Browse to `/Game/GeneratedEnemiesDemo/Blueprints/BP_DemoGoblin`; open it and inspect defaults/metadata. |
| 11:20–12:00 | Summarize validation and generated outputs. Mention that PIE must be stopped during asset generation. |

## Expected output

- `/Game/GeneratedEnemiesDemo/Blueprints/BP_DemoGoblin`
- Optional AI Controller under `/Game/GeneratedEnemiesDemo/AI` when enabled.
- Assigned Skeletal Mesh, all real material-slot overrides, gameplay values, enemy class, and Behavior Tree.

## Recording

The demo has not been captured yet. Record the primary monitor while following the steps above and save the result to:

`Saved/Demos/EnemyCreationWizard_Demo_12min.mp4`

(A recording helper script is not committed to the repo — use OBS or any screen recorder of your choice.)
