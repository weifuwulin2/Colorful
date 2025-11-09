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
#include "Components/CapsuleComponent.h" // [!! FIX !!] 确保我们能获取到胶囊体

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

void ACreatureAIController::SetPatrolSpline(AActor* SplineActor) 
{
    PatrolSplineActor = SplineActor;
    if (PatrolSplineActor)
    {
        PatrolSpline = PatrolSplineActor->FindComponentByClass<USplineComponent>();
    }
}

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

    // [!! FIX !!] Added checks for bCanAttack and bJumpAttackOnCooldown
    if (DistanceToPlayer <= AttackRadius)
    {
        if (bCanAttack && !bJumpAttackOnCooldown)
        {
            StopMovement(); 
            ExecuteJumpAttack(); 
            GetWorld()->GetTimerManager().ClearTimer(StrafeTimerHandle);
            return;
        }
        // If on cooldown, do nothing and continue strafing
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
                // [!! FIX !!] Added bCanAttack and bJumpAttackOnCooldown checks
                if (DistanceToPlayer <= AttackRadius && bCanAttack && !bJumpAttackOnCooldown) 
                { 
                    ExecuteJumpAttack(); 
                }
                else 
                { 
                    GenerateRandomStrafeTarget(); // Get new strafe point if not attacking or on cooldown
                }
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
void ACreatureAIController::ExecuteJumpAttack()
{
    // [!! FIX !!] Added !bCanAttack and bJumpAttackOnCooldown to the guard
    if (!CurrentTarget || !ControlledCreature || !bCanAttack || bJumpAttackOnCooldown ||
        CurrentAIState == EAIState::Attacking || CurrentAIState == EAIState::Cooldown)
        return;
    
    // 重要：设置攻击标志防止重复攻击
    bCanAttack = false;
    bJumpAttackOnCooldown = true; // [!! FIX !!] This attack is now on cooldown
    CurrentAIState = EAIState::Attacking;
    StopMovement();
    
    // 获取玩家位置
    FVector TargetLocation = CurrentTarget->GetActorLocation();

    // [!! FIX !!] Calculate target position relative to creature
    // Get direction from creature to player (2D)
    FVector DirectionToPlayer = (TargetLocation - ControlledCreature->GetActorLocation()).GetSafeNormal2D();
    
    // Land 200 units *behind* the player (from the creature's attack direction)
    // This makes the creature jump "over" the player's initial position.
    // This avoids landing on the player's head.
    FVector JumpTarget = TargetLocation + (DirectionToPlayer * 200.0f);
    
    // [!! OLD CODE !!]
    // FVector PlayerForward = CurrentTarget->GetActorForwardVector();
    // FVector JumpTarget = TargetLocation + PlayerForward * 200.0f;
    
    // 重要：使用 JumpTarget 而不是 TargetLocation 进行地面检测
    FVector TraceStart = JumpTarget + FVector(0, 0, 1000.0f);  // 修复：使用 JumpTarget
    FVector TraceEnd = JumpTarget - FVector(0, 0, 1000.0f);     // 修复：使用 JumpTarget
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ControlledCreature);
    QueryParams.AddIgnoredActor(CurrentTarget);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        LockedJumpTarget = HitResult.Location;
    }
    else
    {
        // 修复：使用 JumpTarget 而不是 TargetLocation
        LockedJumpTarget = FVector(JumpTarget.X, JumpTarget.Y, 0.0f);
    }
    
    GetWorld()->GetTimerManager().ClearTimer(JumpAttackDelayTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        JumpAttackDelayTimerHandle, this, &ACreatureAIController::ExecuteJumpTakeOff, JumpAttackWindUpTime, false
    );
}

void ACreatureAIController::ExecuteJumpTakeOff()
{
    if (!ControlledCreature || !CurrentTarget) 
    { 
        bCanAttack = true; // 重置攻击标志
        StartAttackCooldown(); 
        return; 
    }
    
    // 确保仍在攻击状态
    if (CurrentAIState != EAIState::Attacking)
    {
        bCanAttack = true;
        return;
    }
    
    // [!! FIX !!] 关闭碰撞，让怪物可以穿过玩家
    if (UCapsuleComponent* Capsule = ControlledCreature->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    
    if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
        MoveComp->SetMovementMode(MOVE_Flying); 
        MoveComp->GravityScale = 0.0f; 
    }
    
    ControlledCreature->PlayJumpAttackLand();
    
    JumpStartLocation = ControlledCreature->GetActorLocation();
    JumpStartTime = GetWorld()->GetTimeSeconds();
    
    GetWorld()->GetTimerManager().ClearTimer(JumpAttackTickTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        JumpAttackTickTimerHandle, this, &ACreatureAIController::UpdateJumpMovement, 0.016f, true
    );
}

