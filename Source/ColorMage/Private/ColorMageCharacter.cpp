#include "ColorMageCharacter.h"

#include "ColorMageGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ColorMagePlayerState.h"
#include "ColorProjectile.h"
#include "EnhancedInputComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
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
	if (AnimInstance && DashMontage) 
	{ 
		AnimInstance->Montage_Play(DashMontage); 
	}
    
	// [!! 修复 1 !!] 计算正确的冲刺速度
	const float DashSpeed = DashDistance / DashDuration;
    
	// [!! 修复 2 !!] 使用正确的冲刺向量和速度
	const FVector DashVelocity = GetActorForwardVector() * DashSpeed; // 添加速度倍数
    
	MoveComp->GravityScale = 0.0f; 
    
	// [!! 修复 3 !!] 使用正确的 LaunchCharacter 参数
	LaunchCharacter(DashVelocity, true, true); // XY 和 Z 都覆盖现有速度
    
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

void AColorMageCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	UE_LOG(LogTemp, Warning, TEXT("ColorMageCharacter %s 掉出世界!"), *GetName());

	// 获取 GameMode
	AGameModeBase* CurrentGameModeBase = UGameplayStatics::GetGameMode(this);
	// 确保使用你正确的 GameMode 类名
	AColorMageGameMode* MyGameMode = Cast<AColorMageGameMode>(CurrentGameModeBase); 

	if (MyGameMode)
	{
		// 获取控制这个角色的 Controller
		AController* MyController = GetController();
		if (MyController)
		{
			// 调用 GameMode 的重生函数
			MyGameMode->RespawnPlayer(MyController);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FellOutOfWorld (Character): 无法获取 Controller 来重生!"));
			// 备用方案：调用基类实现（销毁 Actor）
			Super::FellOutOfWorld(dmgType);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FellOutOfWorld (Character): 无法获取 AColorMageGameMode!"));
		// 备用方案：调用基类实现（销毁 Actor）
		Super::FellOutOfWorld(dmgType);
	}

	// 注意：我们不调用 Super::FellOutOfWorld(dmgType);
	// 因为基类的默认实现是销毁 Actor，而我们想要重生。
}