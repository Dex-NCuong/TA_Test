#include "EnemyCreationWizardWidget.h"

#include "AssetRegistry/AssetData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/MultiLineEditableTextBox.h"
#include "Components/NativeWidgetHost.h"
#include "Components/PanelWidget.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "ContentBrowserModule.h"
#include "EnemyCreationWizardHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "IContentBrowserSingleton.h"
#include "Materials/MaterialInterface.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EnemyCreationWizardWidget"

namespace
{
    constexpr const TCHAR* UnusedLegacyMaterialValue = TEXT("__UNUSED_MATERIAL_SLOT__");
    const TArray<FName> LegacyMaterialWidgetNames = {
        TEXT("BodyMaterialInput"),
        TEXT("HeadMaterialInput"),
        TEXT("ArmorMaterialInput"),
        TEXT("WeaponMaterialInput")
    };

    FString ObjectPathOrEmpty(const UObject* Object)
    {
        return Object ? Object->GetPathName() : FString();
    }
}

void UEnemyCreationWizardWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CacheLegacyWidgets();
    InitializeSelectionsFromLegacy();
    InstallNativeControls();

    if (EnemyNameInput)
    {
        EnemyNameInput->OnTextChanged.AddUniqueDynamic(this, &UEnemyCreationWizardWidget::HandleEnemyNameChanged);
        EnemyNameInput->OnTextCommitted.AddUniqueDynamic(this, &UEnemyCreationWizardWidget::HandleEnemyNameCommitted);
        UpdateNameValidationNote(EnemyNameInput->GetText().ToString(), false);
    }

    CachedStepIndex = WizardStepSwitcher ? WizardStepSwitcher->GetActiveWidgetIndex() : INDEX_NONE;
    RefreshReviewSummary(true);
    RefreshNativeNavigation();
}

void UEnemyCreationWizardWidget::NativeDestruct()
{
    if (EnemyNameInput)
    {
        EnemyNameInput->OnTextChanged.RemoveDynamic(this, &UEnemyCreationWizardWidget::HandleEnemyNameChanged);
        EnemyNameInput->OnTextCommitted.RemoveDynamic(this, &UEnemyCreationWizardWidget::HandleEnemyNameCommitted);
    }
    DirectoryPickerComboButton.Reset();
    Super::NativeDestruct();
}

void UEnemyCreationWizardWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    SynchronizeExternalLegacyChanges();

    const int32 CurrentStepIndex = WizardStepSwitcher ? WizardStepSwitcher->GetActiveWidgetIndex() : INDEX_NONE;
    const bool bStepChanged = CurrentStepIndex != CachedStepIndex;
    if (bStepChanged)
    {
        CachedStepIndex = CurrentStepIndex;
        if (CurrentStepIndex != 0 && EnemyNameInput)
        {
            const FString CurrentName = EnemyNameInput->GetText().ToString();
            const FString NormalizedName = EnemyCreationWizard::NormalizeBlueprintName(CurrentName);
            if (!NormalizedName.IsEmpty() && CurrentName != NormalizedName)
            {
                bUpdatingEnemyName = true;
                EnemyNameInput->SetText(FText::FromString(NormalizedName));
                bUpdatingEnemyName = false;
                UpdateNameValidationNote(NormalizedName, true);
            }
        }
    }

    RefreshReviewSummary(bStepChanged);
    RefreshNativeNavigation();
}

FString UEnemyCreationWizardWidget::GetBaseDirectory() const
{
    return EnemyCreationWizard::NormalizeBaseDirectory(BaseDirectoryInput ? BaseDirectoryInput->GetText().ToString() : FString());
}

void UEnemyCreationWizardWidget::GetSelectedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
    OutMaterials.Reset(SelectedMaterials.Num());
    for (UMaterialInterface* Material : SelectedMaterials)
    {
        OutMaterials.Add(Material);
    }
}

void UEnemyCreationWizardWidget::GetMaterialSlotNames(TArray<FString>& OutSlotNames) const
{
    OutSlotNames.Reset();
    if (!SelectedSkeletalMesh)
    {
        return;
    }

    const TArray<FSkeletalMaterial>& MeshMaterials = SelectedSkeletalMesh->GetMaterials();
    OutSlotNames.Reserve(MeshMaterials.Num());
    for (int32 Index = 0; Index < MeshMaterials.Num(); ++Index)
    {
        const FName ImportedName = MeshMaterials[Index].ImportedMaterialSlotName;
        OutSlotNames.Add(ImportedName.IsNone() ? FString::Printf(TEXT("Slot %d"), Index) : ImportedName.ToString());
    }
}

