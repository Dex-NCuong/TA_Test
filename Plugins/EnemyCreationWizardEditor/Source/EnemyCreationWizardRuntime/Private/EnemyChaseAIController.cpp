#include "EnemyChaseAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "UObject/UnrealType.h"

void AEnemyChaseAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    bAllowStrafe = false;
    if (UBehaviorTree* BehaviorTree = GetAssignedBehaviorTree(InPawn))
    {
        RunBehaviorTree(BehaviorTree);
    }
}

UBehaviorTree* AEnemyChaseAIController::GetAssignedBehaviorTree(const APawn* InPawn) const
{
    if (!InPawn)
    {
        return nullptr;
    }

    for (const FName PropertyName : { TEXT("BehaviorTreeAsset"), TEXT("BehaviorTree") })
    {
        if (const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(InPawn->GetClass(), PropertyName))
        {
            return Cast<UBehaviorTree>(Property->GetObjectPropertyValue_InContainer(InPawn));
        }
    }

    return nullptr;
}
