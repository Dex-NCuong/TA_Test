#include "EnemyCreationWizardLibrary.h"

#include "EnemyCreationWizardHelpers.h"
#include "EnemyCreationWizardWidget.h"
#include "EnemyCreationWizardOpponentComponent.h"
#include "EnemyChaseAIController.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UObject/MetaData.h"
#include "UObject/UnrealType.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "EnemyCreationWizard"

namespace EnemyCreationWizard
{
    static void SetStatus(UEditorUtilityWidget* Wizard, const FString& Message)
    {
        if (UTextBlock* StatusText = Cast<UTextBlock>(Wizard->GetWidgetFromName(TEXT("StatusText"))))
        {
            StatusText->SetText(FText::FromString(Message));
        }
    }

    static void AppendLog(UEditorUtilityWidget* Wizard, const FString& Message)
    {
        UWidget* LogWidget = Wizard->GetWidgetFromName(TEXT("LogOutput"));
        if (UMultiLineEditableTextBox* LogOutput = Cast<UMultiLineEditableTextBox>(LogWidget))
        {
            const FString ExistingLog = LogOutput->GetText().ToString();
            LogOutput->SetText(FText::FromString(ExistingLog.IsEmpty() ? Message : ExistingLog + TEXT("\n") + Message));
        }
        if (UTextBlock* LogOutput = Cast<UTextBlock>(LogWidget))
        {
            const FString ExistingLog = LogOutput->GetText().ToString();
            LogOutput->SetText(FText::FromString(ExistingLog.IsEmpty() ? Message : ExistingLog + TEXT("\n") + Message));
        }
    }

    static void ClearLog(UEditorUtilityWidget* Wizard)
    {
        UWidget* LogWidget = Wizard->GetWidgetFromName(TEXT("LogOutput"));
        if (UMultiLineEditableTextBox* LogOutput = Cast<UMultiLineEditableTextBox>(LogWidget))
        {
            LogOutput->SetText(FText::GetEmpty());
        }
        if (UTextBlock* LogOutput = Cast<UTextBlock>(LogWidget))
        {
            LogOutput->SetText(FText::GetEmpty());
        }
    }

    static void SetProgress(UEditorUtilityWidget* Wizard, const float Progress)
    {
        if (UProgressBar* ProgressBar = Cast<UProgressBar>(Wizard->GetWidgetFromName(TEXT("CreationProgress"))))
        {
            ProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
        }
    }

    static void Report(UEditorUtilityWidget* Wizard, const float Progress, const FString& Message)
    {
        SetProgress(Wizard, Progress);
        SetStatus(Wizard, Message);
        AppendLog(Wizard, Message);
    }

    static void Notify(const FString& Message, const bool bSuccess)
    {
        FNotificationInfo Info(FText::FromString(Message));
        Info.ExpireDuration = bSuccess ? 5.0f : 8.0f;
        Info.bUseLargeFont = false;
        Info.bUseThrobber = false;
        FSlateNotificationManager::Get().AddNotification(Info);
    }

    static FString GetText(UEditorUtilityWidget* Wizard, const FName WidgetName)
    {
        if (UEditableTextBox* TextBox = Cast<UEditableTextBox>(Wizard->GetWidgetFromName(WidgetName)))
        {
            return TextBox->GetText().ToString().TrimStartAndEnd();
        }
        return FString();
    }

    static FString GetSelectedOption(UEditorUtilityWidget* Wizard, const FName WidgetName)
    {
        if (UComboBoxString* ComboBox = Cast<UComboBoxString>(Wizard->GetWidgetFromName(WidgetName)))
        {
            return ComboBox->GetSelectedOption();
        }
        return FString();
    }

    static bool GetChecked(UEditorUtilityWidget* Wizard, const FName WidgetName, const bool bFallback)
    {
        if (UCheckBox* CheckBox = Cast<UCheckBox>(Wizard->GetWidgetFromName(WidgetName)))
        {
            return CheckBox->IsChecked();
        }
        return bFallback;
    }

    static float GetNumber(UEditorUtilityWidget* Wizard, const FName WidgetName, const float Fallback)
    {
        if (USpinBox* SpinBox = Cast<USpinBox>(Wizard->GetWidgetFromName(WidgetName)))
        {
            return SpinBox->GetValue();
        }
        return Fallback;
    }