void UEnemyCreationWizardWidget::HandleEnemyNameChanged(const FText& Text)
{
    if (!bUpdatingEnemyName)
    {
        UpdateNameValidationNote(Text.ToString(), false);
        RefreshReviewSummary(true);
    }
}

void UEnemyCreationWizardWidget::HandleEnemyNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    const FString Original = Text.ToString();
    const FString Normalized = EnemyCreationWizard::NormalizeBlueprintName(Original);
    if (!Normalized.IsEmpty() && Normalized != Original && EnemyNameInput)
    {
        bUpdatingEnemyName = true;
        EnemyNameInput->SetText(FText::FromString(Normalized));
        bUpdatingEnemyName = false;
    }
    UpdateNameValidationNote(Normalized, !Normalized.IsEmpty() && Normalized != Original);
    RefreshReviewSummary(true);
}

void UEnemyCreationWizardWidget::CacheLegacyWidgets()
{
    EnemyNameInput = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("EnemyNameInput")));
    BaseDirectoryInput = Cast<UEditableTextBox>(GetWidgetFromName(TEXT("BaseDirectoryInput")));
    NameValidationNote = Cast<UTextBlock>(GetWidgetFromName(TEXT("NameValidationNote")));
    WizardStepSwitcher = Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("WizardStepSwitcher")));
    ReviewSummaryText = Cast<UMultiLineEditableTextBox>(GetWidgetFromName(TEXT("ReviewSummaryText")));
    PreviousButton = Cast<UButton>(GetWidgetFromName(TEXT("PreviousButton")));
    NextButton = Cast<UButton>(GetWidgetFromName(TEXT("NextButton")));
    GenerateEnemyButton = Cast<UButton>(GetWidgetFromName(TEXT("GenerateEnemyButton")));
}

