#include "EnemyCreationWizardHelpers.h"

#include "Misc/PackageName.h"

namespace EnemyCreationWizard
{
    FString NormalizeBlueprintName(const FString& Value)
    {
        FString Normalized = Value.TrimStartAndEnd();
        if (Normalized.IsEmpty())
        {
            return FString();
        }

        if (Normalized.StartsWith(TEXT("BP_"), ESearchCase::IgnoreCase))
        {
            Normalized.RightChopInline(3, EAllowShrinking::No);
        }
        return TEXT("BP_") + Normalized;
    }

    bool IsSafeAssetName(const FString& Name)
    {
        if (Name.IsEmpty() || !FChar::IsAlpha(Name[0]))
        {
            return false;
        }

        for (const TCHAR Character : Name)
        {
            if (!FChar::IsAlnum(Character) && Character != TCHAR('_'))
            {
                return false;
            }
        }
        return true;
    }

    FString NormalizeBaseDirectory(const FString& Value)
    {
        FString Directory = Value.TrimStartAndEnd();
        if (Directory.IsEmpty())
        {
            Directory = TEXT("/Game/Blueprints/Enemies");
        }
        while (Directory.EndsWith(TEXT("/")))
        {
            Directory.LeftChopInline(1);
        }
        return Directory;
    }

