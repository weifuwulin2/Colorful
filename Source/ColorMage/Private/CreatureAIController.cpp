#include "CreatureAIController.h"
#include "CreatureCharacter.h"
#include "ColorMageCharacter.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Navigation/PathFollowingComponent.h" 
#include "ColorMageGameMode.h"
#include "NavigationSystem.h"

// (Constructor remains the same)
ACreatureAIController::ACreatureAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
    
	UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	// ... (SightConfig Setup) ...
    SightConfig->SightRadius = DetectionRadius;
    SightConfig->LoseSightRadius = DetectionRadius + 200.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    SightConfig->SetMaxAge(5.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ACreatureAIController::BeginPlay()
{
    Super::BeginPlay();
    AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ACreatureAIController::OnTargetPerceptionUpdated);
    if (PatrolSplineActor) { PatrolSpline = PatrolSplineActor->FindComponentByClass<USplineComponent>(); }
}

// --- [!! CORE FIX: OnPossess !!] ---
void ACreatureAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    ControlledCreature = Cast<ACreatureCharacter>(InPawn);
    if (ControlledCreature)
    {
       // [!! REMOVED !!] We no longer set any rotation here.
       // We let StartAI() set the correct default.
       StartAI();
    }
}
// --- [!! END FIX !!] ---

void ACreatureAIController::OnUnPossess()
{
    StopAI();
    ControlledCreature = nullptr;
    CurrentTarget = nullptr;
    Super::OnUnPossess();
}

// --- [!! CORE FIX: StartAI !!] ---
void ACreatureAIController::StartAI()
{
    if (bAIActive) return;
    bAIActive = true;
    CurrentAIState = EAIState::Patrolling;
    if (ControlledCreature)
    {
        if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
        {
            MoveComp->MaxWalkSpeed = PatrolSpeed;
            // [!!] Set the *only* correct state for pathfinding
            MoveComp->bOrientRotationToMovement = true;
            MoveComp->bUseControllerDesiredRotation = false; 
            MoveComp->SetMovementMode(MOVE_Walking);
        }
    }
    SetNextPatrolTarget();
}
// --- [!! END FIX !!] ---

void ACreatureAIController::StopAI()
{
    if (!bAIActive) return;
    // ... (Clear all timers) ...
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    CurrentTarget = nullptr;
    CurrentAIState = EAIState::Patrolling;
    bAIActive = false;
    StopMovement();
}

void ACreatureAIController::SetPatrolSpline(AActor* SplineActor) { /* ... (Same) ... */ }

// --- [!! CORE FIX: Perception !!] ---
void ACreatureAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!bAIActive || (ControlledCreature && ControlledCreature->GetCurrentState() == ECreatureState::Unified))
    {
       return;
    }
    
    AColorMageCharacter* Player = Cast<AColorMageCharacter>(Actor);
    if (!Player) return;
    
    if (Stimulus.WasSuccessfullySensed())
    {
        CurrentTarget = Player;
        CurrentAIState = EAIState::Chasing;
        GetWorld()->GetTimerManager().ClearTimer(PatrolWaitTimerHandle); 
        if (ControlledCreature)
        {
            if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
            {
                MoveComp->MaxWalkSpeed = ChaseSpeed;
                MoveComp->SetMovementMode(MOVE_Walking);
                // [!!] Set correct rotation state
                MoveComp->bOrientRotationToMovement = true;
                MoveComp->bUseControllerDesiredRotation = false;
            }
        }
       
       // [!! REMOVED !!] SetFocus(Player);
       
        MoveToActor(Player, StrafeRadius); 
    }
    else if (CurrentTarget == Player)
    {
        CurrentTarget = nullptr;
        CurrentAIState = EAIState::Patrolling;
        if (ControlledCreature)
        {
            if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
            {
                MoveComp->MaxWalkSpeed = PatrolSpeed;
                MoveComp->SetMovementMode(MOVE_Walking);
                // [!!] Set correct rotation state
                MoveComp->bOrientRotationToMovement = true;
                MoveComp->bUseControllerDesiredRotation = false;
            }
        }
       
       // [!! REMOVED !!] ClearFocus();
       
        StopMovement(); 
        SetNextPatrolTarget(); 
    }
}
// --- [!! END FIX !!] ---

void ACreatureAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!bAIActive || !ControlledCreature || ControlledCreature->GetCurrentState() == ECreatureState::Unified)
    {
       StopMovement();
       return;
    }
        
    // [!! REMOVED !!] All manual rotation logic.
        
    switch (CurrentAIState)
    {
    case EAIState::Patrolling: UpdatePatrol(); break;
    case EAIState::Chasing: UpdateChasing(); break;
    case EAIState::Strafing: UpdateStrafing(); break;
    case EAIState::Attacking:
    case EAIState::Cooldown:
        // [!! NEW !!] If we are attacking/cooling, we should *still face the player*
        if (CurrentTarget && ControlledCreature)
        {
            // We use SetFocus *only* when standing still
            SetFocus(CurrentTarget, EAIFocusPriority::Gameplay);
            if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
            {
                MoveComp->bOrientRotationToMovement = false;
                MoveComp->bUseControllerDesiredRotation = true;
            }
        }
        break;
    default:
        break;
    }
}

void ACreatureAIController::UpdatePatrol()
{
    if (!PatrolSpline || PatrolSpline->GetNumberOfSplinePoints() < 2) return;
    
    // Check if we've arrived
    if (GetPathFollowingComponent() && GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle)
    {
        if (!GetWorld()->GetTimerManager().IsTimerActive(PatrolWaitTimerHandle))
        {
            GetWorld()->GetTimerManager().SetTimer(
                PatrolWaitTimerHandle, this, &ACreatureAIController::SetNextPatrolTarget, PatrolWaitTime, false
            );
        }
    }
    
    // [!! REMOVED !!] Rotation logic. bOrientRotationToMovement is handling it.
}

void ACreatureAIController::UpdateChasing()
{
    if (!CurrentTarget || !IsValid(CurrentTarget))
    {
        CurrentAIState = EAIState::Patrolling;
        SetNextPatrolTarget();
        return;
    }
    
    // [!! REMOVED !!] Rotation logic. bOrientRotationToMovement is handling it.
    
    // We only need to check distances. The MoveTo command was already given.
    float DistanceToPlayer = FVector::Dist(ControlledCreature->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    if (DistanceToPlayer <= StrafeRadius) 
    {
        StopMovement(); 
        CurrentAIState = EAIState::Strafing;
        GenerateRandomStrafeTarget(); 
    }
    else
    {
        // Re-issue MoveTo command in case player moved
        MoveToActor(CurrentTarget, StrafeRadius); 
    }
}

void ACreatureAIController::UpdateStrafing()
{
    if (!CurrentTarget || !IsValid(CurrentTarget))
    {
        CurrentAIState = EAIState::Patrolling;
        SetNextPatrolTarget();
        return;
    }
    
    // --- [!! CORE FIX 7: Strafing Rotation !!] ---
    // While strafing, we DO want to face the player.
    if (ControlledCreature && ControlledCreature->GetCharacterMovement())
    {
        ControlledCreature->GetCharacterMovement()->bOrientRotationToMovement = false;
        ControlledCreature->GetCharacterMovement()->bUseControllerDesiredRotation = true;
    }
    SetFocus(CurrentTarget, EAIFocusPriority::Gameplay);
    // --- [!! END FIX !!] ---
    
    float DistanceToPlayer = FVector::Dist(ControlledCreature->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    if (DistanceToPlayer > StrafeRadius + 100.0f) 
    {
         CurrentAIState = EAIState::Chasing;
         MoveToActor(CurrentTarget, StrafeRadius);
         GetWorld()->GetTimerManager().ClearTimer(StrafeTimerHandle); 
         return;
    }

    if (DistanceToPlayer <= AttackRadius)
    {
        StopMovement(); 
        ExecuteJumpAttack(); 
        GetWorld()->GetTimerManager().ClearTimer(StrafeTimerHandle);
        return;
    }

    // Check if we finished our strafe path
    if (GetPathFollowingComponent() && GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle)
    {
        if (!GetWorld()->GetTimerManager().IsTimerActive(StrafeTimerHandle))
        {
             GenerateRandomStrafeTarget();
        }
    }
}

void ACreatureAIController::GenerateRandomStrafeTarget()
{
    if (!CurrentTarget) return;
    FVector StrafeTarget = GetRandomPointAroundPlayer(StrafeRadius * 0.8f); 
    if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
    { 
       MoveComp->MaxWalkSpeed = StrafeSpeed; 
       MoveComp->SetMovementMode(MOVE_Walking);
       // [!!] Set correct rotation state
       MoveComp->bOrientRotationToMovement = false;
       MoveComp->bUseControllerDesiredRotation = true;
    }
    SetFocus(CurrentTarget, EAIFocusPriority::Gameplay); // Make sure we keep facing player
    MoveTo(StrafeTarget);
    
    float RandomStrafeTime = FMath::FRandRange(StrafeTimeMin, StrafeTimeMax);
    GetWorld()->GetTimerManager().ClearTimer(StrafeTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        StrafeTimerHandle,
        [this]() 
        {
            if (CurrentAIState == EAIState::Strafing && CurrentTarget && ControlledCreature)
            {
                float DistanceToPlayer = FVector::Dist(ControlledCreature->GetActorLocation(), CurrentTarget->GetActorLocation());
                if (DistanceToPlayer <= AttackRadius) { ExecuteJumpAttack(); }
                else { GenerateRandomStrafeTarget(); }
            }
        },
        RandomStrafeTime, false
    );
}

FVector ACreatureAIController::GetRandomPointAroundPlayer(float Radius)
{
    if (!CurrentTarget) { return ControlledCreature ? ControlledCreature->GetActorLocation() : FVector::ZeroVector; }
    FVector PlayerLocation = CurrentTarget->GetActorLocation();
    float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
    float RandomDistance = FMath::FRandRange(Radius * 0.5f, Radius);
    FVector RandomDirection = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);
    FVector TargetLocation = PlayerLocation + (RandomDirection * RandomDistance);
    FNavLocation NavLocation;
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys && NavSys->GetRandomPointInNavigableRadius(TargetLocation, 100.0f, NavLocation))
    { return NavLocation.Location; }
    return PlayerLocation; 
}

