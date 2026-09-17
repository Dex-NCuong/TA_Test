#include "BTTask_ChasePlayer.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
    struct FChasePlayerTaskMemory
    {
        float ElapsedSinceTargetMoved = 0.0f;
        FVector LastTargetLocation = FVector::ZeroVector;
        bool bTargetMoved = false;
    };
}

UBTTask_ChasePlayer::UBTTask_ChasePlayer()
{
    NodeName = TEXT("Look At and Chase Player");
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_ChasePlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* Controller = OwnerComp.GetAIOwner();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(OwnerComp.GetWorld(), 0);
    if (!Controller || !PlayerPawn)
    {
        return EBTNodeResult::Failed;
    }

    Controller->SetFocus(PlayerPawn);
    FChasePlayerTaskMemory& Memory = *reinterpret_cast<FChasePlayerTaskMemory*>(NodeMemory);
    Memory = {};
    Memory.LastTargetLocation = PlayerPawn->GetActorLocation();
    // Chase the recorded position so target movement can be deliberately delayed.
    const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(Memory.LastTargetLocation, AcceptanceRadius, true);
    if (MoveResult == EPathFollowingRequestResult::Failed)
    {
        return EBTNodeResult::Failed;
    }
    return EBTNodeResult::InProgress;
}

void UBTTask_ChasePlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    FChasePlayerTaskMemory& Memory = *reinterpret_cast<FChasePlayerTaskMemory*>(NodeMemory);
    AAIController* Controller = OwnerComp.GetAIOwner();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(OwnerComp.GetWorld(), 0);
    if (!Controller || !PlayerPawn)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    Controller->SetFocus(PlayerPawn);
    const FVector CurrentTargetLocation = PlayerPawn->GetActorLocation();
    if (!Memory.bTargetMoved && !CurrentTargetLocation.Equals(Memory.LastTargetLocation, AcceptanceRadius))
    {
        Memory.bTargetMoved = true;
        Memory.ElapsedSinceTargetMoved = 0.0f;
    }

    if (!Memory.bTargetMoved)
    {
        return;
    }

    Memory.ElapsedSinceTargetMoved += DeltaSeconds;
    if (Memory.ElapsedSinceTargetMoved < RepathDelay)
    {
        return;
    }

    Memory.LastTargetLocation = CurrentTargetLocation;
    Memory.bTargetMoved = false;
    const EPathFollowingRequestResult::Type MoveResult = Controller->MoveToLocation(Memory.LastTargetLocation, AcceptanceRadius, true);
    if (MoveResult == EPathFollowingRequestResult::Failed)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
    }
}

uint16 UBTTask_ChasePlayer::GetInstanceMemorySize() const
{
    return sizeof(FChasePlayerTaskMemory);
}
