// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
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
	virtual void Tick(float DeltaTime) override;

protected:
	// --- Input Assets ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireProjectileAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	// --- Camera Default Values ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraDist = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector DefaultCameraOffset = FVector(0.0f, 60.0f, 40.0f);

	// --- Camera Aiming Values ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float AimingCameraDist = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector AimingCameraOffset = FVector(0.0f, 70.0f, 50.0f);

	/** How fast the camera zooms in and out. Higher = faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ZoomInterpSpeed = 15.0f; 
	
	// --- Dash Config ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.25f;

	// --- Projectile Config ---
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AColorProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName ProjectileSpawnSocketName = "BrushTip";

private:
	// --- Cached Components & Timers ---
	TObjectPtr<USpringArmComponent> CameraSpringArm;
	float DefaultGravityScale;
	FTimerHandle TimerHandle_DashFinished;

	// ... (private variables)
	FTimerHandle TimerHandle_AutoAimReset;
	bool bIsManuallyAiming = false;
	
	// --- [!! NEW: Target values for smooth zoom !!] ---
	float TargetArmLength;
	FVector TargetSocketOffset;
protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- Input Handlers ---
	void OnDash();
	void OnDashFinished();
	void OnFireProjectile();
	void OnAimStarted();  // This is your RMB Press
	void OnAimCompleted(); // This is your RMB Release

	// --- [!! NEW REFACTORED FUNCTIONS !!] ---

	/** HELPER: Toggles rotation lock (bUseControllerDesiredRotation) */
	void SetAimRotation(bool bIsAiming);
	
	/** HELPER: Toggles camera zoom (TargetArmLength) */
	void SetAimZoom(bool bIsZooming);

	/** HELPER: Called by timer to reset rotation after hip-fire */
	void ResetHipFireRotation();
};