void ACreatureAIController::SetNextPatrolTarget()
{
    if (!PatrolSpline || PatrolSpline->GetNumberOfSplinePoints() < 2) return;
	
    // --- [!! CORE FIX 6: Patrol Rotation !!] ---
	if (ControlledCreature && ControlledCreature->GetCharacterMovement())
	{
		ControlledCreature->GetCharacterMovement()->bOrientRotationToMovement = true;
		ControlledCreature->GetCharacterMovement()->bUseControllerDesiredRotation = false;
		ControlledCreature->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	ClearFocus(EAIFocusPriority::Gameplay); 
    // --- [!! END FIX !!] ---
	
    int32 TotalPoints = PatrolSpline->GetNumberOfSplinePoints();
    // ... (Patrol index logic) ...
    if (bPatrolForward) { CurrentPatrolIndex++; } else { CurrentPatrolIndex--; }
    if (CurrentPatrolIndex >= TotalPoints) { CurrentPatrolIndex = TotalPoints - 2; bPatrolForward = false; }
    if (CurrentPatrolIndex < 0) { CurrentPatrolIndex = 1; bPatrolForward = true; }
    CurrentPatrolTarget = PatrolSpline->GetLocationAtSplinePoint(CurrentPatrolIndex, ESplineCoordinateSpace::World);
    
    MoveTo(CurrentPatrolTarget); 
}

// --- [!! Fake Jump Attack Logic (Unchanged from No. 61) !!] ---
void ACreatureAIController::ExecuteJumpAttack()
{
    if (!CurrentTarget || !ControlledCreature || CurrentAIState == EAIState::Attacking || CurrentAIState == EAIState::Cooldown)
        return;
    CurrentAIState = EAIState::Attacking;
    StopMovement(); 
    LockedJumpTarget = CurrentTarget->GetActorLocation();
    ControlledCreature->PlayJumpAttackWindUp();
    GetWorld()->GetTimerManager().ClearTimer(JumpAttackDelayTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        JumpAttackDelayTimerHandle, this, &ACreatureAIController::ExecuteJumpTakeOff, JumpAttackWindUpTime, false
    );
}

void ACreatureAIController::ExecuteJumpTakeOff()
{
	if (!ControlledCreature || !CurrentTarget) { StartAttackCooldown(); return; }
	if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Flying); 
		MoveComp->GravityScale = 0.0f; 
	}
	ControlledCreature->PlayJumpAttackTravel(); 
	JumpStartLocation = ControlledCreature->GetActorLocation();
	JumpStartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().ClearTimer(JumpAttackTickTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		JumpAttackTickTimerHandle, this, &ACreatureAIController::UpdateJumpMovement, GetWorld()->GetDeltaSeconds(), true 
	);
}

