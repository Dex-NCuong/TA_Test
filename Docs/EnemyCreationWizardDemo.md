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


## Expected output

- `/Game/GeneratedEnemiesDemo/Blueprints/BP_DemoGoblin`
- Optional AI Controller under `/Game/GeneratedEnemiesDemo/AI` when enabled.
- Assigned Skeletal Mesh, all real material-slot overrides, gameplay values, enemy class, and Behavior Tree.

## Recording


