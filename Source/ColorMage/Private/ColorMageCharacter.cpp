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
#include "NiagaraSystem.h"           
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"

AColorMageCharacter::AColorMageCharacter()
{
	// [!! GDD 修正 !!] 角色不再需要每帧 Tick
	PrimaryActorTick.bCanEverTick = false; 
	DefaultGravityScale = 1.0f;
	ControlType = EPawnControlType::Character; // 确保设置了类型

	HairDMI = nullptr;
	BrushTipDMI = nullptr;

	// --- [!! GDD 修改：新增 !!] ---
	// 1. 创建毛笔的静态网格体组件
	BrushMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BrushMeshComponent"));
	if (BrushMeshComponent)
	{
		// 2. 将它附加到角色的骨骼网格体 (GetMesh())
		//BrushMeshComponent->SetupAttachment(GetMesh(), BrushAttachmentSocketName);
		// 3. (重要!) 关闭毛笔自身的碰撞，防止它挡住玩家或射线
		BrushMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
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
	
	USkeletalMeshComponent* MyMesh = GetMesh();
	if (MyMesh)
	{
		// 1. 创建头发的 DMI (来自骨骼网格体)
		HairDMI = MyMesh->CreateDynamicMaterialInstance(0);
		if (!HairDMI)
		{
			UE_LOG(LogTemp, Error, TEXT("%s: 无法在插槽 %d 上创建头发 DMI!"), *GetName(), HairMaterialSlotIndex);
		}
	}
	else { /* Log Error */ }
	UE_LOG(LogTemp, Error, TEXT("=== CreatureCharacter碰撞调试: %s ==="), *GetName());
    
	if (USkeletalMeshComponent* SkeletalMeshComponent = GetMesh())
	{
		UE_LOG(LogTemp, Error, TEXT("Mesh碰撞启用: %s"), SkeletalMeshComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision ? TEXT("是") : TEXT("否"));
		UE_LOG(LogTemp, Error, TEXT("Mesh碰撞配置: %s"), *SkeletalMeshComponent->GetCollisionProfileName().ToString());
		UE_LOG(LogTemp, Error, TEXT("Mesh可见性碰撞: %s"), SkeletalMeshComponent->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block ? TEXT("阻挡") : TEXT("不阻挡"));
	}
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		UE_LOG(LogTemp, Error, TEXT("胶囊碰撞启用: %s"), Capsule->GetCollisionEnabled() != ECollisionEnabled::NoCollision ? TEXT("是") : TEXT("否"));
		UE_LOG(LogTemp, Error, TEXT("胶囊碰撞配置: %s"), *Capsule->GetCollisionProfileName().ToString());
		UE_LOG(LogTemp, Error, TEXT("胶囊可见性碰撞: %s"), Capsule->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block ? TEXT("阻挡") : TEXT("不阻挡"));
	}
	// 2. [!! 修复 !!] 为毛笔网格体创建 DMI
	if (BrushMeshComponent)
	{
		// 假设毛笔尖的材质在它自己的第 0 个插槽
		BrushTipDMI = BrushMeshComponent->CreateDynamicMaterialInstance(1); 
		if (!BrushTipDMI)
		{
			UE_LOG(LogTemp, Error, TEXT("%s: 无法在 BrushMeshComponent 上创建毛笔尖 DMI! (插槽 0)"), *GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: 找不到 BrushMeshComponent!"), *GetName());
	}
	if (BrushMeshComponent)
	{
		// [!! 关键 !!] 使用 FAttachmentTransformRules::SnapToTarget 确保它正确对齐
		BrushMeshComponent->AttachToComponent(
			GetMesh(), 
			FAttachmentTransformRules::SnapToTargetIncludingScale, 
			BrushAttachmentSocketName // 现在这个变量有正确的值 (例如 "Hand_R_Socket")
		);
		UE_LOG(LogTemp, Warning, TEXT("毛笔已附加到插槽: %s"), *BrushAttachmentSocketName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s: 找不到 BrushMeshComponent!"), *GetName());
	}

	CurrentHealth = MaxHealth;
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

	AColorMagePlayerState* PS = GetPlayerState<AColorMagePlayerState>();
	if (PS)
	{
		// 2. 绑定委托：当 PlayerState->OnPlayerColorChanged 广播时，调用 this->OnPlayerColorChanged
		PS->OnPlayerColorChanged.AddDynamic(this, &AColorMageCharacter::OnPlayerColorChanged);

		// 3. 立即调用一次，以同步游戏开始时的初始颜色
		OnPlayerColorChanged(PS->GetCurrentColor());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s 在 PossessedBy 时无法获取 AColorMagePlayerState!"), *GetName());
	}
}

// [!! 已移除 !!] SetAimRotation, SetAimZoom, OnAimStarted, OnAimCompleted, ResetHipFireRotation

	// --- [!! 射击逻辑 - 已修改 !!] ---
void AColorMageCharacter::OnFireProjectile()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (!AnimInstance) { /*...*/ return; }
	if (!FireProjectileMontage) { /*...*/ return; }
	if (!MoveComp) { /*...*/ return; } // [!! 新增 !!] 确保我们有移动组件

	// 检查刷屏
	if (AnimInstance->Montage_IsPlaying(FireProjectileMontage) || AnimInstance->Montage_IsPlaying(AcquireColorMontage))
	{
		UE_LOG(LogTemp, Log, TEXT("OnFireProjectile: 动作已在播放，阻止刷屏。"));
		return;
	}

	/*// --- [!! 关键修复 !!] ---
	// 检查角色是否在地面上
	if (MoveComp->IsMovingOnGround())
	{
		// 1. (在地面上) 锁定移动
		UE_LOG(LogTemp, Log, TEXT("OnFireProjectile: 在地面上射击，锁定移动。"));
		MoveComp->SetMovementMode(MOVE_None); 
	}
	else
	{
		// 2. (在空中) 不锁定移动 (重力会继续生效)
		//    但我们可以暂时禁用 WASD 输入，防止空中“滑冰” (可选)
		//    (或者保持空中控制 AirControl)
		//    现在，我们暂时什么也不做，只让动画播放
		UE_LOG(LogTemp, Log, TEXT("OnFireProjectile: 在空中射击，保持重力。"));
	}
	// --- [!! 修复结束 !!] ---*/

	// 3. 锁定旋转 (这在空中和地面都应该执行)
	SetAimRotation(true);

	// 4. 播放“挥笔”动画
	AnimInstance->Montage_Play(FireProjectileMontage);
	UE_LOG(LogTemp, Log, TEXT("OnFireProjectile: 播放蒙太奇 %s"), *FireProjectileMontage->GetName());

	// 5. 设置“发射”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_SpawnProjectile);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_SpawnProjectile,
		this,
		&AColorMageCharacter::SpawnProjectile_Internal,
		ProjectileSpawnDelay,
		false
	);

	// 6. 设置“重置”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetFireRotation);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ResetFireRotation,
		this,
		&AColorMageCharacter::ResetActionState,
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
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AnimInstance) { /*...*/ return; }
	if (!AcquireColorMontage) { /*...*/ return; }
	if (!MoveComp) { /*...*/ return; } // [!! 新增 !!]

	// 检查刷屏
	if (AnimInstance->Montage_IsPlaying(AcquireColorMontage) || AnimInstance->Montage_IsPlaying(FireProjectileMontage))
	{
		UE_LOG(LogTemp, Log, TEXT("RequestAcquireColor: 动作已在播放，阻止刷屏。"));
		return;
	}

	/*// --- [!! 关键修复 !!] ---
	// 检查角色是否在地面上
	if (MoveComp->IsMovingOnGround())
	{
		// (在地面上) 锁定移动
		UE_LOG(LogTemp, Log, TEXT("RequestAcquireColor: 在地面上汲取，锁定移动。"));
		MoveComp->SetMovementMode(MOVE_None); 
	}
	else
	{
		// (在空中) 不锁定移动
		UE_LOG(LogTemp, Log, TEXT("RequestAcquireColor: 在空中汲取，保持重力。"));
	}*/
	// --- [!! 修复结束 !!] ---

	// 锁定旋转
	SetAimRotation(true);

	// 播放“汲取”动画
	AnimInstance->Montage_Play(AcquireColorMontage);

	// 设置“汲取”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AcquireColor);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_AcquireColor,
		this,
		&AColorMageCharacter::AcquireColor_Internal,
		AcquireColorDelay,
		false
	);

	// 设置“重置”计时器
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_ResetAcquireRotation);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_ResetAcquireRotation,
		this,
		&AColorMageCharacter::ResetActionState,
		AcquireRotationDuration,
		false
	);
}

