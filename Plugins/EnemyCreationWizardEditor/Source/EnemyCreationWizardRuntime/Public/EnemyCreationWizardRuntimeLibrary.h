#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyCreationWizardRuntimeLibrary.generated.h"

UCLASS()
class ENEMYCREATIONWIZARDRUNTIME_API UEnemyCreationWizardRuntimeLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Opens the Enemy Creation Wizard in an editor tab while running in the editor.
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Creation Wizard")
    static bool OpenEnemyCreationWizard();

    /**
     * Toggles the Enemy Creation Wizard as an in-viewport overlay while PIE is running in the editor.
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Creation Wizard")
    static bool ToggleEnemyCreationWizardOverlay();
};
