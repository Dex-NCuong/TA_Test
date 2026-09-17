#pragma once

#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ChasePlayer.generated.h"

UCLASS()
class ENEMYCREATIONWIZARDRUNTIME_API UBTTask_ChasePlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ChasePlayer();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual uint16 GetInstanceMemorySize() const override;

    UPROPERTY(EditAnywhere, Category = "Chase", meta = (ClampMin = "0.0"))
    float AcceptanceRadius = 150.0f;

    UPROPERTY(EditAnywhere, Category = "Chase", meta = (ClampMin = "0.0"))
    float RepathDelay = 2.0f;
};