void AColorMageCharacter::PlayUnpossessEffect()
{
	if (UnpossessVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), UnpossessVFX, GetActorLocation(), GetActorRotation());
	}
}

void AColorMageCharacter::PlayPossessEffect()
{
	if (PossessVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PossessVFX, GetActorLocation(), GetActorRotation());
	}
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
	FVector TraceEnd = TraceStart + (CamRot.Vector() * AcquireDistance);

	// 3. Single Trace 检测
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
	   HitResult, TraceStart, TraceEnd,
	   ECollisionChannel::ECC_Visibility, QueryParams
	);
	
	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor) return;

		// 4. 检查是否是 ColorSourceActor
		AColorSourceActor* ColorSource = Cast<AColorSourceActor>(HitActor);
		if (ColorSource)
		{
			// 5. 成功，处理 ColorSource
			UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
			if (ColorManager)
			{
				UE_LOG(LogTemp, Warning, TEXT("处理 ColorSource: %s"), *ColorSource->GetName());
				
				// [!! 1. 成功提取 !!]
				ColorManager->HandleAcquireColor(PC, ColorSource);

				// --- [!! 2. 播放 VFX !!] ---
				if (AcquireColorVFX)
				{
					// 在 *颜色源* 的位置播放特效
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						GetWorld(),
						AcquireColorVFX,
						this->GetActorLocation(),
						this->GetActorRotation()
					);
				}
			}
			else { /* Log Error */ }
		}
		else { /* Log (Hit wrong actor) */ }
	}
	else { /* Log (Trace missed) */ }
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

