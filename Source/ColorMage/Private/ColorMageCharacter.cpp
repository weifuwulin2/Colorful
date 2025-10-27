#include "ColorMageCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ColorMagePlayerState.h"
#include "ColorProjectile.h"
#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/KismetSystemLibrary.h" // 包含射线检测

AColorMageCharacter::AColorMageCharacter()
{
	PrimaryActorTick.bCanEverTick = true; 
	DefaultGravityScale = 1.0f;
}

void AColorMageCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) { DefaultGravityScale = MoveComp->GravityScale; }
	
	CameraSpringArm = FindComponentByClass<USpringArmComponent>();
	if (CameraSpringArm)
	{
		CameraSpringArm->TargetArmLength = DefaultCameraDist;
		CameraSpringArm->SocketOffset = DefaultCameraOffset;
		TargetArmLength = DefaultCameraDist;
		TargetSocketOffset = DefaultCameraOffset;
		CameraSpringArm->bEnableCameraLag = false;
		CameraSpringArm->bEnableCameraRotationLag = false;
	}
}

void AColorMageCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Lerp Rotation Logic
	if (bIsLerpingRotation)
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, HipFireRotationSpeed));
		if (GetActorRotation().Equals(TargetRotation, 1.0f))
		{
			bIsLerpingRotation = false;
			if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) { MoveComp->bOrientRotationToMovement = true; }
		}
	}

	// Smooth Zoom Logic
	if (CameraSpringArm)
	{
		CameraSpringArm->TargetArmLength = FMath::FInterpTo(CameraSpringArm->TargetArmLength, TargetArmLength, DeltaTime, ZoomInterpSpeed);
		CameraSpringArm->SocketOffset = FMath::VInterpTo(CameraSpringArm->SocketOffset, TargetSocketOffset, DeltaTime, ZoomInterpSpeed);
	}
}

void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction) { EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump); EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping); }
		if (DashAction) { EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnDash); }
		if (FireProjectileAction) { EnhancedInputComp->BindAction(FireProjectileAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnFireProjectile); }
		if (AimAction) { EnhancedInputComp->BindAction(AimAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnAimStarted); EnhancedInputComp->BindAction(AimAction, ETriggerEvent::Completed, this, &AColorMageCharacter::OnAimCompleted); }
	}
}

/** 当角色被控制器附身时调用 (重写 ACharacter 的函数) */
void AColorMageCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController); // 必须调用父类实现

	// 取消隐藏自身
	SetActorHiddenInGame(false);
	// 重新启用碰撞 (根据你的游戏设置选择合适的类型)
	SetActorEnableCollision(true); 
	// 重新启用 Tick
	SetActorTickEnabled(true);    

	UE_LOG(LogTemp, Log, TEXT("ColorMageCharacter %s已被 %s 附身并取消隐藏。"), *GetName(), *NewController->GetName());
	
	// 位置和旋转现在由控制器在 Possess() 之后通过 TeleportTo() 设置
}

// --- Helper Functions ---
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

// --- Input Handlers ---
void AColorMageCharacter::OnAimStarted()
{
	bIsManuallyAiming = true;
	bIsLerpingRotation = false; 
	SetAimRotation(true); 
	SetAimZoom(true);
}
void AColorMageCharacter::OnAimCompleted()
{
	bIsManuallyAiming = false;
	SetAimRotation(false);
	SetAimZoom(false);
}
void AColorMageCharacter::OnFireProjectile()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	if (!bIsManuallyAiming)
	{
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement()) { MoveComp->bOrientRotationToMovement = false; }
		TargetRotation = PC->GetControlRotation(); TargetRotation.Pitch = 0; TargetRotation.Roll = 0;
		bIsLerpingRotation = true;
	}
	FVector CameraLocation; FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector TraceStart = CameraLocation; FVector TraceEnd = TraceStart + (CameraRotation.Vector() * 10000.0f); 
	FVector TargetLocation = TraceEnd; 
	FHitResult HitResult; FCollisionQueryParams QueryParams; QueryParams.AddIgnoredActor(this); 
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		FVector DirectionToHit = (HitResult.Location - GetActorLocation()).GetSafeNormal();
		if (FVector::DotProduct(GetActorForwardVector(), DirectionToHit) > 0.0f) { TargetLocation = HitResult.Location; }
	}
	FVector SpawnLocation = GetMesh()->GetSocketLocation(ProjectileSpawnSocketName);
	FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
	AColorMagePlayerState* PS = PC->GetPlayerState<AColorMagePlayerState>(); if (!PS) return;
	EColor ColorToFire = PS->GetCurrentColor(); if (!ProjectileClass) return;
	FActorSpawnParameters SpawnParams; SpawnParams.Owner = this; SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AColorProjectile* Projectile = GetWorld()->SpawnActor<AColorProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile) { Projectile->SetProjectileColor(ColorToFire); }
}
void AColorMageCharacter::OnDash()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || MoveComp->GravityScale != DefaultGravityScale) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DashMontage) { AnimInstance->Montage_Play(DashMontage); }
	const float DashSpeed = DashDistance / DashDuration;
	const FVector DashVelocity = GetActorForwardVector();
	MoveComp->GravityScale = 0.0f; LaunchCharacter(DashVelocity, true, true);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashFinished);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_DashFinished, this, &AColorMageCharacter::OnDashFinished, DashDuration, false);
}
void AColorMageCharacter::OnDashFinished()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;
	MoveComp->GravityScale = DefaultGravityScale;
	MoveComp->StopMovementImmediately();
}