#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "EditorUtilityWidget.h"
#include "EnemyCreationWizardLibrary.generated.h"

UCLASS()
class ENEMYCREATIONWIZARDEDITOR_API UEnemyCreationWizardLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Creates a BP_BaseEnemy child from EUW_EnemyCreationWizard.
     * Native UEnemyCreationWizardWidget selections are preferred for base directory, Skeletal Mesh,
     * arbitrary material slots, and Behavior Tree; named legacy controls remain supported as a fallback.
     */
    UFUNCTION(BlueprintCallable, Category = "Enemy Creation Wizard")
    static bool CreateEnemyFromWizard(UEditorUtilityWidget* Wizard);
};