void UEnemyCreationWizardWidget::InstallNativeControls()
{
    UVerticalBox* IdentityPanel = Cast<UVerticalBox>(GetWidgetFromName(TEXT("Step1IdentityPanel")));
    UVerticalBox* VisualsPanel = Cast<UVerticalBox>(GetWidgetFromName(TEXT("Step2VisualsPanel")));
    UVerticalBox* AIPanel = Cast<UVerticalBox>(GetWidgetFromName(TEXT("Step4AIPanel")));

    DirectoryPickerHost = FindOrCreateHost(TEXT("NativeDirectoryPickerHost"), IdentityPanel);
    VisualPickerHost = FindOrCreateHost(TEXT("NativeVisualPickerHost"), VisualsPanel);
    BehaviorTreePickerHost = FindOrCreateHost(TEXT("NativeBehaviorTreePickerHost"), AIPanel);

    if (DirectoryPickerHost)
    {
        DirectoryPickerHost->SetContent(BuildDirectoryPickerWidget());
    }
    if (VisualPickerHost)
    {
        VisualPickerHost->SetContent(BuildVisualPickerWidget());
    }
    if (BehaviorTreePickerHost)
    {
        BehaviorTreePickerHost->SetContent(BuildBehaviorTreePickerWidget());
    }

    InstallNativeStatsControls();

    const TArray<FName> LegacyVisualInputs = {
        TEXT("SkeletalMeshInput"),
        TEXT("BodyMaterialInput"),
        TEXT("HeadMaterialInput"),
        TEXT("ArmorMaterialInput"),
        TEXT("WeaponMaterialInput")
    };
    for (const FName WidgetName : LegacyVisualInputs)
    {
        if (UWidget* Widget = GetWidgetFromName(WidgetName))
        {
            Widget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    if (UWidget* BehaviorTreeInput = GetWidgetFromName(TEXT("BehaviorTreeInput")))
    {
        BehaviorTreeInput->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (UTextBlock* Note = Cast<UTextBlock>(GetWidgetFromName(TEXT("DirectoryPickerNote"))))
    {
        Note->SetText(LOCTEXT("DirectoryPickerReady", "Type a /Game/ path above or choose one with the native Content Browser folder picker below."));
    }
    if (UTextBlock* Note = Cast<UTextBlock>(GetWidgetFromName(TEXT("MeshPickerNote"))))
    {
        Note->SetText(LOCTEXT("MeshPickerReady", "Use the native Skeletal Mesh picker. Material rows rebuild from the mesh's real imported slots."));
    }
    if (UTextBlock* Heading = Cast<UTextBlock>(GetWidgetFromName(TEXT("MaterialSlotsHeading"))))
    {
        Heading->SetText(LOCTEXT("DynamicMaterialHeading", "Dynamic Material Interface overrides by imported mesh slot"));
    }
    if (UTextBlock* Note = Cast<UTextBlock>(GetWidgetFromName(TEXT("BehaviorTreePickerNote"))))
    {
        Note->SetText(LOCTEXT("BehaviorTreePickerReady", "Choose a Behavior Tree with the native filtered asset picker below."));
    }
}


void UEnemyCreationWizardWidget::InstallNativeStatsControls()
{
    UVerticalBox* StatsPanel = Cast<UVerticalBox>(GetWidgetFromName(TEXT("Step3StatsPanel")));
    if (!StatsPanel)
    {
        return;
    }

    HealthSliderHost = FindOrCreateHost(TEXT("NativeHealthSliderHost"), StatsPanel);
    DamageSliderHost = FindOrCreateHost(TEXT("NativeDamageSliderHost"), StatsPanel);
    MovementSpeedSliderHost = FindOrCreateHost(TEXT("NativeMovementSpeedSliderHost"), StatsPanel);

    if (HealthSliderHost)
    {
        HealthSliderHost->SetContent(BuildStatsSliderWidget(TEXT("HealthSpinBox"), TEXT("Health quick adjust"), 1.0f, 100000.0f));
    }
    if (DamageSliderHost)
    {
        DamageSliderHost->SetContent(BuildStatsSliderWidget(TEXT("DamageSpinBox"), TEXT("Base Damage quick adjust"), 0.0f, 100000.0f));
    }
    if (MovementSpeedSliderHost)
    {
        MovementSpeedSliderHost->SetContent(BuildStatsSliderWidget(TEXT("MovementSpeedSpinBox"), TEXT("Movement Speed quick adjust"), 1.0f, 5000.0f));
    }
}

TSharedRef<SWidget> UEnemyCreationWizardWidget::BuildStatsSliderWidget(
    const FName SpinBoxName,
    const FString& Label,
    const float MinValue,
    const float MaxValue)
{
    TWeakObjectPtr<UEnemyCreationWizardWidget> WeakThis(this);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 3.0f, 0.0f, 2.0f)
        [
            SNew(STextBlock).Text(FText::FromString(Label))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SSlider)
            .MinValue(MinValue)
            .MaxValue(MaxValue)
            .Value_Lambda([WeakThis, SpinBoxName, MinValue, MaxValue]()
            {
                if (!WeakThis.IsValid())
                {
                    return 0.0f;
                }
                if (const USpinBox* SpinBox = Cast<USpinBox>(WeakThis->GetWidgetFromName(SpinBoxName)))
                {
                    return FMath::GetRangePct(MinValue, MaxValue, FMath::Clamp(SpinBox->GetValue(), MinValue, MaxValue));
                }
                return 0.0f;
            })
            .OnValueChanged_Lambda([WeakThis, SpinBoxName, MinValue, MaxValue](const float SliderValue)
            {
                if (!WeakThis.IsValid())
                {
                    return;
                }
                if (USpinBox* SpinBox = Cast<USpinBox>(WeakThis->GetWidgetFromName(SpinBoxName)))
                {
                    SpinBox->SetValue(FMath::Lerp(MinValue, MaxValue, FMath::Clamp(SliderValue, 0.0f, 1.0f)));
                }
            })
        ];
}

UNativeWidgetHost* UEnemyCreationWizardWidget::FindOrCreateHost(const FName HostName, UPanelWidget* ParentPanel)
{
    if (!ParentPanel || !WidgetTree)
    {
        return nullptr;
    }

    if (UNativeWidgetHost* ExistingHost = Cast<UNativeWidgetHost>(GetWidgetFromName(HostName)))
    {
        return ExistingHost;
    }

    UNativeWidgetHost* Host = WidgetTree->ConstructWidget<UNativeWidgetHost>(UNativeWidgetHost::StaticClass(), HostName);
    if (!Host)
    {
        return nullptr;
    }

    if (UVerticalBoxSlot* PanelSlot = Cast<UVerticalBoxSlot>(ParentPanel->AddChild(Host)))
    {
        PanelSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 8.0f));
    }
    return Host;
}

TSharedRef<SWidget> UEnemyCreationWizardWidget::BuildDirectoryPickerWidget()
{
    TWeakObjectPtr<UEnemyCreationWizardWidget> WeakThis(this);
    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text_Lambda([WeakThis]()
            {
                return WeakThis.IsValid()
                    ? FText::Format(LOCTEXT("SelectedDirectory", "Selected folder: {0}"), FText::FromString(WeakThis->GetBaseDirectory()))
                    : FText::GetEmpty();
            })
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SAssignNew(DirectoryPickerComboButton, SComboButton)
            .OnGetMenuContent_Lambda([WeakThis]()
            {
                return WeakThis.IsValid() ? WeakThis->BuildDirectoryPickerMenu() : SNullWidget::NullWidget;
            })
            .ButtonContent()
            [
                SNew(STextBlock).Text(LOCTEXT("ChooseContentFolder", "Choose Content Folder..."))
            ]
        ];
}

