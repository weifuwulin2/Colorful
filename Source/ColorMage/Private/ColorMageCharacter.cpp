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

		// Initialize the interpolation targets
		TargetArmLength = DefaultCameraDist;
		TargetSocketOffset = DefaultCameraOffset;

		// Disable the SpringArm's built-in lag
		CameraSpringArm->bEnableCameraLag = false;
		CameraSpringArm->bEnableCameraRotationLag = false;
	}
}

void AColorMageCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- [!! NEW LERP LOGIC !!] ---
	// If we are currently in the "hip-fire lerp" state...
	if (bIsLerpingRotation)
	{
		// Smoothly interpolate the Actor's rotation
		SetActorRotation(FMath::RInterpTo(
			GetActorRotation(),
			TargetRotation,
			DeltaTime,
			HipFireRotationSpeed
		));

		// Check if we've reached the target
		if (GetActorRotation().Equals(TargetRotation, 1.0f))
		{
			// We're done. Stop lerping.
			bIsLerpingRotation = false;

			// Re-enable normal movement rotation
			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
			{
				MoveComp->bOrientRotationToMovement = true;
			}
		}
	}
	// --- [!! END LERP LOGIC !!] ---


	// --- Smooth Zoom Logic (Unchanged) ---
	if (CameraSpringArm)
	{
		CameraSpringArm->TargetArmLength = FMath::FInterpTo(
			CameraSpringArm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed
		);
		CameraSpringArm->SocketOffset = FMath::VInterpTo(
			CameraSpringArm->SocketOffset, TargetSocketOffset, DeltaTime, ZoomInterpSpeed
		);
	}
}


void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// ... (Bindings for Jump, Dash, Fire, Aim) ...
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

// --- Helper Functions for Aiming (Simplified) ---

void AColorMageCharacter::SetAimRotation(bool bIsAiming)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = bIsAiming;
		MoveComp->bOrientRotationToMovement = !bIsAiming;
	}
}

void AColorMageCharacter::SetAimZoom(bool bIsZooming)
{
	TargetArmLength = bIsZooming ? AimingCameraDist : DefaultCameraDist;
	TargetSocketOffset = bIsZooming ? AimingCameraOffset : DefaultCameraOffset;
}

// [!! REMOVED !!] - ResetHipFireRotation is gone.

// --- Aiming Input Handlers ---

void AColorMageCharacter::OnAimStarted()
{
	bIsManuallyAiming = true;
	bIsLerpingRotation = false; // Stop any hip-fire lerp, manual aim takes priority
	SetAimRotation(true); 
	SetAimZoom(true);
}

void AColorMageCharacter::OnAimCompleted()
{
	bIsManuallyAiming = false;
	SetAimRotation(false);
	SetAimZoom(false);
}


// --- Fire Projectile Logic ---

// --- [!! THIS IS YOUR SOLUTION !!] ---
void AColorMageCharacter::OnFireProjectile()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// If we are NOT manually aiming...
	if (!bIsManuallyAiming)
	{
		// ...this is a hip-fire.
		// 1. Temporarily disable movement rotation (so it doesn't fight our lerp)
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = false;
		}

		// 2. Set the target for our lerp
		TargetRotation = PC->GetControlRotation();
		TargetRotation.Pitch = 0; // Don't make the character lean
		TargetRotation.Roll = 0;
		
		// 3. Start the lerp (Tick() will take over from here)
		bIsLerpingRotation = true;
	}

	// --- Projectile Spawning Code (Unchanged) ---
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
		ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams
	);
	if (Projectile)
	{
		Projectile->SetProjectileColor(ColorToFire);
	}
}

// --- Dash Logic (Unchanged) ---
void AColorMageCharacter::OnDash()
{
    // ... (Your existing OnDash code)
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
    // ... (Your existing OnDashFinished code)
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;
	MoveComp->GravityScale = DefaultGravityScale;
	MoveComp->StopMovementImmediately();
}