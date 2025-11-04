#include "ColorMageCharacter.h"

#include "ColorMageGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ColorMagePlayerState.h"
#include "ColorManagerSubsystem.h"
#include "ColorProjectile.h"
#include "ColorSourceActor.h"
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

void AColorMageCharacter::OnFireProjectile()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) { UE_LOG(LogTemp, Error, TEXT("OnFireProjectile: AnimInstance 为空!")); return; }
	if (!FireProjectileMontage) { UE_LOG(LogTemp, Error, TEXT("OnFireProjectile: FireProjectileMontage 未指定!")); return; }

	// [!! 关键 !!] 检查是否已在播放 *任何* 动作
	if (AnimInstance->Montage_IsPlaying(FireProjectileMontage) || AnimInstance->Montage_IsPlaying(AcquireColorMontage))
	{
		UE_LOG(LogTemp, Log, TEXT("OnFireProjectile: 动作已在播放，阻止刷屏。"));
		return;
	}

	// 1. 锁定移动
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_None); 
	}

	// 2. 立即将角色旋转锁定到摄像机方向
	SetAimRotation(true);

	// 3. 播放“挥笔”动画
	AnimInstance->Montage_Play(FireProjectileMontage);

	// 4. 设置“发射”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_SpawnProjectile);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_SpawnProjectile,
		this,
		&AColorMageCharacter::SpawnProjectile_Internal,
		ProjectileSpawnDelay,
		false
	);

	// 5. 设置“重置旋转和移动”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetFireRotation);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ResetFireRotation,
		this,
		&AColorMageCharacter::ResetActionState, // [!!] 调用共用的重置函数
		HipFireRotationDuration, 
		false
	);
}
void AColorMageCharacter::SpawnProjectile_Internal()
{
	UE_LOG(LogTemp, Warning, TEXT("SpawnProjectile_Internal: 函数被调用!"));

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnProjectile_Internal: 失败! 找不到 PlayerController!"));
		return;
	}

	// 检查 ProjectileClass 是否已设置
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnProjectile_Internal: 失败! ProjectileClass 未在蓝图中指定!"));
		return;
	}
	// --- [!! 调试结束 !!] ---

	// (使用最终的“射线检测”方案来确保命中准星)
	FVector CameraLocation; FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector TraceStart = CameraLocation; FVector TraceEnd = TraceStart + (CameraRotation.Vector() * 10000.0f); 
	FVector TargetLocation = TraceEnd; 
	FHitResult HitResult; FCollisionQueryParams QueryParams; QueryParams.AddIgnoredActor(this); 
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		FVector DirectionToHit = (HitResult.Location - GetActorLocation()).GetSafeNormal();
		if (FVector::DotProduct(GetActorForwardVector(), DirectionToHit) > -0.2f)
		{
			TargetLocation = HitResult.Location;
		}
	}
	FVector SpawnLocation = GetMesh()->GetSocketLocation(ProjectileSpawnSocketName);
	FRotator SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
	
	// (投射物生成逻辑)
	AColorMagePlayerState* PS = PC->GetPlayerState<AColorMagePlayerState>(); if (!PS) return;
	EColor ColorToFire = PS->GetCurrentColor();
	FActorSpawnParameters SpawnParams; SpawnParams.Owner = this; SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AColorProjectile* Projectile = GetWorld()->SpawnActor<AColorProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	
	if (Projectile) 
	{ 
		Projectile->SetProjectileColor(ColorToFire); 
		UE_LOG(LogTemp, Warning, TEXT("SpawnProjectile_Internal: 成功发射投射物!"));
	}
}
void AColorMageCharacter::OnDash() 
{
   UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!MoveComp || !AnimInstance) return;
    // --- 打断逻辑 ---
    bool bIsFiring = FireProjectileMontage && AnimInstance->Montage_IsPlaying(FireProjectileMontage);
    bool bIsAcquiring = AcquireColorMontage && AnimInstance->Montage_IsPlaying(AcquireColorMontage);
    if (bIsFiring || bIsAcquiring)
    {
        UE_LOG(LogTemp, Warning, TEXT("Dash 打断了 %s 动作!"), bIsFiring ? TEXT("射击") : TEXT("汲取"));
        if (bIsFiring) AnimInstance->Montage_Stop(0.1f, FireProjectileMontage);
        if (bIsAcquiring) AnimInstance->Montage_Stop(0.1f, AcquireColorMontage);
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_SpawnProjectile);
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetFireRotation);
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AcquireColor);
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetAcquireRotation);
        ResetActionState();
    }
    // --- 重力检查 ---
    if (MoveComp->GravityScale != DefaultGravityScale && !bIsFiring && !bIsAcquiring)
    {
        UE_LOG(LogTemp, Log, TEXT("OnDash: 已经在冲刺中，阻止连按。"));
        return;
    }
    
    // --- 1. 播放冲刺动画（使用动画自己的时长）---
    if (AnimInstance && DashMontage) 
    { 
        AnimInstance->Montage_Play(DashMontage, 1.0f); // 动画以原速播放
        UE_LOG(LogTemp, Warning, TEXT("播放冲刺动画，动画自管理时长"));
    }
    
    // --- 2. 执行冲刺移动（使用 DashDuration）---
    const float DashSpeed = DashDistance / DashDuration;
    const FVector DashVelocity = GetActorForwardVector() * DashSpeed;
    
    UE_LOG(LogTemp, Warning, TEXT("冲刺移动 - 距离:%f, 时间:%f, 速度:%f"), 
           DashDistance, DashDuration, DashSpeed);
    
    MoveComp->GravityScale = 0.0f; 
    LaunchCharacter(DashVelocity, true, true);
    
    // --- 3. 设置移动结束计时器（只管移动，不管动画）---
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashFinished);
    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle_DashFinished, 
        this, 
        &AColorMageCharacter::OnDashFinished, 
        DashDuration,  // 使用移动时间
        false
    );
}

