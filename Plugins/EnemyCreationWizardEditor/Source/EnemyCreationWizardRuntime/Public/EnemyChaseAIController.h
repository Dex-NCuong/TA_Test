#pragma once

#include "AIController.h"
#include "EnemyChaseAIController.generated.h"

class UBehaviorTree;

UCLASS()
class ENEMYCREATIONWIZARDRUNTIME_API AEnemyChaseAIController : public AAIController
{
    GENERATED_BODY()

protected:
    virtual void OnPossess(APawn* InPawn) override;

private:
    UBehaviorTree* GetAssignedBehaviorTree(const APawn* InPawn) const;
};