TSharedRef<SWidget> UEnemyCreationWizardWidget::BuildDirectoryPickerMenu()
{
    FPathPickerConfig PickerConfig;
    PickerConfig.DefaultPath = GetBaseDirectory();
    PickerConfig.bAllowClassesFolder = false;
    PickerConfig.bAllowContextMenu = true;
    PickerConfig.bAllowReadOnlyFolders = false;
    PickerConfig.bAddDefaultPath = true;
    PickerConfig.bShowFavorites = true;
    PickerConfig.bShowViewOptions = true;
    PickerConfig.OnPathSelected = FOnPathSelected::CreateUObject(this, &UEnemyCreationWizardWidget::HandleDirectorySelected);

    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    return SNew(SBox)
        .WidthOverride(440.0f)
        .HeightOverride(520.0f)
        [
            ContentBrowserModule.Get().CreatePathPicker(PickerConfig)
        ];
}

TSharedRef<SWidget> UEnemyCreationWizardWidget::BuildVisualPickerWidget()
{
    TWeakObjectPtr<UEnemyCreationWizardWidget> WeakThis(this);
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            SNew(STextBlock).Text(LOCTEXT("NativeMeshPickerLabel", "Skeletal Mesh"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SObjectPropertyEntryBox)
            .ObjectPath_Lambda([WeakThis]()
            {
                return WeakThis.IsValid() ? WeakThis->GetSkeletalMeshObjectPath() : FString();
            })
            .AllowedClass(USkeletalMesh::StaticClass())
            .AllowClear(false)
            .AllowCreate(false)
            .DisplayUseSelected(true)
            .DisplayBrowse(true)
            .EnableContentPicker(true)
            .OnObjectChanged_Lambda([WeakThis](const FAssetData& AssetData)
            {
                if (WeakThis.IsValid())
                {
                    WeakThis->HandleSkeletalMeshAssetChanged(AssetData);
                }
            })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SSeparator)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
        [
            BuildMaterialPickerWidget()
        ];
}

TSharedRef<SWidget> UEnemyCreationWizardWidget::BuildBehaviorTreePickerWidget()
{
    TWeakObjectPtr<UEnemyCreationWizardWidget> WeakThis(this);
    return SNew(SObjectPropertyEntryBox)
        .ObjectPath_Lambda([WeakThis]()
        {
            return WeakThis.IsValid() ? WeakThis->GetBehaviorTreeObjectPath() : FString();
        })
        .AllowedClass(UBehaviorTree::StaticClass())
        .AllowClear(false)
        .AllowCreate(false)
        .DisplayUseSelected(true)
        .DisplayBrowse(true)
        .EnableContentPicker(true)
        .OnObjectChanged_Lambda([WeakThis](const FAssetData& AssetData)
        {
            if (WeakThis.IsValid())
            {
                WeakThis->HandleBehaviorTreeAssetChanged(AssetData);
            }
        });
}

TSharedRef<SWidget> UEnemyCreationWizardWidget::BuildMaterialPickerWidget()
{
    TSharedRef<SVerticalBox> MaterialList = SNew(SVerticalBox);
    if (!SelectedSkeletalMesh)
    {
        MaterialList->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock).Text(LOCTEXT("SelectMeshForMaterials", "Select a Skeletal Mesh to discover its material slots."))
        ];
        return MaterialList;
    }

    const TArray<FSkeletalMaterial>& MeshMaterials = SelectedSkeletalMesh->GetMaterials();
    if (MeshMaterials.IsEmpty())
    {
        MaterialList->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock).Text(LOCTEXT("NoMeshMaterials", "This mesh has 0 material slots. No material overrides are required."))
        ];
        return MaterialList;
    }

    TWeakObjectPtr<UEnemyCreationWizardWidget> WeakThis(this);
    for (int32 Index = 0; Index < MeshMaterials.Num(); ++Index)
    {
        const FName ImportedName = MeshMaterials[Index].ImportedMaterialSlotName;
        const FText SlotLabel = FText::Format(
            LOCTEXT("MaterialSlotLabel", "[{0}] {1}"),
            FText::AsNumber(Index),
            FText::FromString(ImportedName.IsNone() ? FString::Printf(TEXT("Slot %d"), Index) : ImportedName.ToString()));

        MaterialList->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 3.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 3.0f)
            [
                SNew(STextBlock).Text(SlotLabel)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SObjectPropertyEntryBox)
                .ObjectPath_Lambda([WeakThis, Index]()
                {
                    return WeakThis.IsValid() ? WeakThis->GetMaterialObjectPath(Index) : FString();
                })
                .AllowedClass(UMaterialInterface::StaticClass())
                .AllowClear(true)
                .AllowCreate(false)
                .DisplayUseSelected(true)
                .DisplayBrowse(true)
                .EnableContentPicker(true)
                .OnObjectChanged_Lambda([WeakThis, Index](const FAssetData& AssetData)
                {
                    if (WeakThis.IsValid())
                    {
                        WeakThis->HandleMaterialAssetChanged(AssetData, Index);
                    }
                })
            ]
        ];
    }
    return MaterialList;
}

