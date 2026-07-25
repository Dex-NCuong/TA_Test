#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "EnemyCreationWizardWidget.generated.h"

class UBehaviorTree;
class UButton;
class UEditableTextBox;
class UMaterialInterface;
class UMultiLineEditableTextBox;
class UNativeWidgetHost;
class UPanelWidget;
class USkeletalMesh;
class UTextBlock;
class UWidgetSwitcher;
class SComboButton;
class SWidget;
struct FAssetData;

UCLASS(BlueprintType, Blueprintable)
class ENEMYCREATIONWIZARDEDITOR_API UEnemyCreationWizardWidget : public UEditorUtilityWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    USkeletalMesh* GetSelectedSkeletalMesh() const { return SelectedSkeletalMesh; }
    UBehaviorTree* GetSelectedBehaviorTree() const { return SelectedBehaviorTree; }
    FString GetBaseDirectory() const;
    void GetSelectedMaterials(TArray<UMaterialInterface*>& OutMaterials) const;
    void GetMaterialSlotNames(TArray<FString>& OutSlotNames) const;

private:
    UFUNCTION()
    void HandleEnemyNameChanged(const FText& Text);

    UFUNCTION()
    void HandleEnemyNameCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    void CacheLegacyWidgets();
    void InstallNativeControls();
    void InstallNativeStatsControls();
    void RefreshNativeNavigation();
    bool IsWizardStepValid(int32 StepIndex) const;
    UNativeWidgetHost* FindOrCreateHost(FName HostName, UPanelWidget* ParentPanel);
    TSharedRef<SWidget> BuildDirectoryPickerWidget();
    TSharedRef<SWidget> BuildDirectoryPickerMenu();
    TSharedRef<SWidget> BuildVisualPickerWidget();
    TSharedRef<SWidget> BuildBehaviorTreePickerWidget();
    TSharedRef<SWidget> BuildMaterialPickerWidget();
    TSharedRef<SWidget> BuildStatsSliderWidget(FName SpinBoxName, const FString& Label, float MinValue, float MaxValue);

    void InitializeSelectionsFromLegacy();
    void SynchronizeExternalLegacyChanges();
    void SetSelectedSkeletalMesh(USkeletalMesh* NewMesh, bool bResetMaterialsToMeshDefaults);
    void SetSelectedBehaviorTree(UBehaviorTree* NewBehaviorTree);
    void SetMaterialSelection(int32 SlotIndex, UMaterialInterface* NewMaterial);
    void MirrorMeshToLegacy();
    void MirrorBehaviorTreeToLegacy();
    void MirrorMaterialsToLegacy();
    void RebuildMaterialPicker();
    void HandleDirectorySelected(const FString& SelectedPath);
    void HandleSkeletalMeshAssetChanged(const FAssetData& AssetData);
    void HandleBehaviorTreeAssetChanged(const FAssetData& AssetData);
    void HandleMaterialAssetChanged(const FAssetData& AssetData, int32 SlotIndex);

    FString GetSkeletalMeshObjectPath() const;
    FString GetBehaviorTreeObjectPath() const;
    FString GetMaterialObjectPath(int32 SlotIndex) const;
    FString GetLegacyText(FName WidgetName) const;
    void SetLegacyText(FName WidgetName, const FString& Value) const;
    void UpdateNameValidationNote(const FString& CurrentValue, bool bWasNormalized);
    void RefreshReviewSummary(bool bForce = false);
    FString BuildStateSignature() const;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> EnemyNameInput;

    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> BaseDirectoryInput;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameValidationNote;

    UPROPERTY(Transient)
    TObjectPtr<UWidgetSwitcher> WizardStepSwitcher;

    UPROPERTY(Transient)
    TObjectPtr<UMultiLineEditableTextBox> ReviewSummaryText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> PreviousButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> NextButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> GenerateEnemyButton;

    UPROPERTY(Transient)
    TObjectPtr<UNativeWidgetHost> DirectoryPickerHost;

    UPROPERTY(Transient)
    TObjectPtr<UNativeWidgetHost> VisualPickerHost;

    UPROPERTY(Transient)
    TObjectPtr<UNativeWidgetHost> BehaviorTreePickerHost;

    UPROPERTY(Transient)
    TObjectPtr<UNativeWidgetHost> HealthSliderHost;

    UPROPERTY(Transient)
    TObjectPtr<UNativeWidgetHost> DamageSliderHost;

    UPROPERTY(Transient)
    TObjectPtr<UNativeWidgetHost> MovementSpeedSliderHost;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> SelectedSkeletalMesh;

    UPROPERTY(Transient)
    TObjectPtr<UBehaviorTree> SelectedBehaviorTree;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> SelectedMaterials;

    TSharedPtr<SComboButton> DirectoryPickerComboButton;
    FString CachedStateSignature;
    FString LastMirroredMeshPath;
    FString LastMirroredBehaviorTreePath;
    int32 CachedStepIndex = INDEX_NONE;
    bool bUpdatingLegacyFields = false;
    bool bUpdatingEnemyName = false;
};
