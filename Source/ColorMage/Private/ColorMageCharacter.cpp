#include "ColorMageCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ColorMagePlayerState.h"
#include "ColorProjectile.h"
#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"

AColorMageCharacter::AColorMageCharacter()
{
	// --- [!! MODIFIED !!] ---
	// Enable Tick() so our smooth zoom will work
	PrimaryActorTick.bCanEverTick = true; 
	DefaultGravityScale = 1.0f;
}

void AColorMageCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultGravityScale = MoveComp->GravityScale;
	}
	
	CameraSpringArm = FindComponentByClass<USpringArmComponent>();
	if (CameraSpringArm)
	{
		// SET the component's values FROM our UPROPERTY defaults
		CameraSpringArm->TargetArmLength = DefaultCameraDist;
		CameraSpringArm->SocketOffset = DefaultCameraOffset;

		// --- [!! NEW !!] ---
		// Initialize the interpolation targets
		TargetArmLength = DefaultCameraDist;
		TargetSocketOffset = DefaultCameraOffset;

		// --- [!! THIS IS THE FIX !!] ---
		// Disable the SpringArm's built-in lag.
		// This gives a crisp, responsive follow-camera.
		CameraSpringArm->bEnableCameraLag = false;
		CameraSpringArm->bEnableCameraRotationLag = false;
	}
}

// --- [!! NEW FUNCTION !!] ---
/** Called every frame */
void AColorMageCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- Smooth Zoom Logic ---
	if (CameraSpringArm)
	{
		// Smoothly interpolate the Arm Length
		CameraSpringArm->TargetArmLength = FMath::FInterpTo(
			CameraSpringArm->TargetArmLength, // Current
			TargetArmLength,                  // Target
			DeltaTime,
			ZoomInterpSpeed
		);

		// Smoothly interpolate the Socket Offset
		CameraSpringArm->SocketOffset = FMath::VInterpTo(
			CameraSpringArm->SocketOffset,    // Current
			TargetSocketOffset,               // Target
			DeltaTime,
			ZoomInterpSpeed
		);
	}
}


void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind all character-specific actions
		if (JumpAction)
		{
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (DashAction)
		{
			EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnDash);
		}
		if (FireProjectileAction)
		{
			EnhancedInputComp->BindAction(FireProjectileAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnFireProjectile);
		}
		if (AimAction)
		{
			EnhancedInputComp->BindAction(AimAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnAimStarted);
			EnhancedInputComp->BindAction(AimAction, ETriggerEvent::Completed, this, &AColorMageCharacter::OnAimCompleted);
		}
	}
}

// --- Helper Functions for Aiming ---

/** Toggles the character's rotation mode */
void AColorMageCharacter::SetAimRotation(bool bIsAiming)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (bIsAiming)
		{
			MoveComp->bUseControllerDesiredRotation = true;
			MoveComp->bOrientRotationToMovement = false;
		}
		else
		{
			MoveComp->bUseControllerDesiredRotation = false;
			MoveComp->bOrientRotationToMovement = true;
		}
	}
}

// --- [!! MODIFIED !!] ---
/** Toggles the camera's zoom level by setting the *target* for Tick() */
void AColorMageCharacter::SetAimZoom(bool bIsZooming)
{
	// We no longer "snap" the camera. We just set the goal.
	// The Tick() function will handle the smooth movement.
	if (bIsZooming)
	{
		TargetArmLength = AimingCameraDist;
		TargetSocketOffset = AimingCameraOffset;
	}
	else
	{
		TargetArmLength = DefaultCameraDist;
		TargetSocketOffset = DefaultCameraOffset;
	}
}

/** Called by the timer to turn off hip-fire rotation */
void AColorMageCharacter::ResetHipFireRotation()
{
	if (!bIsManuallyAiming)
	{
		SetAimRotation(false);
	}
}

// --- Aiming Input Handlers ---

// Called when RMB is PRESSED
void AColorMageCharacter::OnAimStarted()
{
	bIsManuallyAiming = true;
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoAimReset);
	SetAimRotation(true); // Lock Rotation
	SetAimZoom(true);     // Set "Zoom In" target
}

// Called when RMB is RELEASED
void AColorMageCharacter::OnAimCompleted()
{
	bIsManuallyAiming = false;
	SetAimRotation(false); // Unlock Rotation
	SetAimZoom(false);     // Set "Zoom Out" target
}


// --- Fire Projectile Logic ---

// Called when LMB is PRESSED
void AColorMageCharacter::OnFireProjectile()
{
	bool bIsAlreadyInAimRotation = bIsManuallyAiming || GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_AutoAimReset);

	if (!bIsAlreadyInAimRotation)
	{
		// This is a "hip-fire" shot. JUST lock the rotation.
		SetAimRotation(true);
	}

	if (!bIsManuallyAiming)
	{
		// Reset the "auto-exit" timer every time we hip-fire
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoAimReset); 
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_AutoAimReset,
			this,
			&AColorMageCharacter::ResetHipFireRotation,
			0.75f, 
			false
		);
	}

	// --- Projectile Spawning Code (Unchanged) ---
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	AColorMagePlayerState* PS = PC->GetPlayerState<AColorMagePlayerState>();
	if (!PS) return;
	EColor ColorToFire = PS->GetCurrentColor();
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: ProjectileClass is not set in BP_ColorMageCharacter."));
		return;
	}
	FVector SpawnLocation = GetMesh()->GetSocketLocation(ProjectileSpawnSocketName);
	FRotator SpawnRotation = PC->GetControlRotation(); 
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AColorProjectile* Projectile = GetWorld()->SpawnActor<AColorProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);
	if (Projectile)
	{
		Projectile->SetProjectileColor(ColorToFire);
	}
}

// --- Dash Logic (Unchanged) ---
void AColorMageCharacter::OnDash()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || MoveComp->GravityScale != DefaultGravityScale) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DashMontage)
	{
		AnimInstance->Montage_Play(DashMontage);
	}
	
	const float DashSpeed = DashDistance / DashDuration;
	const FVector DashVelocity = GetActorForwardVector();
	MoveComp->GravityScale = 0.0f;
	LaunchCharacter(DashVelocity, true, true);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashFinished);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_DashFinished,
		this,
		&AColorMageCharacter::OnDashFinished,
		DashDuration,
		false
	);
}

void AColorMageCharacter::OnDashFinished()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	MoveComp->GravityScale = DefaultGravityScale;
	MoveComp->StopMovementImmediately();
}