void UEnemyCreationWizardWidget::InitializeSelectionsFromLegacy()
{
    const FString MeshPath = EnemyCreationWizard::ToObjectPath(GetLegacyText(TEXT("SkeletalMeshInput")));
    USkeletalMesh* InitialMesh = MeshPath.IsEmpty() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *MeshPath);
    SetSelectedSkeletalMesh(InitialMesh, true);

    if (InitialMesh)
    {
        for (int32 Index = 0; Index < FMath::Min(LegacyMaterialWidgetNames.Num(), SelectedMaterials.Num()); ++Index)
        {
            const FString LegacyMaterialPath = EnemyCreationWizard::ToObjectPath(GetLegacyText(LegacyMaterialWidgetNames[Index]));
            if (!LegacyMaterialPath.IsEmpty())
            {
                if (UMaterialInterface* LegacyMaterial = LoadObject<UMaterialInterface>(nullptr, *LegacyMaterialPath))
                {
                    SelectedMaterials[Index] = LegacyMaterial;
                }
            }
        }
        MirrorMaterialsToLegacy();
    }

    const FString BehaviorTreePath = EnemyCreationWizard::ToObjectPath(GetLegacyText(TEXT("BehaviorTreeInput")));
    SetSelectedBehaviorTree(BehaviorTreePath.IsEmpty() ? nullptr : LoadObject<UBehaviorTree>(nullptr, *BehaviorTreePath));
}

void UEnemyCreationWizardWidget::SynchronizeExternalLegacyChanges()
{
    if (bUpdatingLegacyFields)
    {
        return;
    }

    const FString LegacyMeshPath = EnemyCreationWizard::ToObjectPath(GetLegacyText(TEXT("SkeletalMeshInput")));
    if (LegacyMeshPath != LastMirroredMeshPath)
    {
        SetSelectedSkeletalMesh(LegacyMeshPath.IsEmpty() ? nullptr : LoadObject<USkeletalMesh>(nullptr, *LegacyMeshPath), true);
    }

    const FString LegacyBehaviorTreePath = EnemyCreationWizard::ToObjectPath(GetLegacyText(TEXT("BehaviorTreeInput")));
    if (LegacyBehaviorTreePath != LastMirroredBehaviorTreePath)
    {
        SetSelectedBehaviorTree(LegacyBehaviorTreePath.IsEmpty() ? nullptr : LoadObject<UBehaviorTree>(nullptr, *LegacyBehaviorTreePath));
    }
}

void UEnemyCreationWizardWidget::SetSelectedSkeletalMesh(USkeletalMesh* NewMesh, const bool bResetMaterialsToMeshDefaults)
{
    SelectedSkeletalMesh = NewMesh;
    if (bResetMaterialsToMeshDefaults)
    {
        SelectedMaterials.Reset();
        if (SelectedSkeletalMesh)
        {
            const TArray<FSkeletalMaterial>& MeshMaterials = SelectedSkeletalMesh->GetMaterials();
            SelectedMaterials.Reserve(MeshMaterials.Num());
            for (const FSkeletalMaterial& MeshMaterial : MeshMaterials)
            {
                SelectedMaterials.Add(MeshMaterial.MaterialInterface);
            }
        }
    }

    MirrorMeshToLegacy();
    MirrorMaterialsToLegacy();
    RebuildMaterialPicker();
    RefreshReviewSummary(true);
}

void UEnemyCreationWizardWidget::SetSelectedBehaviorTree(UBehaviorTree* NewBehaviorTree)
{
    SelectedBehaviorTree = NewBehaviorTree;
    MirrorBehaviorTreeToLegacy();
    RefreshReviewSummary(true);
}

void UEnemyCreationWizardWidget::SetMaterialSelection(const int32 SlotIndex, UMaterialInterface* NewMaterial)
{
    if (!SelectedMaterials.IsValidIndex(SlotIndex))
    {
        return;
    }
    SelectedMaterials[SlotIndex] = NewMaterial;
    MirrorMaterialsToLegacy();
    RefreshReviewSummary(true);
}

