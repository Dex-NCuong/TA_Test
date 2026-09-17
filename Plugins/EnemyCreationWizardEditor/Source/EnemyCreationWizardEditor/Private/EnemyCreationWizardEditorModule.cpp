#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BTTask_ChasePlayer.h"
#include "BlueprintEditorLibrary.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "EnemyCreationWizardRuntimeLibrary.h"
#include "EnemyChaseAIController.h"
#include "EnemyCreationWizardRuntimeBridge.h"
#include "EnemyCreationWizardWidget.h"
#include "Engine/Blueprint.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "EnemyCreationWizardEditorModule"

namespace
{
    const TCHAR* WizardAssetObjectPath = TEXT("/Game/GeneratedEnemiesDemo/Editor/Widgets/EUW_EnemyCreationWizard.EUW_EnemyCreationWizard");
    const TCHAR* DemoBehaviorTreePackagePath = TEXT("/Game/Blueprints/Enemies/BT_TestEnemy");
    const TCHAR* DemoBehaviorTreeObjectPath = TEXT("/Game/Blueprints/Enemies/BT_TestEnemy.BT_TestEnemy");

}

class FEnemyCreationWizardEditorModule;

class FEnemyWizardPIEInputProcessor final : public IInputProcessor
{
public:
    explicit FEnemyWizardPIEInputProcessor(FEnemyCreationWizardEditorModule& InOwner)
        : Owner(InOwner)
    {
    }

    virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
    {
    }

    virtual const TCHAR* GetDebugName() const override
    {
        return TEXT("EnemyWizardPIEInputProcessor");
    }

    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

private:
    static bool HasTextEntryFocus(const FSlateApplication& SlateApp)
    {
        const TSharedPtr<SWidget> FocusedWidget = SlateApp.GetKeyboardFocusedWidget();
        if (!FocusedWidget.IsValid())
        {
            return false;
        }

        const FString WidgetType = FocusedWidget->GetTypeAsString();
        return WidgetType.Contains(TEXT("EditableText")) || WidgetType.Contains(TEXT("TextBox")) || WidgetType.Contains(TEXT("MultiLine"));
    }

    FEnemyCreationWizardEditorModule& Owner;
};

class FEnemyCreationWizardEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if (IsRunningCommandlet())
        {
            return;
        }

        FEnemyCreationWizardRuntimeBridge::OnOpenWizardRequested().BindRaw(
            this,
            &FEnemyCreationWizardEditorModule::HandleOpenWizardRequested);

        ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
            FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FEnemyCreationWizardEditorModule::RegisterMenus));

        if (FSlateApplication::IsInitialized())
        {
            InputProcessor = MakeShared<FEnemyWizardPIEInputProcessor>(*this);
            FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor);
        }

        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        if (AssetRegistryModule.Get().IsLoadingAssets())
        {
            AssetRegistryFilesLoadedHandle = AssetRegistryModule.Get().OnFilesLoaded().AddRaw(
                this,
                &FEnemyCreationWizardEditorModule::HandleAssetRegistryReady);
        }
        else
        {
            ScheduleStartupTasks();
        }
    }

    virtual void ShutdownModule() override
    {
        FEnemyCreationWizardRuntimeBridge::OnOpenWizardRequested().Unbind();

        if (ToolMenusStartupHandle.IsValid())
        {
            UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
            ToolMenusStartupHandle.Reset();
        }
        UToolMenus::UnregisterOwner(this);

        if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
            InputProcessor.Reset();
        }

        if (StartupTickerHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(StartupTickerHandle);
            StartupTickerHandle.Reset();
        }

        if (AssetRegistryFilesLoadedHandle.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
        {
            FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
                .Get()
                .OnFilesLoaded()
                .Remove(AssetRegistryFilesLoadedHandle);
            AssetRegistryFilesLoadedHandle.Reset();
        }
    }