void AColorMageCharacter::OnPlayerColorChanged(EColor NewColor)
{
	UE_LOG(LogTemp, Warning, TEXT("%s: OnPlayerColorChanged 触发! 新颜色: %d"), *GetName(), (int32)NewColor);

	// 1. 在我们的 TMap 中查找对应的 FLinearColor
	FLinearColor* ColorToApply = ColorMapping.Find(NewColor);

	// 2. 如果没找到 (比如 TMap 中没有 EC_None 的条目)，设置一个备用颜色
	if (!ColorToApply)
	{
		// (你之前的备用逻辑是完美的)
		if (FLinearColor* DefaultColor = ColorMapping.Find(EColor::EC_None))
		{ ColorToApply = DefaultColor; }
		else
		{
			FLinearColor FallbackGray = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
			ColorToApply = &FallbackGray;
		}
	}

	// 3. 应用到头发的 DMI
	if (HairDMI)
	{
		HairDMI->SetVectorParameterValue(HairColorParameterName, *ColorToApply);
	}
	else { UE_LOG(LogTemp, Error, TEXT("HairDMI 为空!")); }

	// 4. [!! 修复 !!] 应用到毛笔尖的 DMI
	if (BrushTipDMI)
	{
		BrushTipDMI->SetVectorParameterValue(BrushTipColorParameterName, *ColorToApply);
	}
	else { UE_LOG(LogTemp, Error, TEXT("BrushTipDMI 为空!")); }
}

void AColorMageCharacter::TakeDamage(int32 DamageAmount)
{
	if (DamageAmount <= 0) return;
    
	CurrentHealth = FMath::Max(0, CurrentHealth - DamageAmount);
    
	UE_LOG(LogTemp, Warning, TEXT("玩家受到 %d 点伤害，当前血量：%d/%d"), DamageAmount, CurrentHealth, MaxHealth);
    
	// 广播血量变化
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    
	// 停止当前的回血计时器
	GetWorld()->GetTimerManager().ClearTimer(HealthRegenTimerHandle);
    
	// 检查是否死亡
	if (CurrentHealth <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("玩家死亡！"));
		// 这里可以添加死亡处理逻辑
		return;
	}
    
	// 开始回血倒计时
	StartHealthRegeneration();
}
void AColorMageCharacter::StartHealthRegeneration()
{
	if (CurrentHealth >= MaxHealth) return;
    
	GetWorld()->GetTimerManager().ClearTimer(HealthRegenTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		HealthRegenTimerHandle,
		this,
		&AColorMageCharacter::RegenerateHealth,
		HealthRegenDelay,
		false
	);
}
void AColorMageCharacter::RegenerateHealth()
{
	if (CurrentHealth >= MaxHealth) return;
    
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + 1);
    
	UE_LOG(LogTemp, Log, TEXT("玩家回血：%d/%d"), CurrentHealth, MaxHealth);
    
	// 广播血量变化
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}