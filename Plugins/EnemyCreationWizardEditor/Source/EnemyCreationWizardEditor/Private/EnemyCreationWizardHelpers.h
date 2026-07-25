#pragma once

#include "CoreMinimal.h"

namespace EnemyCreationWizard
{
    struct FReviewData
    {
        FString EnemyName;
        FString BaseDirectory;
        bool bCreateSubFolders = true;
        FString SkeletalMeshPath;
        TArray<FString> MaterialSlotNames;
        TArray<FString> MaterialPaths;
        float Health = 0.0f;
        float Damage = 0.0f;
        float MovementSpeed = 0.0f;
        FString EnemyClass;
        FString BehaviorTreePath;
        FString AnimationBlueprintPath;
        FString DataAssetPath;
        FString GameplayTags;
        FString AIType;
        bool bGenerateAI = false;
    };

    ENEMYCREATIONWIZARDEDITOR_API FString NormalizeBlueprintName(const FString& Value);
    ENEMYCREATIONWIZARDEDITOR_API bool IsSafeAssetName(const FString& Name);
    ENEMYCREATIONWIZARDEDITOR_API FString NormalizeBaseDirectory(const FString& Value);
    ENEMYCREATIONWIZARDEDITOR_API FString ToObjectPath(const FString& AssetPath);
    ENEMYCREATIONWIZARDEDITOR_API bool ValidateMaterialSelectionCount(int32 MaterialSlotCount, int32 SelectionCount, FString& OutError);
    ENEMYCREATIONWIZARDEDITOR_API FString BuildReviewSummary(const FReviewData& Data);
}
