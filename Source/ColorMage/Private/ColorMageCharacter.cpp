// ColorMageCharacter.cpp
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
	DefaultGravityScale = 1.0f;
}

void AColorMageCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultGravityScale = MoveComp->GravityScale;
	}
	
	// --- [!! CORRECTED LOGIC !!] ---
	// Find the SpringArm and SET its defaults from our variables
	CameraSpringArm = FindComponentByClass<USpringArmComponent>();
	if (CameraSpringArm)
	{
		CameraSpringArm->TargetArmLength = DefaultCameraDist;
		CameraSpringArm->SocketOffset = DefaultCameraOffset;
	}
}

void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind Jump, Dash, Fire, Aim
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

// --- [!! REFACTORED !!] ---
// Called when RMB is PRESSED
void AColorMageCharacter::OnAimStarted()
{
	bIsManuallyAiming = true; // Mark as manual aim
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoAimReset); // Cancel any auto-exit timer
	EnterAimState(); // Call the shared logic
}

// --- [!! REFACTORED !!] ---
// Called when RMB is RELEASED
void AColorMageCharacter::OnAimCompleted()
{
	bIsManuallyAiming = false; // No longer manually aiming
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoAimReset); // (Good practice)
	ExitAimState(); // Call the shared logic
}

// --- [!! NEW HELPER FUNCTION !!] ---
void AColorMageCharacter::EnterAimState()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->bOrientRotationToMovement = false;
	}
	if (CameraSpringArm)
	{
		// Pro-tip: You can use FMath::FInterpTo for a smooth zoom here
		CameraSpringArm->TargetArmLength = AimingCameraDist;
		CameraSpringArm->SocketOffset = AimingCameraOffset;
	}
}

// --- [!! NEW HELPER FUNCTION with SAFETY CHECK !!] ---
void AColorMageCharacter::ExitAimState()
{
	// This function can be called by RMB release OR the timer.
    // If the timer fires, but we are now MANUALLY holding RMB,
    // we must ignore the timer.
	if (bIsManuallyAiming)
	{
		return; // We are holding RMB, so block the timer's request.
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement = true;
	}
	if (CameraSpringArm)
	{
		CameraSpringArm->TargetArmLength = DefaultCameraDist;
		CameraSpringArm->SocketOffset = DefaultCameraOffset;
	}
}

// --- [!! CORRECTED !!] ---
// Called when LMB is PRESSED
void AColorMageCharacter::OnFireProjectile()
{
	// Check if we are aiming manually OR if an auto-aim timer is already active
	bool bIsAlreadyAiming = bIsManuallyAiming || GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_AutoAimReset);

	// If we are not aiming in any way...
	if (!bIsAlreadyAiming)
	{
		// ...this is the first "hip fire" shot, so enter the aim state.
		EnterAimState();
	}
	
	// --- [!! THIS IS YOUR FIX !!] ---
    // If we are NOT manually aiming...
    if (!bIsManuallyAiming)
    {
        // ...then we are in an "auto-aim" state.
        // Firing a shot should ALWAYS reset the "auto-exit" timer.
        // Clear any old timer...
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoAimReset); 
        
        // ...and set a new one.
        GetWorld()->GetTimerManager().SetTimer(
			TimerHandle_AutoAimReset,
			this,
			&AColorMageCharacter::ExitAimState, // The timer just calls ExitAimState
			1.5f,  // 1.5 seconds from THIS shot
			false
		);
    }
	// --- [!! END OF FIX !!] ---


	// --- (Rest of your projectile code is identical) ---
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
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (!MoveComp || MoveComp->GravityScale != DefaultGravityScale) return;
    
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (AnimInstance && DashMontage)
    {
       AnimInstance->Montage_Play(DashMontage);
    }
    
    const float DashSpeed = DashDistance / DashDuration;
    const FVector DashVelocity = GetActorForwardVector() * DashSpeed;

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