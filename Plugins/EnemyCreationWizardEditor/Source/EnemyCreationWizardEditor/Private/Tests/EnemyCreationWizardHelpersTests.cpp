#include "Misc/AutomationTest.h"

#include "EnemyCreationWizardHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyWizardNormalizeNameTest,
    "EnemyCreationWizard.Helpers.NormalizeBlueprintName",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyWizardNormalizeNameTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Empty remains empty"), EnemyCreationWizard::NormalizeBlueprintName(TEXT("   ")), FString());
    TestEqual(TEXT("Missing prefix is added"), EnemyCreationWizard::NormalizeBlueprintName(TEXT("Goblin")), FString(TEXT("BP_Goblin")));
    TestEqual(TEXT("Existing prefix remains stable"), EnemyCreationWizard::NormalizeBlueprintName(TEXT("BP_Goblin")), FString(TEXT("BP_Goblin")));
    TestEqual(TEXT("Prefix casing is canonicalized"), EnemyCreationWizard::NormalizeBlueprintName(TEXT("bp_Goblin")), FString(TEXT("BP_Goblin")));
    TestTrue(TEXT("Normalized safe name passes"), EnemyCreationWizard::IsSafeAssetName(TEXT("BP_Goblin_01")));
    TestFalse(TEXT("Spaces are rejected"), EnemyCreationWizard::IsSafeAssetName(TEXT("BP_Bad Name")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyWizardObjectPathTest,
    "EnemyCreationWizard.Helpers.ObjectPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyWizardObjectPathTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("Package path gains object suffix"),
        EnemyCreationWizard::ToObjectPath(TEXT("/Game/Enemies/BT_TestEnemy")),
        FString(TEXT("/Game/Enemies/BT_TestEnemy.BT_TestEnemy")));
    TestEqual(
        TEXT("Object path remains unchanged"),
        EnemyCreationWizard::ToObjectPath(TEXT("/Game/Enemies/BT_TestEnemy.BT_TestEnemy")),
        FString(TEXT("/Game/Enemies/BT_TestEnemy.BT_TestEnemy")));
    TestEqual(
        TEXT("Export-text wrapper is removed"),
        EnemyCreationWizard::ToObjectPath(TEXT("BehaviorTree'/Game/Enemies/BT_TestEnemy.BT_TestEnemy'")),
        FString(TEXT("/Game/Enemies/BT_TestEnemy.BT_TestEnemy")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyWizardMaterialCountTest,
    "EnemyCreationWizard.Helpers.MaterialSlotCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyWizardMaterialCountTest::RunTest(const FString& Parameters)
{
    FString Error;
    TestTrue(TEXT("Zero-slot meshes accept an empty selection array"), EnemyCreationWizard::ValidateMaterialSelectionCount(0, 0, Error));
    TestTrue(TEXT("Arbitrary slot counts are accepted when exact"), EnemyCreationWizard::ValidateMaterialSelectionCount(14, 14, Error));
    TestFalse(TEXT("Mismatched counts fail"), EnemyCreationWizard::ValidateMaterialSelectionCount(5, 4, Error));
    TestTrue(TEXT("Mismatch explains both counts"), Error.Contains(TEXT("5")) && Error.Contains(TEXT("4")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEnemyWizardReviewSummaryTest,
    "EnemyCreationWizard.Helpers.ReviewSummary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyWizardReviewSummaryTest::RunTest(const FString& Parameters)
{
    EnemyCreationWizard::FReviewData Data;
    Data.EnemyName = TEXT("Goblin");
    Data.BaseDirectory = TEXT("/Game/GeneratedEnemies/");
    Data.bCreateSubFolders = true;
    Data.SkeletalMeshPath = TEXT("/Game/Meshes/SK_Goblin.SK_Goblin");
    Data.MaterialSlotNames = { TEXT("Body"), TEXT("Armor") };
    Data.MaterialPaths = { TEXT("/Game/Materials/M_Body.M_Body"), TEXT("/Game/Materials/M_Armor.M_Armor") };
    Data.Health = 150.0f;
    Data.Damage = 20.0f;
    Data.MovementSpeed = 450.0f;
    Data.EnemyClass = TEXT("Grunt");
    Data.BehaviorTreePath = TEXT("/Game/Blueprints/Enemies/BT_TestEnemy.BT_TestEnemy");

    const FString Summary = EnemyCreationWizard::BuildReviewSummary(Data);
    TestTrue(TEXT("Summary contains normalized resolved output"), Summary.Contains(TEXT("/Game/GeneratedEnemies/Blueprints/BP_Goblin")));
    TestTrue(TEXT("Summary contains imported material slot names"), Summary.Contains(TEXT("[1] Armor")));
    TestTrue(TEXT("Summary contains Behavior Tree"), Summary.Contains(TEXT("BT_TestEnemy")));
    TestTrue(TEXT("Summary contains concrete stat values"), Summary.Contains(TEXT("150")) && Summary.Contains(TEXT("450")));
    return true;
}

#endif