    FString ToObjectPath(const FString& AssetPath)
    {
        FString Path = AssetPath.TrimStartAndEnd();
        if (Path.IsEmpty())
        {
            return FString();
        }

        const int32 FirstQuote = Path.Find(TEXT("'"));
        const int32 LastQuote = Path.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (FirstQuote != INDEX_NONE && LastQuote > FirstQuote)
        {
            Path = Path.Mid(FirstQuote + 1, LastQuote - FirstQuote - 1);
        }

        const int32 DotIndex = Path.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        const int32 LastSlashIndex = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (DotIndex != INDEX_NONE && DotIndex > LastSlashIndex)
        {
            return Path;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(Path);
        return AssetName.IsEmpty() ? FString() : Path + TEXT(".") + AssetName;
    }

    bool ValidateMaterialSelectionCount(const int32 MaterialSlotCount, const int32 SelectionCount, FString& OutError)
    {
        if (MaterialSlotCount < 0 || SelectionCount < 0)
        {
            OutError = TEXT("Material slot and selection counts cannot be negative.");
            return false;
        }
        if (MaterialSlotCount != SelectionCount)
        {
            OutError = FString::Printf(
                TEXT("The selected Skeletal Mesh exposes %d material slot(s), but the wizard contains %d material selection(s). Re-select the mesh to rebuild the slot list."),
                MaterialSlotCount,
                SelectionCount);
            return false;
        }
        OutError.Reset();
        return true;
    }

    FString BuildReviewSummary(const FReviewData& Data)
    {
        const FString AssetName = NormalizeBlueprintName(Data.EnemyName);
        const FString BaseDirectory = NormalizeBaseDirectory(Data.BaseDirectory);
        const FString BlueprintDirectory = Data.bCreateSubFolders ? BaseDirectory / TEXT("Blueprints") : BaseDirectory;
        const FString AIDirectory = Data.bCreateSubFolders ? BaseDirectory / TEXT("AI") : BaseDirectory;
        const FString BlueprintPath = AssetName.IsEmpty() ? TEXT("<invalid name>") : BlueprintDirectory / AssetName;
        const FString AIAssetName = AssetName.Len() > 3 ? TEXT("BP_AI_") + AssetName.RightChop(3) : TEXT("<invalid name>");
        const FString AIPath = Data.bGenerateAI && AIAssetName != TEXT("<invalid name>") ? AIDirectory / AIAssetName : TEXT("Not requested");

        TArray<FString> Lines;
        Lines.Reserve(24 + Data.MaterialPaths.Num());
        Lines.Add(TEXT("IDENTITY & OUTPUT"));
        Lines.Add(FString::Printf(TEXT("  Enemy name: %s"), Data.EnemyName.IsEmpty() ? TEXT("<empty>") : *Data.EnemyName));
        Lines.Add(FString::Printf(TEXT("  Normalized Blueprint name: %s"), AssetName.IsEmpty() ? TEXT("<invalid>") : *AssetName));
        Lines.Add(FString::Printf(TEXT("  Base directory: %s"), *BaseDirectory));
        Lines.Add(FString::Printf(TEXT("  Create sub-folders: %s"), Data.bCreateSubFolders ? TEXT("Yes") : TEXT("No")));
        Lines.Add(FString::Printf(TEXT("  Enemy Blueprint output: %s"), *BlueprintPath));
        Lines.Add(FString::Printf(TEXT("  AI Controller output: %s"), *AIPath));
        Lines.Add(TEXT(""));

        Lines.Add(TEXT("VISUALS"));
        Lines.Add(FString::Printf(TEXT("  Skeletal Mesh: %s"), Data.SkeletalMeshPath.IsEmpty() ? TEXT("<not selected>") : *Data.SkeletalMeshPath));
        Lines.Add(FString::Printf(TEXT("  Material slots: %d"), Data.MaterialPaths.Num()));
        for (int32 Index = 0; Index < Data.MaterialPaths.Num(); ++Index)
        {
            const FString SlotName = Data.MaterialSlotNames.IsValidIndex(Index) && !Data.MaterialSlotNames[Index].IsEmpty()
                ? Data.MaterialSlotNames[Index]
                : FString::Printf(TEXT("Slot %d"), Index);
            const FString& Path = Data.MaterialPaths[Index];
            Lines.Add(FString::Printf(TEXT("    [%d] %s: %s"), Index, *SlotName, Path.IsEmpty() ? TEXT("<not selected>") : *Path));
        }
        if (Data.MaterialPaths.IsEmpty())
        {
            Lines.Add(TEXT("    No material overrides required by this mesh."));
        }
        Lines.Add(TEXT(""));

        Lines.Add(TEXT("STATS"));
        Lines.Add(FString::Printf(TEXT("  Health: %s"), *FString::SanitizeFloat(Data.Health)));
        Lines.Add(FString::Printf(TEXT("  Damage: %s"), *FString::SanitizeFloat(Data.Damage)));
        Lines.Add(FString::Printf(TEXT("  Movement Speed: %s"), *FString::SanitizeFloat(Data.MovementSpeed)));
        Lines.Add(FString::Printf(TEXT("  Enemy Class: %s"), Data.EnemyClass.IsEmpty() ? TEXT("<not selected>") : *Data.EnemyClass));
        Lines.Add(TEXT(""));

        Lines.Add(TEXT("AI & OPTIONAL DATA"));
        Lines.Add(FString::Printf(TEXT("  Behavior Tree: %s"), Data.BehaviorTreePath.IsEmpty() ? TEXT("<not selected>") : *Data.BehaviorTreePath));
        Lines.Add(FString::Printf(TEXT("  AI Type: %s"), Data.AIType.IsEmpty() ? TEXT("<not configured>") : *Data.AIType));
        Lines.Add(FString::Printf(TEXT("  Generate AI Controller: %s"), Data.bGenerateAI ? TEXT("Yes") : TEXT("No")));
        Lines.Add(FString::Printf(TEXT("  Animation Blueprint: %s"), Data.AnimationBlueprintPath.IsEmpty() ? TEXT("<none>") : *Data.AnimationBlueprintPath));
        Lines.Add(FString::Printf(TEXT("  Data Asset: %s"), Data.DataAssetPath.IsEmpty() ? TEXT("<none>") : *Data.DataAssetPath));
        Lines.Add(FString::Printf(TEXT("  Gameplay Tags: %s"), Data.GameplayTags.IsEmpty() ? TEXT("<none>") : *Data.GameplayTags));

        return FString::Join(Lines, TEXT("\n"));
    }
}