    static bool GetRequiredObjectPath(UEditorUtilityWidget* Wizard, const FName WidgetName, const FString& Label, FString& OutObjectPath, FString& OutError)
    {
        const FString RawPath = GetText(Wizard, WidgetName);
        if (RawPath.IsEmpty())
        {
            OutError = FString::Printf(TEXT("%s is required."), *Label);
            return false;
        }

        OutObjectPath = ToObjectPath(RawPath);
        if (!OutObjectPath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidObjectPath(OutObjectPath))
        {
            OutError = FString::Printf(TEXT("%s must be a valid /Game/ asset path."), *Label);
            return false;
        }
        return true;
    }

    static bool GetOptionalObjectPath(UEditorUtilityWidget* Wizard, const FName WidgetName, const FString& Label, FString& OutObjectPath, FString& OutError)
    {
        const FString RawPath = GetText(Wizard, WidgetName);
        if (RawPath.IsEmpty())
        {
            OutObjectPath.Reset();
            return true;
        }

        OutObjectPath = ToObjectPath(RawPath);
        if (!OutObjectPath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidObjectPath(OutObjectPath))
        {
            OutError = FString::Printf(TEXT("%s must be a valid /Game/ asset path."), *Label);
            return false;
        }
        return true;
    }
    static bool ParseGameplayTags(const FString& Source, FString& OutNormalizedTags, FString& OutError)
    {
        TArray<FString> Values;
        Source.ParseIntoArray(Values, TEXT(","), true);
        TArray<FString> ValidTags;

        for (FString Value : Values)
        {
            Value = Value.TrimStartAndEnd();
            if (Value.IsEmpty())
            {
                continue;
            }

            const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Value), false);
            if (!Tag.IsValid())
            {
                OutError = FString::Printf(TEXT("Gameplay Tag '%s' is not registered."), *Value);
                return false;
            }
            ValidTags.Add(Tag.ToString());
        }

        OutNormalizedTags = FString::Join(ValidTags, TEXT(","));
        return true;
    }

    static void WriteMetadata(UBlueprint* Blueprint, const FName Key, const FString& Value)
    {
        if (Blueprint && Blueprint->GetOutermost())
        {
            Blueprint->GetOutermost()->GetMetaData().SetValue(Blueprint, Key, *Value);
        }
    }

    static UBlueprint* CreateBlueprintAsset(const FString& PackageName, const FString& AssetName, UClass* ParentClass)
    {
        UPackage* Package = CreatePackage(*PackageName);
        if (!Package)
        {
            return nullptr;
        }

        UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
            ParentClass,
            Package,
            FName(*AssetName),
            BPTYPE_Normal,
            FName(TEXT("EnemyCreationWizard")));

        if (Blueprint)
        {
            FAssetRegistryModule::AssetCreated(Blueprint);
            Blueprint->MarkPackageDirty();
        }
        return Blueprint;
    }

    static void DiscardUnsavedBlueprint(UBlueprint*& Blueprint)
    {
        if (!Blueprint)
        {
            return;
        }

        // No save occurs before final validation/compile, so this removes only a transient failed result.
        FAssetRegistryModule::AssetDeleted(Blueprint);
        if (UPackage* Package = Blueprint->GetOutermost())
        {
            Package->SetDirtyFlag(false);
        }
        Blueprint->ClearFlags(RF_Public | RF_Standalone);
        Blueprint->MarkAsGarbage();
        Blueprint = nullptr;
    }
    static bool EnsureContentFolder(const FString& LongPackagePath)
    {
        if (!FPackageName::IsValidLongPackageName(LongPackagePath, false))
        {
            return false;
        }

        const FString DiskPath = FPackageName::LongPackageNameToFilename(LongPackagePath, TEXT(""));
        return IFileManager::Get().MakeDirectory(*DiskPath, true);
    }

    static bool SetNumericDefault(UObject* Object, const FName PropertyName, const double Value)
    {
        if (FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName))
        {
            FloatProperty->SetPropertyValue_InContainer(Object, static_cast<float>(Value));
            return true;
        }
        if (FDoubleProperty* DoubleProperty = FindFProperty<FDoubleProperty>(Object->GetClass(), PropertyName))
        {
            DoubleProperty->SetPropertyValue_InContainer(Object, Value);
            return true;
        }
        return false;
    }

    static bool SetStringDefault(UObject* Object, const FName PropertyName, const FString& Value)
    {
        if (FStrProperty* StringProperty = FindFProperty<FStrProperty>(Object->GetClass(), PropertyName))
        {
            StringProperty->SetPropertyValue_InContainer(Object, Value);
            return true;
        }
        if (FNameProperty* NameProperty = FindFProperty<FNameProperty>(Object->GetClass(), PropertyName))
        {
            NameProperty->SetPropertyValue_InContainer(Object, FName(*Value));
            return true;
        }
        return false;
    }

    static bool SetObjectDefault(UObject* Object, const FName PropertyName, UObject* Value)
    {
        if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
        {
            if (!Value || Value->IsA(ObjectProperty->PropertyClass))
            {
                ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
                return true;
            }
        }
        return false;
    }

    static bool IsFinitePositive(const float Value)
    {
        return FMath::IsFinite(Value) && Value > 0.0f;
    }

    static bool IsFiniteNonNegative(const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f;
    }

    static bool SetEnemyClassDefault(UObject* Object, const FString& Value)
    {
        if (SetStringDefault(Object, TEXT("EnemyClass"), Value))
        {
            return true;
        }

        if (FEnumProperty* EnumProperty = FindFProperty<FEnumProperty>(Object->GetClass(), TEXT("EnemyClass")))
        {
            const int64 EnumValue = EnumProperty->GetEnum()->GetValueByNameString(Value);
            if (EnumValue != INDEX_NONE)
            {
                EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(Object), EnumValue);
                return true;
            }
        }
        if (FByteProperty* ByteProperty = FindFProperty<FByteProperty>(Object->GetClass(), TEXT("EnemyClass")))
        {
            if (UEnum* Enum = ByteProperty->Enum)
            {
                const int64 EnumValue = Enum->GetValueByNameString(Value);
                if (EnumValue != INDEX_NONE)
                {
                    ByteProperty->SetPropertyValue_InContainer(Object, static_cast<uint8>(EnumValue));
                    return true;
                }
            }
        }
        return false;
    }

    static bool CanSetObjectDefault(const UObject* Object, const FName PropertyName, const UClass* ValueClass)
    {
        const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName);
        return Property && Property->PropertyClass && ValueClass->IsChildOf(Property->PropertyClass);
    }

    static bool ValidateBaseEnemyContract(ACharacter* BaseEnemyCDO, FString& OutError)
    {
        if (!BaseEnemyCDO || !BaseEnemyCDO->GetMesh())
        {
            OutError = TEXT("BP_BaseEnemy must derive from Character and provide a Skeletal Mesh component.");
            return false;
        }
        if (!FindFProperty<FFloatProperty>(BaseEnemyCDO->GetClass(), TEXT("Health")) ||
            !FindFProperty<FFloatProperty>(BaseEnemyCDO->GetClass(), TEXT("Damage")) ||
            !FindFProperty<FFloatProperty>(BaseEnemyCDO->GetClass(), TEXT("MovementSpeed")))
        {
            OutError = TEXT("BP_BaseEnemy must expose float Health, Damage, and MovementSpeed variables.");
            return false;
        }
        if (!CanSetObjectDefault(BaseEnemyCDO, TEXT("SkeletalMesh"), USkeletalMesh::StaticClass()))
        {
            OutError = TEXT("BP_BaseEnemy must expose a SkeletalMesh default variable.");
            return false;
        }
        if (!CanSetObjectDefault(BaseEnemyCDO, TEXT("BehaviorTreeAsset"), UBehaviorTree::StaticClass()) &&
            !CanSetObjectDefault(BaseEnemyCDO, TEXT("BehaviorTree"), UBehaviorTree::StaticClass()))
        {
            OutError = TEXT("BP_BaseEnemy must expose BehaviorTreeAsset or the legacy BehaviorTree variable.");
            return false;
        }
        return true;
    }

    static bool ApplyMaterialDefaults(ACharacter* EnemyCDO, const TArray<UMaterialInterface*>& Materials)
    {
        if (!EnemyCDO || !EnemyCDO->GetMesh() || EnemyCDO->GetMesh()->GetNumMaterials() < Materials.Num())
        {
            return false;
        }

        USkeletalMeshComponent* MeshComponent = EnemyCDO->GetMesh();
        for (int32 SlotIndex = 0; SlotIndex < Materials.Num(); ++SlotIndex)
        {
            MeshComponent->SetMaterial(SlotIndex, Materials[SlotIndex]);
        }
        return true;
    }

}

