#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Components/SplineComponent.h"
#include "CreatureAIController.generated.h"

class UNiagaraSystem;
class AColorMageCharacter;
class ACreatureCharacter;

UENUM(BlueprintType)
enum class EAIState : uint8 
{
    Patrolling UMETA(DisplayName = "Patrolling"), // 巡逻
    Chasing       UMETA(DisplayName = "Chasing"),    // 追逐
    Strafing   UMETA(DisplayName = "Strafing"),   // [!! 已加回 !!] 平移
    Attacking  UMETA(DisplayName = "Attacking"),  // 攻击中 (执行假跳跃)
    Cooldown   UMETA(DisplayName = "Cooldown")    // 攻击冷却
};

UCLASS()
class COLORMAGE_API ACreatureAIController : public AAIController
{
  GENERATED_BODY()
public:
    ACreatureAIController();
    virtual void Tick(float DeltaTime) override;
    UFUNCTION(BlueprintCallable)
    void SetPatrolSpline(AActor* SplineActor);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override; // Will be fixed
    virtual void OnUnPossess() override;

    // AI & State
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    TObjectPtr<class ACreatureCharacter> ControlledCreature;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    TObjectPtr<AActor> CurrentTarget;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    EAIState CurrentAIState = EAIState::Patrolling;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    bool bAIActive = false;
    void StartAI();
    void StopAI();

    // Perception
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception")
    float DetectionRadius = 2000.0f;
    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    // Patrol
    UPROPERTY(EditInstanceOnly, Category = "AI|Patrol")
    TObjectPtr<AActor> PatrolSplineActor;
    UPROPERTY()
    TObjectPtr<class USplineComponent> PatrolSpline;
    int32 CurrentPatrolIndex = 0;
    bool bPatrolForward = true;
    FVector CurrentPatrolTarget;
    FTimerHandle PatrolWaitTimerHandle;
    UPROPERTY(EditAnywhere, Category = "AI|Patrol")
    float PatrolSpeed = 150.0f;
    UPROPERTY(EditAnywhere, Category = "AI|Patrol")
    float PatrolWaitTime = 3.0f;
    void UpdatePatrol(); // Removed DeltaTime
    void SetNextPatrolTarget();

    // Chase
    UPROPERTY(EditAnywhere, Category = "AI|Chase")
    float ChaseSpeed = 400.0f;
    void UpdateChasing(); // Removed DeltaTime

    // Strafe
    UPROPERTY(EditAnywhere, Category = "AI|Strafe")
    float StrafeRadius = 800.0f;
    UPROPERTY(EditAnywhere, Category = "AI|Strafe")
    float StrafeTimeMin = 1.5f;
    UPROPERTY(EditAnywhere, Category = "AI|Strafe")
    float StrafeTimeMax = 3.0f;
    UPROPERTY(EditAnywhere, Category = "AI|Strafe")
    float StrafeSpeed = 200.0f;
    FTimerHandle StrafeTimerHandle;
    void UpdateStrafing(); // Removed DeltaTime
    void GenerateRandomStrafeTarget();
    FVector GetRandomPointAroundPlayer(float Radius);
    
    // Attack (Fake Jump)
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float AttackRadius = 300.0f;
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float JumpAttackRadius = 600.0f;
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float AttackDamage = 1.0f;
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float AttackCooldown = 3.0f;
    
    // [!! ADDED THIS LINE !!]
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float JumpAttackCooldown = 3.0f; // A separate, longer cooldown for the jump

    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float JumpAttackWindUpTime = 0.5f;
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float JumpAttackTravelTime = 0.5f;
    UPROPERTY(EditAnywhere, Category = "AI|Attack")
    float JumpAttackArcHeight = 700.0f;
    FTimerHandle CooldownTimerHandle;
    FTimerHandle JumpAttackDelayTimerHandle;
    FTimerHandle JumpAttackTickTimerHandle;
    FVector JumpStartLocation;
    FVector LockedJumpTarget;
    float JumpStartTime;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Attack")
    class UNiagaraSystem* JumpAttackLandVFX;

    UNiagaraSystem* ShockWave;
    void ExecuteJumpAttack();
    void ExecuteJumpTakeOff();
    void UpdateJumpMovement();
    void CheckAndExecuteAttack();
    void PerformAreaAttack();
    void StartAttackCooldown();

private:
    bool bCanAttack = true;
    bool bJumpAttackOnCooldown = false;
    FTimerHandle JumpAttackSpecialCooldownHandle;
    bool bJustFinishedJumpAttack = false;
};