void UEnemyCreationWizardWidget::MirrorMeshToLegacy()
{
    bUpdatingLegacyFields = true;
    LastMirroredMeshPath = GetSkeletalMeshObjectPath();
    SetLegacyText(TEXT("SkeletalMeshInput"), LastMirroredMeshPath);
    bUpdatingLegacyFields = false;
}

void UEnemyCreationWizardWidget::MirrorBehaviorTreeToLegacy()
{
    bUpdatingLegacyFields = true;
    LastMirroredBehaviorTreePath = GetBehaviorTreeObjectPath();
    SetLegacyText(TEXT("BehaviorTreeInput"), LastMirroredBehaviorTreePath);
    bUpdatingLegacyFields = false;
}

void UEnemyCreationWizardWidget::MirrorMaterialsToLegacy()
{
    bUpdatingLegacyFields = true;
    for (int32 Index = 0; Index < LegacyMaterialWidgetNames.Num(); ++Index)
    {
        const FString Value = SelectedMaterials.IsValidIndex(Index)
            ? ObjectPathOrEmpty(SelectedMaterials[Index])
            : FString(UnusedLegacyMaterialValue);
        SetLegacyText(LegacyMaterialWidgetNames[Index], Value);
    }
    bUpdatingLegacyFields = false;
}

void UEnemyCreationWizardWidget::RebuildMaterialPicker()
{
    if (VisualPickerHost)
    {
        VisualPickerHost->SetContent(BuildVisualPickerWidget());
    }
}

void UEnemyCreationWizardWidget::HandleDirectorySelected(const FString& SelectedPath)
{
    if (BaseDirectoryInput)
    {
        BaseDirectoryInput->SetText(FText::FromString(EnemyCreationWizard::NormalizeBaseDirectory(SelectedPath)));
    }
    if (DirectoryPickerComboButton)
    {
        DirectoryPickerComboButton->SetIsOpen(false);
    }
    RefreshReviewSummary(true);
}

void UEnemyCreationWizardWidget::HandleSkeletalMeshAssetChanged(const FAssetData& AssetData)
{
    SetSelectedSkeletalMesh(Cast<USkeletalMesh>(AssetData.GetAsset()), true);
}

void UEnemyCreationWizardWidget::HandleBehaviorTreeAssetChanged(const FAssetData& AssetData)
{
    SetSelectedBehaviorTree(Cast<UBehaviorTree>(AssetData.GetAsset()));
}

void UEnemyCreationWizardWidget::HandleMaterialAssetChanged(const FAssetData& AssetData, const int32 SlotIndex)
{
    SetMaterialSelection(SlotIndex, Cast<UMaterialInterface>(AssetData.GetAsset()));
}

FString UEnemyCreationWizardWidget::GetSkeletalMeshObjectPath() const
{
    return ObjectPathOrEmpty(SelectedSkeletalMesh);
}

FString UEnemyCreationWizardWidget::GetBehaviorTreeObjectPath() const
{
    return ObjectPathOrEmpty(SelectedBehaviorTree);
}

FString UEnemyCreationWizardWidget::GetMaterialObjectPath(const int32 SlotIndex) const
{
    return SelectedMaterials.IsValidIndex(SlotIndex) ? ObjectPathOrEmpty(SelectedMaterials[SlotIndex]) : FString();
}

FString UEnemyCreationWizardWidget::GetLegacyText(const FName WidgetName) const
{
    if (const UEditableTextBox* TextBox = Cast<UEditableTextBox>(GetWidgetFromName(WidgetName)))
    {
        return TextBox->GetText().ToString().TrimStartAndEnd();
    }
    return FString();
}

void UEnemyCreationWizardWidget::SetLegacyText(const FName WidgetName, const FString& Value) const
{
    if (UEditableTextBox* TextBox = Cast<UEditableTextBox>(GetWidgetFromName(WidgetName)))
    {
        if (TextBox->GetText().ToString() != Value)
        {
            TextBox->SetText(FText::FromString(Value));
        }
    }
}

