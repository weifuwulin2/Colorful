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
	// ... (JumpAction, DashAction, FireProjectileAction, AimAction) ...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireProjectileAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;

	// --- Camera Config ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraDist = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector DefaultCameraOffset = FVector(0.0f, 60.0f, 40.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float AimingCameraDist = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector AimingCameraOffset = FVector(0.0f, 70.0f, 50.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float ZoomInterpSpeed = 15.0f;
	/** "Hip-Fire" (非瞄准) 时的“向前”偏移值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float HipFireForwardOffset = 730.0f; // <-- 偏移多一些

	/** "Aiming" (瞄准) 时的“向前”偏移值 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AimingForwardOffset = 400.0f;
	
	/** How fast the character 'lerps' to the hip-fire direction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float HipFireRotationSpeed = 10.0f; // <--- [!! NEW !!]

	// ... (Dash and Projectile Config) ...
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


private:
	// --- Cached Components & Timers ---
	TObjectPtr<USpringArmComponent> CameraSpringArm;
	float DefaultGravityScale;
	FTimerHandle TimerHandle_DashFinished;

	// [!! REMOVED !!] - TimerHandle_AutoAimReset is gone.

	/** Tracks if the player is holding the Aim button */
	bool bIsManuallyAiming = false;
	
	/** Tracks if we are currently lerping our rotation from a hip-fire */
	bool bIsLerpingRotation = false; // <--- [!! NEW !!]
	
	/** The rotation we are trying to lerp to */
	FRotator TargetRotation; // <--- [!! NEW !!]

	// --- Target values for smooth zoom ---
	float TargetArmLength;
	FVector TargetSocketOffset;


protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- Input Handlers ---
	void OnDash();
	void OnDashFinished();
	void OnFireProjectile();
	void OnAimStarted();
	void OnAimCompleted();

	// --- Helper Functions ---
	/** HELPER: Toggles rotation lock (bUseControllerDesiredRotation) */
	void SetAimRotation(bool bIsAiming);
	
	/** HELPER: Toggles camera zoom (TargetArmLength) */
	void SetAimZoom(bool bIsZooming);

	// [!! REMOVED !!] - ResetHipFireRotation is gone.
};
