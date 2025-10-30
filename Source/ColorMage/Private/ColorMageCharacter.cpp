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
	// [!! GDD 修正 !!] 角色不再需要每帧 Tick
	PrimaryActorTick.bCanEverTick = false; 
	DefaultGravityScale = 1.0f;
	ControlType = EPawnControlType::Character; // 确保设置了类型
}

void AColorMageCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultGravityScale = MoveComp->GravityScale;
	}
	
	// --- [!! GDD 修正：简化 BeginPlay !!] ---
	// 只设置一次默认值，不再需要插值
	CameraSpringArm = FindComponentByClass<USpringArmComponent>();
	if (CameraSpringArm)
	{
		CameraSpringArm->TargetArmLength = DefaultCameraDist;
		CameraSpringArm->SocketOffset = DefaultCameraOffset;
		// 确保 SpringArm Lag 被禁用，以实现“跟随迅速”
		CameraSpringArm->bEnableCameraLag = false;
		CameraSpringArm->bEnableCameraRotationLag = false;
	}
	// --- [!! GDD 修正结束 !!] ---
}

// [!! 已移除 !!] Tick 函数已被移除

void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// --- [!! GDD 修正 !!] ---
		if (JumpAction) { EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump); EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping); }
		if (DashAction) { EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnDash); }
		if (FireProjectileAction) { EnhancedInputComp->BindAction(FireProjectileAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnFireProjectile); }
		// [!! 已移除 !!] AimAction 绑定
		// --- [!! GDD 修正结束 !!] ---
	}
}

void AColorMageCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true); 
	SetActorTickEnabled(false); // 角色不需要 Tick
	UE_LOG(LogTemp, Log, TEXT("ColorMageCharacter %s 已被重新附身并取消隐藏。"), *GetName());
}

// [!! 已移除 !!] SetAimRotation, SetAimZoom, OnAimStarted, OnAimCompleted, ResetHipFireRotation

// --- [!! GDD 修正：简化 !!] ---
void AColorMageCharacter::OnFireProjectile()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	// [!! 已移除 !!] 所有 Hip-fire 旋转逻辑均已移除
	// GDD 规定：射击时使用腰射，角色朝向移动方向

	// --- (使用最终的“射线检测”方案来确保命中准星) ---
	FVector CameraLocation; FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector TraceStart = CameraLocation; FVector TraceEnd = TraceStart + (CameraRotation.Vector() * 10000.0f); 
	FVector TargetLocation = TraceEnd; 
	FHitResult HitResult; FCollisionQueryParams QueryParams; QueryParams.AddIgnoredActor(this); 
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		FVector DirectionToHit = (HitResult.Location - GetActorLocation()).GetSafeNormal();
		if (FVector::DotProduct(GetActorForwardVector(), DirectionToHit) > -0.2f) // 防止向后射击
		{
			TargetLocation = HitResult.Location;
		}
	}
	FVector SpawnLocation = GetMesh()->GetSocketLocation(ProjectileSpawnSocketName);
	FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
	
	// --- (投射物生成逻辑) ---
	AColorMagePlayerState* PS = PC->GetPlayerState<AColorMagePlayerState>(); if (!PS) return;
	EColor ColorToFire = PS->GetCurrentColor(); if (!ProjectileClass) return;
	FActorSpawnParameters SpawnParams; SpawnParams.Owner = this; SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AColorProjectile* Projectile = GetWorld()->SpawnActor<AColorProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile) { Projectile->SetProjectileColor(ColorToFire); }
}

// --- (Dash 和 DashFinished 保持不变) ---
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