void UEnemyCreationWizardWidget::UpdateNameValidationNote(const FString& CurrentValue, const bool bWasNormalized)
{
    if (!NameValidationNote)
    {
        return;
    }

    const FString Trimmed = CurrentValue.TrimStartAndEnd();
    if (Trimmed.IsEmpty())
    {
        NameValidationNote->SetText(LOCTEXT("NameEmpty", "Name is required. Enter a Blueprint asset name; BP_ is added automatically when editing is committed."));
        NameValidationNote->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.25f, 0.2f)));
        return;
    }

    const FString Normalized = EnemyCreationWizard::NormalizeBlueprintName(Trimmed);
    if (!EnemyCreationWizard::IsSafeAssetName(Normalized) || Normalized.Len() <= 3)
    {
        NameValidationNote->SetText(LOCTEXT("NameInvalid", "Invalid name. Use letters, digits, and underscores, with at least one character after BP_."));
        NameValidationNote->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.25f, 0.2f)));
        return;
    }

    if (bWasNormalized)
    {
        NameValidationNote->SetText(FText::Format(LOCTEXT("NameNormalized", "Normalized to {0}. The generated Blueprint will use this exact BP_ name."), FText::FromString(Normalized)));
    }
    else if (Trimmed != Normalized)
    {
        NameValidationNote->SetText(FText::Format(LOCTEXT("NameWillNormalize", "Valid input. It will normalize to {0} when editing is committed."), FText::FromString(Normalized)));
    }
    else
    {
        NameValidationNote->SetText(LOCTEXT("NameValid", "Valid Blueprint name. BP_ prefix is present."));
    }
    NameValidationNote->SetColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.9f, 0.35f)));
}


bool UEnemyCreationWizardWidget::IsWizardStepValid(const int32 StepIndex) const
{
    switch (StepIndex)
    {
    case 0:
    {
        const FString RawName = GetLegacyText(TEXT("EnemyNameInput"));
        const FString NormalizedName = EnemyCreationWizard::NormalizeBlueprintName(RawName);
        const FString RawDirectory = GetLegacyText(TEXT("BaseDirectoryInput"));
        const FString Directory = GetBaseDirectory();
        return !RawName.IsEmpty() &&
            NormalizedName.Len() > 3 &&
            EnemyCreationWizard::IsSafeAssetName(NormalizedName) &&
            !RawDirectory.IsEmpty() &&
            Directory.StartsWith(TEXT("/Game/")) &&
            FPackageName::IsValidLongPackageName(Directory, false);
    }
    case 1:
    {
        if (!SelectedSkeletalMesh)
        {
            return false;
        }
        const int32 SlotCount = SelectedSkeletalMesh->GetMaterials().Num();
        if (SelectedMaterials.Num() != SlotCount)
        {
            return false;
        }
        for (UMaterialInterface* Material : SelectedMaterials)
        {
            if (!Material)
            {
                return false;
            }
        }
        return true;
    }
    case 2:
    {
        const USpinBox* Health = Cast<USpinBox>(GetWidgetFromName(TEXT("HealthSpinBox")));
        const USpinBox* Damage = Cast<USpinBox>(GetWidgetFromName(TEXT("DamageSpinBox")));
        const USpinBox* MovementSpeed = Cast<USpinBox>(GetWidgetFromName(TEXT("MovementSpeedSpinBox")));
        const UComboBoxString* EnemyClass = Cast<UComboBoxString>(GetWidgetFromName(TEXT("EnemyClassCombo")));
        static const TSet<FString> ValidClasses = { TEXT("Grunt"), TEXT("Scout"), TEXT("Brute"), TEXT("Boss") };
        return Health && Damage && MovementSpeed && EnemyClass &&
            FMath::IsFinite(Health->GetValue()) && Health->GetValue() > 0.0f &&
            FMath::IsFinite(Damage->GetValue()) && Damage->GetValue() >= 0.0f &&
            FMath::IsFinite(MovementSpeed->GetValue()) && MovementSpeed->GetValue() > 0.0f &&
            ValidClasses.Contains(EnemyClass->GetSelectedOption());
    }
    case 3:
        return SelectedBehaviorTree != nullptr;
    case 4:
        return true;
    default:
        return false;
    }
}