bool UEnemyCreationWizardLibrary::CreateEnemyFromWizard(UEditorUtilityWidget* Wizard)
{
    using namespace EnemyCreationWizard;

    if (!Wizard)
    {
        return false;
    }

    ClearLog(Wizard);
    auto Fail = [Wizard](const FString& Message)
    {
        Report(Wizard, 0.0f, Message);
        Notify(Message, false);
        return false;
    };

    FScopedSlowTask ProgressTask(100.0f, LOCTEXT("GeneratingEnemy", "Generating enemy Blueprint..."));
    ProgressTask.MakeDialogDelayed(0.5f);
    Report(Wizard, 0.05f, TEXT("Validating wizard configuration..."));
    ProgressTask.EnterProgressFrame(10.0f);

    const UEnemyCreationWizardWidget* NativeWizard = Cast<UEnemyCreationWizardWidget>(Wizard);
    const FString EnteredName = GetText(Wizard, TEXT("EnemyNameInput"));
    const FString AssetName = NormalizeBlueprintName(EnteredName);
    if (AssetName.Len() <= 3)
    {
        return Fail(TEXT("Enemy Name must contain characters after the BP_ prefix."));
    }
    if (!IsSafeAssetName(AssetName))
    {
        return Fail(TEXT("Enemy Name must begin with a letter and use only letters, digits, or underscores."));
    }

    const FString BaseDirectory = NativeWizard
        ? NativeWizard->GetBaseDirectory()
        : NormalizeBaseDirectory(GetText(Wizard, TEXT("BaseDirectoryInput")));
    if (!BaseDirectory.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(BaseDirectory, false))
    {
        return Fail(TEXT("Base directory must be a valid /Game/ content path."));
    }

    const bool bCreateSubFolders = GetChecked(Wizard, TEXT("CreateSubFoldersCheckBox"), true);
    const FString BlueprintDirectory = bCreateSubFolders ? BaseDirectory / TEXT("Blueprints") : BaseDirectory;
    const FString MeshDirectory = BaseDirectory / TEXT("Meshes");
    const FString MaterialsDirectory = BaseDirectory / TEXT("Materials");
    const FString AIDirectory = bCreateSubFolders ? BaseDirectory / TEXT("AI") : BaseDirectory;

    const FString EnemyClass = GetSelectedOption(Wizard, TEXT("EnemyClassCombo"));
    static const TSet<FString> ValidEnemyClasses = { TEXT("Grunt"), TEXT("Scout"), TEXT("Brute"), TEXT("Boss") };
    if (!ValidEnemyClasses.Contains(EnemyClass))
    {
        return Fail(TEXT("Enemy Class must be Grunt, Scout, Brute, or Boss."));
    }

    const float Health = GetNumber(Wizard, TEXT("HealthSpinBox"), -1.0f);
    const float Damage = GetNumber(Wizard, TEXT("DamageSpinBox"), -1.0f);
    const float MovementSpeed = GetNumber(Wizard, TEXT("MovementSpeedSpinBox"), -1.0f);
    if (!IsFinitePositive(Health))
    {
        return Fail(TEXT("Health must be a finite value greater than zero."));
    }
    if (!IsFiniteNonNegative(Damage))
    {
        return Fail(TEXT("Damage must be a finite value greater than or equal to zero."));
    }
    if (!IsFinitePositive(MovementSpeed))
    {
        return Fail(TEXT("Movement Speed must be a finite value greater than zero."));
    }

    FString Error;
    FString MeshPath;
    USkeletalMesh* SkeletalMesh = nullptr;
    if (NativeWizard)
    {
        SkeletalMesh = NativeWizard->GetSelectedSkeletalMesh();
        MeshPath = SkeletalMesh ? SkeletalMesh->GetPathName() : FString();
        if (!SkeletalMesh)
        {
            return Fail(TEXT("Skeletal Mesh is required."));
        }
    }
    else
    {
        if (!GetRequiredObjectPath(Wizard, TEXT("SkeletalMeshInput"), TEXT("Skeletal Mesh"), MeshPath, Error))
        {
            return Fail(Error);
        }
        SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath);
        if (!SkeletalMesh)
        {
            return Fail(TEXT("Skeletal Mesh path is invalid or does not reference a Skeletal Mesh."));
        }
    }

    const int32 MaterialSlotCount = SkeletalMesh->GetMaterials().Num();
    TArray<FString> MaterialSlotNames;
    MaterialSlotNames.Reserve(MaterialSlotCount);
    for (int32 Index = 0; Index < MaterialSlotCount; ++Index)
    {
        const FName ImportedName = SkeletalMesh->GetMaterials()[Index].ImportedMaterialSlotName;
        MaterialSlotNames.Add(ImportedName.IsNone() ? FString::Printf(TEXT("Slot %d"), Index) : ImportedName.ToString());
    }

    TArray<UMaterialInterface*> Materials;
    if (NativeWizard)
    {
        NativeWizard->GetSelectedMaterials(Materials);
        if (!ValidateMaterialSelectionCount(MaterialSlotCount, Materials.Num(), Error))
        {
            return Fail(Error);
        }
    }
    else
    {
        if (MaterialSlotCount > 4)
        {
            return Fail(TEXT("This mesh requires the native Enemy Creation Wizard integration because it has more than four material slots."));
        }

        const TArray<FName> MaterialControls = { TEXT("BodyMaterialInput"), TEXT("HeadMaterialInput"), TEXT("ArmorMaterialInput"), TEXT("WeaponMaterialInput") };
        const TArray<FString> MaterialLabels = { TEXT("Body Material"), TEXT("Head Material"), TEXT("Armor Material"), TEXT("Weapon Material") };
        Materials.Reserve(MaterialSlotCount);
        for (int32 Index = 0; Index < MaterialSlotCount; ++Index)
        {
            FString MaterialPath;
            if (!GetRequiredObjectPath(Wizard, MaterialControls[Index], MaterialLabels[Index], MaterialPath, Error))
            {
                return Fail(Error);
            }
            Materials.Add(LoadObject<UMaterialInterface>(nullptr, *MaterialPath));
        }
    }

    for (int32 Index = 0; Index < Materials.Num(); ++Index)
    {
        if (!Materials[Index])
        {
            const FString SlotName = MaterialSlotNames.IsValidIndex(Index) ? MaterialSlotNames[Index] : FString::Printf(TEXT("Slot %d"), Index);
            return Fail(FString::Printf(TEXT("Material slot [%d] %s requires a valid Material Interface."), Index, *SlotName));
        }
    }

    FString BehaviorTreePath;
    UBehaviorTree* BehaviorTree = nullptr;
    if (NativeWizard)
    {
        BehaviorTree = NativeWizard->GetSelectedBehaviorTree();
        BehaviorTreePath = BehaviorTree ? BehaviorTree->GetPathName() : FString();
        if (!BehaviorTree)
        {
            return Fail(TEXT("Behavior Tree is required."));
        }
    }
    else
    {
        if (!GetRequiredObjectPath(Wizard, TEXT("BehaviorTreeInput"), TEXT("Behavior Tree"), BehaviorTreePath, Error))
        {
            return Fail(Error);
        }
        BehaviorTree = LoadObject<UBehaviorTree>(nullptr, *BehaviorTreePath);
        if (!BehaviorTree)
        {
            return Fail(TEXT("Behavior Tree path is invalid or does not reference a Behavior Tree."));
        }
    }

    FString AnimBlueprintPath;
    if (!GetOptionalObjectPath(Wizard, TEXT("AnimationBlueprintInput"), TEXT("Animation Blueprint"), AnimBlueprintPath, Error))
    {
        return Fail(Error);
    }
    UBlueprint* AnimationBlueprint = nullptr;
    if (!AnimBlueprintPath.IsEmpty())
    {
        AnimationBlueprint = LoadObject<UBlueprint>(nullptr, *AnimBlueprintPath);
        if (!AnimationBlueprint || !AnimationBlueprint->GeneratedClass || !AnimationBlueprint->GeneratedClass->IsChildOf(UAnimInstance::StaticClass()))
        {
            return Fail(TEXT("Animation Blueprint path is invalid or is not an AnimInstance Blueprint."));
        }
    }

    FString DataAssetPath;
    if (!GetOptionalObjectPath(Wizard, TEXT("DataAssetInput"), TEXT("Data Asset"), DataAssetPath, Error))
    {
        return Fail(Error);
    }
    UObject* DataAsset = DataAssetPath.IsEmpty() ? nullptr : StaticLoadObject(UObject::StaticClass(), nullptr, *DataAssetPath);
    if (!DataAssetPath.IsEmpty() && !DataAsset)
    {
        return Fail(TEXT("Data Asset path could not be loaded."));
    }

    FString NormalizedTags;
    if (!ParseGameplayTags(GetText(Wizard, TEXT("GameplayTagsInput")), NormalizedTags, Error))
    {
        return Fail(Error);
    }

    const FString AIType = GetSelectedOption(Wizard, TEXT("AITypeCombo"));
    const bool bGenerateAI = GetChecked(Wizard, TEXT("GenerateAICheckBox"), false);

    UBlueprint* BaseEnemy = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Blueprints/Enemies/BP_BaseEnemy.BP_BaseEnemy"));
    ACharacter* BaseEnemyCDO = BaseEnemy && BaseEnemy->GeneratedClass ? Cast<ACharacter>(BaseEnemy->GeneratedClass->GetDefaultObject()) : nullptr;
    if (!BaseEnemy || !BaseEnemy->GeneratedClass || !ValidateBaseEnemyContract(BaseEnemyCDO, Error))
    {
        return Fail(Error.IsEmpty() ? TEXT("BP_BaseEnemy could not be loaded.") : Error);
    }

    const FString PackageName = BlueprintDirectory / AssetName;
    if (FPackageName::DoesPackageExist(PackageName))
    {
        return Fail(FString::Printf(TEXT("An asset already exists at %s."), *PackageName));
    }

    const FString AIAssetName = TEXT("BP_AI_") + AssetName.RightChop(3);
    const FString AIPackageName = AIDirectory / AIAssetName;
    if (bGenerateAI && FPackageName::DoesPackageExist(AIPackageName))
    {
        return Fail(FString::Printf(TEXT("An AI Controller asset already exists at %s."), *AIPackageName));
    }

    // All validation is complete before folders/assets are created to prevent avoidable partial results.
    Report(Wizard, 0.18f, FString::Printf(TEXT("Creating base content directory %s..."), *BaseDirectory));
    ProgressTask.EnterProgressFrame(8.0f);
    if (!EnsureContentFolder(BaseDirectory))
    {
        return Fail(TEXT("Could not create the requested base content directory."));
    }

    if (bCreateSubFolders)
    {
        Report(Wizard, 0.24f, TEXT("Creating standard sub-folders: Blueprints, Meshes, Materials, and AI..."));
        ProgressTask.EnterProgressFrame(7.0f);
        if (!EnsureContentFolder(BlueprintDirectory) ||
            !EnsureContentFolder(MeshDirectory) ||
            !EnsureContentFolder(MaterialsDirectory) ||
            !EnsureContentFolder(AIDirectory))
        {
            return Fail(TEXT("Could not create the requested content folder structure."));
        }
    }

    Report(Wizard, 0.30f, FString::Printf(TEXT("Creating child Blueprint %s..."), *AssetName));
    ProgressTask.EnterProgressFrame(25.0f);
    UBlueprint* EnemyBlueprint = CreateBlueprintAsset(PackageName, AssetName, BaseEnemy->GeneratedClass);
    if (!EnemyBlueprint)
    {
        return Fail(TEXT("Failed to create the enemy Blueprint asset."));
    }

    auto FailAfterEnemyCreation = [&Fail, &EnemyBlueprint](const FString& Message)
    {
        DiscardUnsavedBlueprint(EnemyBlueprint);
        return Fail(Message);
    };

    FKismetEditorUtilities::CompileBlueprint(EnemyBlueprint);
    if (EnemyBlueprint->Status == BS_Error)
    {
        return FailAfterEnemyCreation(TEXT("The generated enemy Blueprint failed to compile."));
    }

    ACharacter* EnemyCDO = EnemyBlueprint->GeneratedClass ? Cast<ACharacter>(EnemyBlueprint->GeneratedClass->GetDefaultObject()) : nullptr;
    if (!EnemyCDO || !EnemyCDO->GetMesh())
    {
        return FailAfterEnemyCreation(TEXT("The generated enemy Blueprint has no Skeletal Mesh component."));
    }

    Report(Wizard, 0.55f, TEXT("Applying mesh, materials, stats, and AI defaults..."));
    ProgressTask.EnterProgressFrame(30.0f);
    EnemyCDO->Modify();
    EnemyCDO->GetMesh()->SetSkeletalMesh(SkeletalMesh);
    if (AnimationBlueprint)
    {
        EnemyCDO->GetMesh()->SetAnimInstanceClass(AnimationBlueprint->GeneratedClass);
    }

    if (!ApplyMaterialDefaults(EnemyCDO, Materials) ||
        !SetNumericDefault(EnemyCDO, TEXT("Health"), Health) ||
        !SetNumericDefault(EnemyCDO, TEXT("Damage"), Damage) ||
        !SetNumericDefault(EnemyCDO, TEXT("MovementSpeed"), MovementSpeed) ||
        !SetEnemyClassDefault(EnemyCDO, EnemyClass) ||
        !SetObjectDefault(EnemyCDO, TEXT("SkeletalMesh"), SkeletalMesh))
    {
        return FailAfterEnemyCreation(TEXT("BP_BaseEnemy no longer matches the required default-property contract."));
    }

    // Named material variables are legacy conveniences only. The component overrides above are authoritative.
    const TArray<FName> MaterialProperties = { TEXT("BodyMaterial"), TEXT("HeadMaterial"), TEXT("ArmorMaterial"), TEXT("WeaponMaterial") };
    for (int32 Index = 0; Index < FMath::Min(Materials.Num(), MaterialProperties.Num()); ++Index)
    {
        if (FindFProperty<FObjectPropertyBase>(EnemyCDO->GetClass(), MaterialProperties[Index]))
        {
            SetObjectDefault(EnemyCDO, MaterialProperties[Index], Materials[Index]);
        }
    }

    // Prefer the requested variable name, while preserving compatibility with the current BP_BaseEnemy.
    if (!SetObjectDefault(EnemyCDO, TEXT("BehaviorTreeAsset"), BehaviorTree) &&
        !SetObjectDefault(EnemyCDO, TEXT("BehaviorTree"), BehaviorTree))
    {
        return FailAfterEnemyCreation(TEXT("Could not assign the required Behavior Tree to BP_BaseEnemy."));
    }

    if (UCharacterMovementComponent* MovementComponent = EnemyCDO->GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = MovementSpeed;
    }

    WriteMetadata(EnemyBlueprint, TEXT("EnemyClass"), EnemyClass);
    WriteMetadata(EnemyBlueprint, TEXT("AIType"), AIType);
    WriteMetadata(EnemyBlueprint, TEXT("GameplayTags"), NormalizedTags);
    WriteMetadata(EnemyBlueprint, TEXT("SkeletalMesh"), MeshPath);
    WriteMetadata(EnemyBlueprint, TEXT("BehaviorTree"), BehaviorTreePath);
    TArray<FString> MaterialMetadata;
    MaterialMetadata.Reserve(Materials.Num());
    for (int32 Index = 0; Index < Materials.Num(); ++Index)
    {
        const FString SlotName = MaterialSlotNames.IsValidIndex(Index) ? MaterialSlotNames[Index] : FString::Printf(TEXT("Slot %d"), Index);
        MaterialMetadata.Add(FString::Printf(TEXT("[%d] %s=%s"), Index, *SlotName, *Materials[Index]->GetPathName()));
        WriteMetadata(EnemyBlueprint, FName(*FString::Printf(TEXT("MaterialSlot_%d"), Index)), Materials[Index]->GetPathName());
    }
    WriteMetadata(EnemyBlueprint, TEXT("MaterialSlotCount"), FString::FromInt(Materials.Num()));
    WriteMetadata(EnemyBlueprint, TEXT("Materials"), FString::Join(MaterialMetadata, TEXT("|")));
    WriteMetadata(EnemyBlueprint, TEXT("AnimationBlueprint"), AnimBlueprintPath);
    WriteMetadata(EnemyBlueprint, TEXT("DataAsset"), DataAsset ? DataAsset->GetPathName() : FString());
    WriteMetadata(EnemyBlueprint, TEXT("Health"), FString::SanitizeFloat(Health));
    WriteMetadata(EnemyBlueprint, TEXT("Damage"), FString::SanitizeFloat(Damage));
    WriteMetadata(EnemyBlueprint, TEXT("MovementSpeed"), FString::SanitizeFloat(MovementSpeed));
    WriteMetadata(EnemyBlueprint, TEXT("GenerateAI"), bGenerateAI ? TEXT("true") : TEXT("false"));
    EnemyBlueprint->MarkPackageDirty();

    UBlueprint* AIControllerBlueprint = nullptr;
    if (bGenerateAI)
    {
        Report(Wizard, 0.75f, TEXT("Creating optional AI Controller..."));
        ProgressTask.EnterProgressFrame(15.0f);
        AIControllerBlueprint = CreateBlueprintAsset(AIPackageName, AIAssetName, AEnemyChaseAIController::StaticClass());
        if (!AIControllerBlueprint || !AIControllerBlueprint->GeneratedClass)
        {
            DiscardUnsavedBlueprint(AIControllerBlueprint);
            return FailAfterEnemyCreation(TEXT("Failed to create the optional AI Controller Blueprint."));
        }

        if (FClassProperty* AIControllerProperty = FindFProperty<FClassProperty>(EnemyCDO->GetClass(), TEXT("AIControllerClass")))
        {
            AIControllerProperty->SetPropertyValue_InContainer(EnemyCDO, AIControllerBlueprint->GeneratedClass);
        }
        WriteMetadata(EnemyBlueprint, TEXT("AIController"), AIControllerBlueprint->GetPathName());
        AIControllerBlueprint->MarkPackageDirty();
    }

    Report(Wizard, 0.90f, TEXT("Compiling and saving generated assets..."));
    ProgressTask.EnterProgressFrame(20.0f);
    FKismetEditorUtilities::CompileBlueprint(EnemyBlueprint);
    if (EnemyBlueprint->Status == BS_Error)
    {
        DiscardUnsavedBlueprint(AIControllerBlueprint);
        return FailAfterEnemyCreation(TEXT("The generated enemy Blueprint failed to compile after defaults were applied."));
    }

    UEditorAssetSubsystem* AssetSubsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
    if (!AssetSubsystem)
    {
        DiscardUnsavedBlueprint(AIControllerBlueprint);
        return FailAfterEnemyCreation(TEXT("Editor Asset Subsystem is unavailable; no generated asset was saved."));
    }

    TArray<UObject*> AssetsToSave;
    AssetsToSave.Add(EnemyBlueprint);
    if (AIControllerBlueprint)
    {
        AssetsToSave.Add(AIControllerBlueprint);
    }
    if (!AssetSubsystem->SaveLoadedAssets(AssetsToSave, false))
    {
        return Fail(TEXT("Saving generated assets failed. Review the Content Browser for any assets saved before the editor reported the failure."));
    }

    FString SuccessMessage = AIControllerBlueprint
        ? FString::Printf(TEXT("Created and saved %s plus %s in %s."), *AssetName, *AIControllerBlueprint->GetName(), *BaseDirectory)
        : FString::Printf(TEXT("Created and saved %s in %s."), *AssetName, *BaseDirectory);

    // Select generated asset in Content Browser for immediate visual feedback
    if (GEditor)
    {
        TArray<FAssetData> AssetsToSelect;
        AssetsToSelect.Add(FAssetData(EnemyBlueprint));
        GEditor->SyncBrowserToObjects(AssetsToSelect);
    }

    Report(Wizard, 1.0f, SuccessMessage);
    Notify(SuccessMessage, true);
    return true;
}
#undef LOCTEXT_NAMESPACE
