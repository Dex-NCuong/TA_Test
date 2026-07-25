using UnrealBuildTool;

public class EnemyCreationWizardRuntime : ModuleRules
{
    public EnemyCreationWizardRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "Blutility",
                "UnrealEd",
                "UMGEditor"
            });
        }
    }
}