void UEnemyCreationWizardWidget::RefreshNativeNavigation()
{
    const int32 ActiveStep = WizardStepSwitcher ? WizardStepSwitcher->GetActiveWidgetIndex() : INDEX_NONE;
    if (ActiveStep == INDEX_NONE)
    {
        return;
    }

    if (PreviousButton)
    {
        PreviousButton->SetIsEnabled(ActiveStep > 0);
    }
    if (NextButton)
    {
        NextButton->SetIsEnabled(ActiveStep >= 0 && ActiveStep < 4 && IsWizardStepValid(ActiveStep));
    }
    if (GenerateEnemyButton)
    {
        GenerateEnemyButton->SetVisibility(ActiveStep == 4 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        GenerateEnemyButton->SetIsEnabled(ActiveStep == 4 && IsWizardStepValid(4));
    }
}

void UEnemyCreationWizardWidget::RefreshReviewSummary(const bool bForce)
{
    if (!ReviewSummaryText)
    {
        return;
    }

    const FString Signature = BuildStateSignature();
    if (!bForce && Signature == CachedStateSignature)
    {
        return;
    }
    CachedStateSignature = Signature;

    EnemyCreationWizard::FReviewData Data;
    Data.EnemyName = GetLegacyText(TEXT("EnemyNameInput"));
    Data.BaseDirectory = GetBaseDirectory();
    if (const UCheckBox* CheckBox = Cast<UCheckBox>(GetWidgetFromName(TEXT("CreateSubFoldersCheckBox"))))
    {
        Data.bCreateSubFolders = CheckBox->IsChecked();
    }
    Data.SkeletalMeshPath = GetSkeletalMeshObjectPath();
    GetMaterialSlotNames(Data.MaterialSlotNames);
    for (UMaterialInterface* Material : SelectedMaterials)
    {
        Data.MaterialPaths.Add(ObjectPathOrEmpty(Material));
    }
    if (const USpinBox* SpinBox = Cast<USpinBox>(GetWidgetFromName(TEXT("HealthSpinBox"))))
    {
        Data.Health = SpinBox->GetValue();
    }
    if (const USpinBox* SpinBox = Cast<USpinBox>(GetWidgetFromName(TEXT("DamageSpinBox"))))
    {
        Data.Damage = SpinBox->GetValue();
    }
    if (const USpinBox* SpinBox = Cast<USpinBox>(GetWidgetFromName(TEXT("MovementSpeedSpinBox"))))
    {
        Data.MovementSpeed = SpinBox->GetValue();
    }
    if (const UComboBoxString* ComboBox = Cast<UComboBoxString>(GetWidgetFromName(TEXT("EnemyClassCombo"))))
    {
        Data.EnemyClass = ComboBox->GetSelectedOption();
    }
    Data.BehaviorTreePath = GetBehaviorTreeObjectPath();
    Data.AnimationBlueprintPath = GetLegacyText(TEXT("AnimationBlueprintInput"));
    Data.DataAssetPath = GetLegacyText(TEXT("DataAssetInput"));
    Data.GameplayTags = GetLegacyText(TEXT("GameplayTagsInput"));
    if (const UComboBoxString* ComboBox = Cast<UComboBoxString>(GetWidgetFromName(TEXT("AITypeCombo"))))
    {
        Data.AIType = ComboBox->GetSelectedOption();
    }
    if (const UCheckBox* CheckBox = Cast<UCheckBox>(GetWidgetFromName(TEXT("GenerateAICheckBox"))))
    {
        Data.bGenerateAI = CheckBox->IsChecked();
    }

    ReviewSummaryText->SetText(FText::FromString(EnemyCreationWizard::BuildReviewSummary(Data)));
}


FString UEnemyCreationWizardWidget::BuildStateSignature() const
{
    TArray<FString> Values;
    Values.Reserve(24 + SelectedMaterials.Num());
    Values.Add(GetLegacyText(TEXT("EnemyNameInput")));
    Values.Add(GetBaseDirectory());
    Values.Add(GetSkeletalMeshObjectPath());
    Values.Add(GetBehaviorTreeObjectPath());
    Values.Add(GetLegacyText(TEXT("AnimationBlueprintInput")));
    Values.Add(GetLegacyText(TEXT("DataAssetInput")));
    Values.Add(GetLegacyText(TEXT("GameplayTagsInput")));

    const TArray<FName> NumericAndSelectionWidgets = {
        TEXT("HealthSpinBox"), TEXT("DamageSpinBox"), TEXT("MovementSpeedSpinBox"),
        TEXT("EnemyClassCombo"), TEXT("AITypeCombo"), TEXT("CreateSubFoldersCheckBox"), TEXT("GenerateAICheckBox")
    };
    for (const FName WidgetName : NumericAndSelectionWidgets)
    {
        if (const USpinBox* SpinBox = Cast<USpinBox>(GetWidgetFromName(WidgetName)))
        {
            Values.Add(FString::SanitizeFloat(SpinBox->GetValue()));
        }
        else if (const UComboBoxString* ComboBox = Cast<UComboBoxString>(GetWidgetFromName(WidgetName)))
        {
            Values.Add(ComboBox->GetSelectedOption());
        }
        else if (const UCheckBox* CheckBox = Cast<UCheckBox>(GetWidgetFromName(WidgetName)))
        {
            Values.Add(CheckBox->IsChecked() ? TEXT("1") : TEXT("0"));
        }
        else
        {
            Values.Add(TEXT("<missing>"));
        }
    }

    for (UMaterialInterface* Material : SelectedMaterials)
    {
        Values.Add(ObjectPathOrEmpty(Material));
    }
    return FString::Join(Values, TEXT("|"));
}

#undef LOCTEXT_NAMESPACE



