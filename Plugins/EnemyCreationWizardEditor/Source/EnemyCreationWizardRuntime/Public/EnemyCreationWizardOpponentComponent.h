#pragma once

#include "Components/ActorComponent.h"
#include "EnemyCreationWizardOpponentComponent.generated.h"

UCLASS(ClassGroup=(EnemyCreationWizard), meta=(BlueprintSpawnableComponent))
class ENEMYCREATIONWIZARDRUNTIME_API UEnemyCreationWizardOpponentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyCreationWizardOpponentComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void InitializeForTarget(AActor* InTargetActor, float InDamagePerHit, float InAttackRange = 180.0f, float InAttackInterval = 1.0f);

private:
    void FaceTarget(const FVector& Direction) const;
    void MoveTowardTarget(const FVector& Direction) const;
    void TryAttackTarget();

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> TargetActor;

    UPROPERTY(EditAnywhere, Category = "Enemy Wizard")
    float DamagePerHit = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy Wizard")
    float AttackRange = 180.0f;

    UPROPERTY(EditAnywhere, Category = "Enemy Wizard")
    float AttackInterval = 1.0f;

    double LastAttackTime = -1000.0;
};