void ACreatureAIController::UpdateJumpMovement()
{
    if (!ControlledCreature || CurrentAIState != EAIState::Attacking)
    {
        GetWorld()->GetTimerManager().ClearTimer(JumpAttackTickTimerHandle);
        if (ControlledCreature)
        {
            // 恢复正常移动
            if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
            {
                MoveComp->SetMovementMode(MOVE_Walking);
                MoveComp->GravityScale = 1.0f;
            }
        }
        bCanAttack = true;
        StartAttackCooldown();
        return;
    }
    
    float ElapsedTime = GetWorld()->GetTimeSeconds() - JumpStartTime;
    float Alpha = FMath::Clamp(ElapsedTime / JumpAttackTravelTime, 0.0f, 1.0f);
    
    FVector NewLocation = FMath::Lerp(JumpStartLocation, LockedJumpTarget, Alpha);
    float Arc = FMath::Sin(Alpha * PI) * JumpAttackArcHeight;
    NewLocation.Z += Arc;
    
    ControlledCreature->SetActorLocation(NewLocation, true, nullptr, ETeleportType::None);
    
    if (Alpha >= 1.0f)
    {
        GetWorld()->GetTimerManager().ClearTimer(JumpAttackTickTimerHandle);
        FVector FinalLocation = LockedJumpTarget;
        ControlledCreature->SetActorLocation(FinalLocation, true, nullptr, ETeleportType::None);
        PerformAreaAttack();
    }
}

void ACreatureAIController::CheckAndExecuteAttack()
{
    if (!CurrentTarget || !ControlledCreature || !bCanAttack || 
        CurrentAIState == EAIState::Attacking || CurrentAIState == EAIState::Cooldown)
            return;
        
    float DistanceToTarget = FVector::Dist(ControlledCreature->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    if (DistanceToTarget <= AttackRadius)
    {
        // [!! FIX !!] Using AttackRadius here since JumpAttackRadius isn't in the .h
        // And adding the bJumpAttackOnCooldown check
        if (DistanceToTarget <= AttackRadius && !bJumpAttackOnCooldown)
        {
            ExecuteJumpAttack();
        }
        // 如果跳跃攻击在冷却中，就不攻击，继续移动
    }
}
void ACreatureAIController::PerformAreaAttack()
{
    if (!ControlledCreature) return;

    // [!! FIX !!] 落地后，立刻恢复碰撞
    if (UCapsuleComponent* Capsule = ControlledCreature->GetCapsuleComponent())
    {
        // 恢复为Pawn的标准碰撞
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
    
    // 恢复正常移动模式
    if (UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement())
    {
        MoveComp->SetMovementMode(MOVE_Walking);
        MoveComp->GravityScale = 1.0f;
    }
    
    // 区域伤害逻辑 - 使用更大的攻击范围来补偿跳跃位置的偏移
    TArray<FHitResult> HitResults;
    FVector AttackLocation = ControlledCreature->GetActorLocation();
    FCollisionShape Sphere = FCollisionShape::MakeSphere(AttackRadius * 1.5f); // 增加攻击范围
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ControlledCreature);
    
    bool bHit = GetWorld()->SweepMultiByChannel(HitResults, AttackLocation, AttackLocation, FQuat::Identity, ECC_Pawn, Sphere, QueryParams);
    if (bHit) 
    { 
        for (const FHitResult& Hit : HitResults)
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                UE_LOG(LogTemp, Warning, TEXT("Jump attack hit: %s"), *HitActor->GetName());
            }
        }
    }
    
    StartAttackCooldown();
}


void ACreatureAIController::StartAttackCooldown()
{
    CurrentAIState = EAIState::Cooldown;
    
    // 立即根据距离进入相应状态
    if (CurrentTarget && IsValid(CurrentTarget))
    {
        UCharacterMovementComponent* MoveComp = ControlledCreature->GetCharacterMovement();
        if (MoveComp)
        {
            float DistanceToPlayer = FVector::Dist(ControlledCreature->GetActorLocation(), CurrentTarget->GetActorLocation());
            
            if (DistanceToPlayer <= StrafeRadius)
            {
                CurrentAIState = EAIState::Strafing;
                MoveComp->MaxWalkSpeed = StrafeSpeed;
                MoveComp->bOrientRotationToMovement = false;
                MoveComp->bUseControllerDesiredRotation = true;
                SetFocus(CurrentTarget, EAIFocusPriority::Gameplay);
                GenerateRandomStrafeTarget();
            }
            else
            {
                CurrentAIState = EAIState::Chasing;
                MoveComp->MaxWalkSpeed = ChaseSpeed;
                MoveComp->bOrientRotationToMovement = true;
                MoveComp->bUseControllerDesiredRotation = false;
                ClearFocus(EAIFocusPriority::Gameplay);
                MoveToActor(CurrentTarget, StrafeRadius);
            }
        }
    }
    
    // [!! FIX !!] Start TWO separate cooldown timers

    // 1. General attack cooldown (bCanAttack)
    GetWorld()->GetTimerManager().SetTimer(
        CooldownTimerHandle,
        [this]() {
            bCanAttack = true;
            // 重要：不要在这里立即检查攻击！
            UE_LOG(LogTemp, Warning, TEXT("Attack cooldown finished, can attack again"));
        },
        AttackCooldown, false
    );

    // 2. Jump-specific attack cooldown (bJumpAttackOnCooldown)
    GetWorld()->GetTimerManager().SetTimer(
        JumpAttackSpecialCooldownHandle, // Use the handle from your .h file
        [this]() {
            bJumpAttackOnCooldown = false;
            UE_LOG(LogTemp, Warning, TEXT("Jump Attack special cooldown finished"));
        },
        JumpAttackCooldown, false // Use the new variable you just added to the .h
    );
}