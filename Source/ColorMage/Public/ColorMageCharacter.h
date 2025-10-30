// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "PawnControlType.h"
#include "GameFramework/Character.h"
#include "ColorMageCharacter.generated.h"

class USpringArmComponent;
class AColorProjectile;

UCLASS()
class COLORMAGE_API AColorMageCharacter : public ACharacter
{

	GENERATED_BODY()

public:
	AColorMageCharacter();
	// [!! 已修改 !!] Tick 已移除
	// virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "UI")
	EPawnControlType GetControlType() const { return ControlType; }

protected:
	// --- [!! GDD 修正：输入 !!] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireProjectileAction; // (LMB)
	// [!! 已移除 !!] AimAction
	// --- [!! GDD 修正结束 !!] ---

	// --- [!! GDD 修正：简化摄像机 !!] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraDist = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector DefaultCameraOffset = FVector(0.0f, 60.0f, 40.0f);
	// [!! 已移除 !!] AimingCameraDist, AimingCameraOffset, ZoomInterpSpeed, HipFireRotationSpeed
	// --- [!! GDD 修正结束 !!] ---

	// ... (Dash 和 Projectile Config 保持不变) ...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AColorProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName ProjectileSpawnSocketName = "BrushTip";

	/** 角色 UI 控制类型 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPawnControlType ControlType = EPawnControlType::Character;

private:
	// --- [!! GDD 修正：已清理 !!] ---
	TObjectPtr<USpringArmComponent> CameraSpringArm;
	float DefaultGravityScale;
	FTimerHandle TimerHandle_DashFinished;
	// [!! 已移除 !!] bIsManuallyAiming, bIsLerpingRotation, TargetRotation, TimerHandle_AutoAimReset
	// [!! 已移除 !!] TargetArmLength, TargetSocketOffset
	// --- [!! GDD 修正结束 !!] ---

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	// --- Input Handlers ---
	void OnDash();
	void OnDashFinished();
	void OnFireProjectile();
	// [!! 已移除 !!] OnAimStarted, OnAimCompleted
};
