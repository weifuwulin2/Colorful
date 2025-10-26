// ColorMageCharacter.cpp
#include "ColorMageCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/PlayerController.h" // Needed to get PlayerState
#include "ColorMagePlayerState.h"        // Needed to get color
#include "ColorProjectile.h"              // Needed to spawn projectile

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
}

// Binds inputs specific to this Character body
void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind Jump
		if (JumpAction)
		{
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		// Bind Dash
		if (DashAction)
		{
			EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnDash);
		}
		
		// --- NEW: Bind Fire Projectile (LMB) ---
		if (FireProjectileAction)
		{
			EnhancedInputComp->BindAction(FireProjectileAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnFireProjectile);
		}
	}
}

// --- NEW: Fire Projectile Logic ---
void AColorMageCharacter::OnFireProjectile()
{
	// 1. Get Player Controller and Player State
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	AColorMagePlayerState* PS = PC->GetPlayerState<AColorMagePlayerState>();
	if (!PS) return;

	// 2. Get the color to fire
	EColor ColorToFire = PS->GetCurrentColor();

	// 3. Check if we have a color (can't fire "Gray")
	if (ColorToFire == EColor::EC_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fire Failed: No color equipped."));
		// (Play "no ammo" sound)
		return;
	}

	// 4. Check if we have a valid Projectile Class assigned
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Fire Failed: ProjectileClass is not set in BP_ColorMageCharacter."));
		return;
	}

	// 5. Get spawn location and rotation
	FVector SpawnLocation = GetMesh()->GetSocketLocation(ProjectileSpawnSocketName);
	// Fire in the direction the camera is facing
	FRotator SpawnRotation = PC->GetControlRotation(); 

	// 6. Set spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 7. Spawn the projectile
	AColorProjectile* Projectile = GetWorld()->SpawnActor<AColorProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (Projectile)
	{
		// 8. Tell the projectile what color it is
		Projectile->SetProjectileColor(ColorToFire);
		// (Play "Fire" animation montage)
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