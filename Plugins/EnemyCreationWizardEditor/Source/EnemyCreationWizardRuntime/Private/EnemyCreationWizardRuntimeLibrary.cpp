#include "EnemyCreationWizardRuntimeLibrary.h"
#include "EnemyCreationWizardRuntimeBridge.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#endif

namespace
{
    const TCHAR* WizardAssetObjectPath = TEXT("/Game/GeneratedEnemiesDemo/Editor/Widgets/EUW_EnemyCreationWizard.EUW_EnemyCreationWizard");

#if WITH_EDITOR
    TWeakObjectPtr<UUserWidget> ActiveWizardOverlay;
    TWeakObjectPtr<APlayerController> ActiveWizardOverlayOwner;

    void RestoreGameInput(APlayerController* PlayerController)
    {
        if (!PlayerController)
        {
            return;
        }

        FInputModeGameOnly GameOnlyInput;
        PlayerController->SetInputMode(GameOnlyInput);
        PlayerController->bShowMouseCursor = false;
    }

    void ActivateOverlayInput(APlayerController* PlayerController, UUserWidget* OverlayWidget)
    {
        if (!PlayerController || !OverlayWidget)
        {
            return;
        }

        FInputModeGameAndUI GameAndUIInput;
        GameAndUIInput.SetWidgetToFocus(OverlayWidget->TakeWidget());
        GameAndUIInput.SetHideCursorDuringCapture(false);
        PlayerController->SetInputMode(GameAndUIInput);
        PlayerController->bShowMouseCursor = true;

        OverlayWidget->SetKeyboardFocus();
    }

    bool CloseActiveOverlay()
    {
        if (UUserWidget* ExistingOverlay = ActiveWizardOverlay.Get())
        {
            ExistingOverlay->RemoveFromParent();
            RestoreGameInput(ActiveWizardOverlayOwner.Get());
            ActiveWizardOverlay.Reset();
            ActiveWizardOverlayOwner.Reset();
            return true;
        }

        ActiveWizardOverlay.Reset();
        ActiveWizardOverlayOwner.Reset();
        return false;
    }
#endif
}

bool UEnemyCreationWizardRuntimeLibrary::OpenEnemyCreationWizard()
{
    return FEnemyCreationWizardRuntimeBridge::OnOpenWizardRequested().IsBound()
        ? FEnemyCreationWizardRuntimeBridge::OnOpenWizardRequested().Execute()
        : false;
}

bool UEnemyCreationWizardRuntimeLibrary::ToggleEnemyCreationWizardOverlay()
{
#if WITH_EDITOR
    if (IsRunningCommandlet() || !GEditor || !GEditor->PlayWorld)
    {
        return false;
    }

    if (CloseActiveOverlay())
    {
        return true;
    }

    APlayerController* PlayerController = GEditor->PlayWorld->GetFirstPlayerController();
    if (!PlayerController)
    {
        return false;
    }

    UEditorUtilityWidgetBlueprint* WidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, WizardAssetObjectPath);
    TSubclassOf<UUserWidget> OverlayClass = WidgetBlueprint ? TSubclassOf<UUserWidget>(WidgetBlueprint->GeneratedClass) : nullptr;
    if (!OverlayClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: could not load viewport widget %s."), WizardAssetObjectPath);
        return false;
    }

    UUserWidget* OverlayWidget = CreateWidget<UUserWidget>(PlayerController, OverlayClass);
    if (!OverlayWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: failed to create viewport overlay instance from %s."), WizardAssetObjectPath);
        return false;
    }

    OverlayWidget->AddToViewport(1000);
    ActivateOverlayInput(PlayerController, OverlayWidget);
    ActiveWizardOverlay = OverlayWidget;
    ActiveWizardOverlayOwner = PlayerController;
    return true;
#else
    return false;
#endif
}
