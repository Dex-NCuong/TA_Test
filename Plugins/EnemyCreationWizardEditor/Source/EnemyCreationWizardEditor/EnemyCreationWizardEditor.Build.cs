using UnrealBuildTool;

public class EnemyCreationWizardEditor : ModuleRules
{
    public EnemyCreationWizardEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
            "Blutility",
            "GameplayTags"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "EnemyCreationWizardRuntime",
            "UnrealEd",
            "AssetRegistry",
            "AssetTools",
            "AIModule",
            "Slate",
            "SlateCore",
            "PropertyEditor",
            "ContentBrowser",
            "UMGEditor",
            "InputCore",
            "BlueprintEditorLibrary",
            "ToolMenus",
            "LevelEditor"
        });
    }
}