private:
    void HandleAssetRegistryReady()
    {
        if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
        {
            FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
                .Get()
                .OnFilesLoaded()
                .Remove(AssetRegistryFilesLoadedHandle);
        }
        AssetRegistryFilesLoadedHandle.Reset();
        ScheduleStartupTasks();
    }

    void ScheduleStartupTasks()
    {
        if (!StartupTickerHandle.IsValid())
        {
            StartupTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateRaw(this, &FEnemyCreationWizardEditorModule::RunStartupTasks));
        }
    }

    bool RunStartupTasks(float DeltaTime)
    {
        if (!GEditor)
        {
            return true;
        }

        StartupTickerHandle.Reset();
        EnsureDemoBehaviorTree();
        MigrateWizardBlueprint();
        return false;
    }

    void MigrateWizardBlueprint() const
    {
        UEditorUtilityWidgetBlueprint* WidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, WizardAssetObjectPath);
        if (!WidgetBlueprint)
        {
            UE_LOG(LogTemp, Warning, TEXT("Enemy Creation Wizard: could not load %s for native reparent migration."), WizardAssetObjectPath);
            return;
        }

        if (UBlueprintEditorLibrary::GetBlueprintParentClass(WidgetBlueprint) == UEnemyCreationWizardWidget::StaticClass())
        {
            return;
        }

        WidgetBlueprint->Modify();
        UBlueprintEditorLibrary::ReparentBlueprint(WidgetBlueprint, UEnemyCreationWizardWidget::StaticClass());
        FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
        if (WidgetBlueprint->Status == BS_Error)
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: native reparent compiled with errors; the asset was not saved."));
            return;
        }

        if (UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>())
        {
            if (!AssetSubsystem->SaveLoadedAsset(WidgetBlueprint, false))
            {
                UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: reparent succeeded but saving %s failed."), WizardAssetObjectPath);
                return;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("Enemy Creation Wizard: migrated %s to UEnemyCreationWizardWidget."), WizardAssetObjectPath);
    }

    void EnsureDemoBehaviorTree() const
    {
        if (UObject* ExistingObject = StaticLoadObject(UObject::StaticClass(), nullptr, DemoBehaviorTreeObjectPath))
        {
            if (!ExistingObject->IsA<UBehaviorTree>())
            {
                UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: %s exists but is not a Behavior Tree; it was left unchanged."), DemoBehaviorTreeObjectPath);
            }
            else
            {
                EnsureChaseTask(CastChecked<UBehaviorTree>(ExistingObject));
            }
            return;
        }
        if (FPackageName::DoesPackageExist(DemoBehaviorTreePackagePath))
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: package %s exists but the expected Behavior Tree object could not be loaded; it was left unchanged."), DemoBehaviorTreePackagePath);
            return;
        }

        UPackage* Package = CreatePackage(DemoBehaviorTreePackagePath);
        if (!Package)
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: failed to create demo Behavior Tree package %s."), DemoBehaviorTreePackagePath);
            return;
        }

        UBehaviorTree* BehaviorTree = NewObject<UBehaviorTree>(
            Package,
            TEXT("BT_TestEnemy"),
            RF_Public | RF_Standalone | RF_Transactional);
        if (!BehaviorTree)
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: failed to allocate BT_TestEnemy."));
            return;
        }

        BehaviorTree->RootNode = NewObject<UBTComposite_Sequence>(
            BehaviorTree,
            TEXT("RootSequence"),
            RF_Transactional);
        EnsureChaseTask(BehaviorTree);
        FAssetRegistryModule::AssetCreated(BehaviorTree);
        BehaviorTree->MarkPackageDirty();

        UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
        if (!AssetSubsystem || !AssetSubsystem->SaveLoadedAsset(BehaviorTree, false))
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: created BT_TestEnemy in memory, but saving it failed."));
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("Enemy Creation Wizard: created demo Behavior Tree %s with a Sequence root."), DemoBehaviorTreeObjectPath);
    }

    void EnsureChaseTask(UBehaviorTree* BehaviorTree) const
    {
        if (!BehaviorTree)
        {
            return;
        }

        UBTComposite_Sequence* Root = Cast<UBTComposite_Sequence>(BehaviorTree->RootNode);
        if (!Root)
        {
            BehaviorTree->Modify();
            Root = NewObject<UBTComposite_Sequence>(BehaviorTree, TEXT("RootSequence"), RF_Transactional);
            BehaviorTree->RootNode = Root;
        }

        for (const FBTCompositeChild& Child : Root->Children)
        {
            if (Child.ChildTask && Child.ChildTask->IsA<UBTTask_ChasePlayer>())
            {
                return;
            }
        }

        Root->Modify();
        FBTCompositeChild& Child = Root->Children.AddDefaulted_GetRef();
        Child.ChildTask = NewObject<UBTTask_ChasePlayer>(Root, TEXT("ChasePlayer"), RF_Transactional);
        BehaviorTree->MarkPackageDirty();

        if (UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>())
        {
            AssetSubsystem->SaveLoadedAsset(BehaviorTree, false);
        }
    }

    void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(this);
        const FUIAction OpenWizardAction(FExecuteAction::CreateRaw(this, &FEnemyCreationWizardEditorModule::OpenWizard));

        if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
        {
            FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("EnemyCreationWizard"), LOCTEXT("EnemyWizardSection", "Enemy Creation"));
            Section.AddEntry(FToolMenuEntry::InitMenuEntry(
                TEXT("OpenEnemyCreationWizard"),
                LOCTEXT("OpenEnemyCreationWizard", "Enemy Creation Wizard"),
                LOCTEXT("OpenEnemyCreationWizardTooltip", "Open the Enemy Creation Wizard Editor Utility Widget."),
                FSlateIcon(),
                OpenWizardAction));
        }

        if (UToolMenu* UserToolbar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.User")))
        {
            FToolMenuSection& Section = UserToolbar->FindOrAddSection(TEXT("EnemyCreationWizard"));
            FToolMenuEntry ToolbarEntry = FToolMenuEntry::InitToolBarButton(
                TEXT("OpenEnemyCreationWizardToolbar"),
                OpenWizardAction,
                LOCTEXT("OpenEnemyCreationWizardToolbar", "Enemy Wizard"),
                LOCTEXT("OpenEnemyCreationWizardToolbarTooltip", "Open the Enemy Creation Wizard and generate a configured enemy Blueprint."),
                FSlateIcon());
            ToolbarEntry.StyleNameOverride = TEXT("CalloutToolbar");
            Section.AddEntry(ToolbarEntry);
        }
    }

    bool HandleOpenWizardRequested() const
    {
        return TryOpenWizard();
    }

    void OpenWizard() const
    {
        TryOpenWizard();
    }

    bool TryOpenWizard() const
    {
        if (!GEditor)
        {
            return false;
        }

        UEditorUtilityWidgetBlueprint* WidgetBlueprint = LoadObject<UEditorUtilityWidgetBlueprint>(nullptr, WizardAssetObjectPath);
        UEditorUtilitySubsystem* UtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
        if (!WidgetBlueprint || !UtilitySubsystem)
        {
            UE_LOG(LogTemp, Error, TEXT("Enemy Creation Wizard: could not open %s."), WizardAssetObjectPath);
            return false;
        }

        UtilitySubsystem->SpawnAndRegisterTab(WidgetBlueprint);
        return true;
    }

    TSharedPtr<FEnemyWizardPIEInputProcessor> InputProcessor;
    FDelegateHandle AssetRegistryFilesLoadedHandle;
    FDelegateHandle ToolMenusStartupHandle;
    FTSTicker::FDelegateHandle StartupTickerHandle;
};

bool FEnemyWizardPIEInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.IsRepeat() || InKeyEvent.GetKey() != EKeys::K)
    {
        return false;
    }

    if (!GEditor || !GEditor->PlayWorld || HasTextEntryFocus(SlateApp))
    {
        return false;
    }

    if (UEnemyCreationWizardRuntimeLibrary::ToggleEnemyCreationWizardOverlay())
    {
        return true;
    }

    return UEnemyCreationWizardRuntimeLibrary::OpenEnemyCreationWizard();
}

IMPLEMENT_MODULE(FEnemyCreationWizardEditorModule, EnemyCreationWizardEditor)

#undef LOCTEXT_NAMESPACE
