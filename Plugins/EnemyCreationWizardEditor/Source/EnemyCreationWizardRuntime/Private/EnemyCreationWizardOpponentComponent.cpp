#include "EnemyCreationWizardOpponentComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UEnemyCreationWizardOpponentComponent::UEnemyCreationWizardOpponentComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UEnemyCreationWizardOpponentComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!TargetActor.IsValid())
    {
        TargetActor = UGameplayStatics::GetPlayerPawn(this, 0);
    }
}

void UEnemyCreationWizardOpponentComponent::InitializeForTarget(AActor* InTargetActor, const float InDamagePerHit, const float InAttackRange, const float InAttackInterval)
{
    TargetActor = InTargetActor;
    DamagePerHit = InDamagePerHit;
    AttackRange = FMath::Max(50.0f, InAttackRange);
    AttackInterval = FMath::Max(0.2f, InAttackInterval);
}

void UEnemyCreationWizardOpponentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* OwnerActor = GetOwner();
    AActor* CurrentTarget = TargetActor.Get();
    if (!OwnerActor || !CurrentTarget)
    {
        return;
    }

    const FVector ToTarget = CurrentTarget->GetActorLocation() - OwnerActor->GetActorLocation();
    const FVector HorizontalToTarget(ToTarget.X, ToTarget.Y, 0.0f);
    const float DistanceToTarget = HorizontalToTarget.Size();
    if (DistanceToTarget <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector Direction = HorizontalToTarget / DistanceToTarget;
    FaceTarget(Direction);

    if (DistanceToTarget > AttackRange)
    {
        MoveTowardTarget(Direction);
        return;
    }

    TryAttackTarget();
}

void UEnemyCreationWizardOpponentComponent::FaceTarget(const FVector& Direction) const
{
    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->SetActorRotation(Direction.Rotation());
    }
}

void UEnemyCreationWizardOpponentComponent::MoveTowardTarget(const FVector& Direction) const
{
    if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
        {
            MovementComponent->MaxWalkSpeed = FMath::Max(150.0f, MovementComponent->MaxWalkSpeed);
        }

        OwnerCharacter->AddMovementInput(Direction, 1.0f, true);
        return;
    }

    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->AddActorWorldOffset(Direction * 160.0f * GetWorld()->GetDeltaSeconds(), true);
    }
}

void UEnemyCreationWizardOpponentComponent::TryAttackTarget()
{
    AActor* OwnerActor = GetOwner();
    AActor* CurrentTarget = TargetActor.Get();
    UWorld* World = GetWorld();
    if (!OwnerActor || !CurrentTarget || !World)
    {
        return;
    }

    const double CurrentTime = World->GetTimeSeconds();
    if (CurrentTime - LastAttackTime < AttackInterval)
    {
        return;
    }

    LastAttackTime = CurrentTime;
    UGameplayStatics::ApplyDamage(CurrentTarget, DamagePerHit, nullptr, OwnerActor, nullptr);
}