void AColorMageCharacter::OnDashFinished() 
{
	UE_LOG(LogTemp, Warning, TEXT("冲刺移动结束，恢复重力和停止移动"));
    
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;
    
	// 只恢复移动相关的设置，不管动画
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

void AColorMageCharacter::RequestAcquireColor()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) { UE_LOG(LogTemp, Error, TEXT("RequestAcquireColor: AnimInstance 为空!")); return; }
	if (!AcquireColorMontage) { UE_LOG(LogTemp, Error, TEXT("RequestAcquireColor: AcquireColorMontage 未指定!")); return; }

	// [!! 关键 !!] 检查是否已在播放 *任何* 动作
	if (AnimInstance->Montage_IsPlaying(AcquireColorMontage) || AnimInstance->Montage_IsPlaying(FireProjectileMontage))
	{
		UE_LOG(LogTemp, Log, TEXT("RequestAcquireColor: 动作已在播放，阻止刷屏。"));
		return;
	}

	// --- [!! 新增逻辑：锁定 !!] ---
	// 1. 锁定移动
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_None); 
	}
	// 2. 锁定旋转
	SetAimRotation(true);
	// --- [!! 逻辑结束 !!] ---

	// 3. 播放“汲取”动画
	AnimInstance->Montage_Play(AcquireColorMontage);

	// 4. 设置“汲取”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AcquireColor);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AcquireColor,
		this,
		&AColorMageCharacter::AcquireColor_Internal,
		AcquireColorDelay,
		false
	);

	// 5. [!! 新增 !!] 设置“重置旋转和移动”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetAcquireRotation);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ResetAcquireRotation,
		this,
		&AColorMageCharacter::ResetActionState, // [!!] 调用共用的重置函数
		AcquireRotationDuration, // 使用汲取专用的时长
		false
	);
}