void ACreatureAIController::UpdateJumpMovement()
{
	if (!ControlledCreature)
	{
		GetWorld()->GetTimerManager().ClearTimer(JumpAttackTickTimerHandle);
		StartAttackCooldown();
		return;
	}
	float ElapsedTime = GetWorld()->GetTimeSeconds() - JumpStartTime;
	float Alpha = FMath::Clamp(ElapsedTime / JumpAttackTravelTime, 0.0f, 1.0f);
	FVector NewLocation = FMath::Lerp(JumpStartLocation, LockedJumpTarget, Alpha);
	float Arc = FMath::Sin(Alpha * PI) * JumpAttackArcHeight;
	NewLocation.Z += Arc; 
	ControlledCreature->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
	if (Alpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(JumpAttackTickTimerHandle);
		PerformAreaAttack();
	}
}

void ACreatureAIController::PerformAreaAttack()
{
    if (!ControlledCreature) return;
	if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Falling); 
		MoveComp->GravityScale = 1.0f;
	}
    ControlledCreature->PlayJumpAttackLand();
    // ... (Area damage logic) ...
    TArray<FHitResult> HitResults; FVector AttackLocation = ControlledCreature->GetActorLocation(); FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius); FCollisionQueryParams QueryParams; QueryParams.AddIgnoredActor(ControlledCreature);
    bool bHit = GetWorld()->SweepMultiByChannel(HitResults, AttackLocation, AttackLocation, FQuat::Identity, ECC_Pawn, Sphere, QueryParams);
    if (bHit) { /* ... Damage player ... */ }
    StartAttackCooldown();
}

void ACreatureAIController::StartAttackCooldown()
{
    CurrentAIState = EAIState::Cooldown;
    GetWorld()->GetTimerManager().SetTimer(
        CooldownTimerHandle,
        [this]() {
            if (!ControlledCreature) return;
            UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement();
            if (!MoveComp) return;

            if (CurrentTarget && IsValid(CurrentTarget))
            {
				float DistanceToPlayer = FVector::Dist(ControlledCreature->GetActorLocation(), CurrentTarget->GetActorLocation());
				if (DistanceToPlayer <= StrafeRadius)
				{
					CurrentAIState = EAIState::Strafing;
					MoveComp->MaxWalkSpeed = StrafeSpeed;
                    MoveComp->bOrientRotationToMovement = false; // Set strafe rotation
                    MoveComp->bUseControllerDesiredRotation = true;
                    SetFocus(CurrentTarget, EAIFocusPriority::Gameplay);
					GenerateRandomStrafeTarget(); 
				}
				else
				{
					CurrentAIState = EAIState::Chasing; 
					MoveComp->MaxWalkSpeed = ChaseSpeed;
                    MoveComp->bOrientRotationToMovement = true; // Set chase rotation
                    MoveComp->bUseControllerDesiredRotation = false;
                    ClearFocus(EAIFocusPriority::Gameplay);
					MoveToActor(CurrentTarget, StrafeRadius); 
				}
            }
            else
            {
                CurrentAIState = EAIState::Patrolling;
                MoveComp->MaxWalkSpeed = PatrolSpeed;
                MoveComp->bOrientRotationToMovement = true; // Set patrol rotation
                MoveComp->bUseControllerDesiredRotation = false;
                ClearFocus(EAIFocusPriority::Gameplay);
                SetNextPatrolTarget();
            }
        },
        AttackCooldown, false
    );
}