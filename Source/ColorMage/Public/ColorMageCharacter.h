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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float HipFireRotationSpeed = 10.0f;

	// --- Dash Config ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 500.0f;
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

	/** Tracks if the player is holding the Aim button */
	bool bIsManuallyAiming = false;
	
	/** Tracks if we are currently lerping our rotation from a hip-fire */
	bool bIsLerpingRotation = false; 
	
	/** The rotation we are trying to lerp to */
	FRotator TargetRotation;

	// --- Target values for smooth zoom ---
	float TargetArmLength;
	FVector TargetSocketOffset;


protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** 当这个角色被控制器附身时调用 (用于取消隐藏) */
	virtual void PossessedBy(AController* NewController) override;

	// --- Input Handlers ---
	void OnDash();
	void OnDashFinished();
	void OnFireProjectile();
	void OnAimStarted();
	void OnAimCompleted();

	// --- Helper Functions ---
	void SetAimRotation(bool bIsAiming);
	void SetAimZoom(bool bIsZooming);

	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
};