void AColorMageCharacter::AcquireColor_Internal()
{
	UE_LOG(LogTemp, Warning, TEXT("=== AcquireColor_Internal 开始执行 (MultiTrace) ==="));

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) 
	{
		UE_LOG(LogTemp, Error, TEXT("PlayerController 为空！"));
		return;
	}

	// 1. 获取摄像机视角
	FVector CamLoc; 
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	// 2. 设置射线
	FVector TraceStart = CamLoc;
	float AcquireDistance = 15500.0f;
	FVector TraceEnd = TraceStart + (CamRot.Vector() * AcquireDistance);

	// 3. Multi Trace 检测
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	bool bHit = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		TraceStart,
		TraceEnd,
		ECollisionChannel::ECC_Visibility,
		QueryParams
	);

	/*// 调试可视化
	DrawDebugLine(
		GetWorld(), 
		TraceStart, 
		TraceEnd, 
		bHit ? FColor::Green : FColor::Red, 
		false, 
		3.0f,
		0, 
		3.0f
	);*/

	UE_LOG(LogTemp, Warning, TEXT("Multi射线检测结果: %s, 命中数量: %d"), 
		   bHit ? TEXT("有命中") : TEXT("无命中"), HitResults.Num());

	if (bHit && HitResults.Num() > 0)
	{
		// 4. 遍历所有命中结果，寻找 ColorSourceActor
		AColorSourceActor* ClosestColorSource = nullptr;
		float ClosestDistance = FLT_MAX;

		for (const FHitResult& Hit : HitResults)
		{
			if (Hit.GetActor())
			{
				UE_LOG(LogTemp, Log, TEXT("命中物体: %s, 距离: %f"), 
					   *Hit.GetActor()->GetName(), Hit.Distance);

				// 检查是否是 ColorSourceActor
				AColorSourceActor* ColorSource = Cast<AColorSourceActor>(Hit.GetActor());
				if (ColorSource)
				{
					UE_LOG(LogTemp, Warning, TEXT("找到 ColorSourceActor: %s"), *ColorSource->GetName());
					
					// 找最近的那个
					if (Hit.Distance < ClosestDistance)
					{
						ClosestDistance = Hit.Distance;
						ClosestColorSource = ColorSource;
					}
				}
			}
		}

		// 5. 处理最近的 ColorSource
		if (ClosestColorSource)
		{
			UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
			if (ColorManager)
			{
				UE_LOG(LogTemp, Warning, TEXT("处理最近的ColorSource: %s (距离: %f)"), 
					   *ClosestColorSource->GetName(), ClosestDistance);
				ColorManager->HandleAcquireColor(PC, ClosestColorSource);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("ColorManagerSubsystem 为空！"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("未找到任何 ColorSourceActor"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Multi射线未命中任何物体"));
	}
}

void AColorMageCharacter::SetAimRotation(bool bLockRotationToCamera)
{
	UE_LOG(LogTemp, Warning, TEXT("SetAimRotation: bLockRotationToCamera = %s"), 
		   bLockRotationToCamera ? TEXT("TRUE") : TEXT("FALSE"));

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	if (bLockRotationToCamera)
	{
		// 1. 立即获取控制器旋转并应用到角色
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			FRotator ControlRotation = PC->GetControlRotation();
			// 只使用 Yaw，保持角色直立
			FRotator NewRotation = FRotator(0.0f, ControlRotation.Yaw, 0.0f);
			SetActorRotation(NewRotation);
            
			UE_LOG(LogTemp, Warning, TEXT("立即设置角色旋转为: %s"), *NewRotation.ToString());
		}

		// 2. 设置移动组件跟随控制器旋转
		MoveComp->bUseControllerDesiredRotation = true;
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // 快速旋转
	}
	else
	{
		// 恢复正常模式
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // 正常旋转速度
	}
}

/**
 * 计时器调用的函数，用于在“腰射”后恢复正常的探索旋转模式
 */
void AColorMageCharacter::ResetActionState()
{
	UE_LOG(LogTemp, Warning, TEXT("ResetActionState: 计时器触发，重置旋转和移动。"));
	
	// 1. 恢复旋转为“探索”模式
	SetAimRotation(false);

	// 2. 恢复移动
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetDefaultMovementMode(); 
	}
}