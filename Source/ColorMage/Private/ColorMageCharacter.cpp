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

void AColorMageCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	UE_LOG(LogTemp, Warning, TEXT("ColorMageCharacter %s 掉出世界!"), *GetName());

	// 获取 GameMode
	AGameModeBase* CurrentGameModeBase = UGameplayStatics::GetGameMode(this);
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
			UE_LOG(LogTemp, Error, TEXT("FellOutOfWorld: 无法获取 Controller 来重生!"));
			// 备用方案：直接销毁？
			// Destroy();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FellOutOfWorld: 无法获取 MyGameMode!"));
		// 备用方案：调用基类实现（通常是销毁 Actor）
		Super::FellOutOfWorld(dmgType);
	}
	
	// 注意：我们通常不调用 Super::FellOutOfWorld(dmgType);
	// 因为基类的默认实现是销毁 Actor，而我们想要重生。
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
    // --- [!! Debug Logs Added !!] ---
    UE_LOG(LogTemp, Warning, TEXT("OnDash() - Function Called!"));

    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (!MoveComp)
    {
        UE_LOG(LogTemp, Error, TEXT("OnDash() - FAILED: CharacterMovementComponent not found!"));
        return;
    }

    // Check if already dashing (using gravity scale)
    if (MoveComp->GravityScale != DefaultGravityScale)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnDash() - FAILED: Already dashing? GravityScale (%f) != Default (%f)"), MoveComp->GravityScale, DefaultGravityScale);
        return;
    }

    // Check Animation Montage (Optional check)
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if (!AnimInstance || !DashMontage)
    {
         UE_LOG(LogTemp, Warning, TEXT("OnDash() - WARNING: AnimInstance or DashMontage not found, but attempting dash anyway."));
         // If you require the montage to dash, uncomment the next line:
         // return;
    }
    else
    {
    	AnimInstance->Montage_Play(DashMontage);
        UE_LOG(LogTemp, Log, TEXT("OnDash() - Playing DashMontage."));
    }

    // Check Dash Distance and Duration values
    if (DashDistance <= 0.f || DashDuration <= 0.f)
    {
        UE_LOG(LogTemp, Error, TEXT("OnDash() - FAILED: Invalid DashDistance (%f) or DashDuration (%f)! Must be > 0."), DashDistance, DashDuration);
        return;
    }

    // --- Calculate Dash Velocity ---
    const float DashSpeed = DashDistance / DashDuration;
    const FVector ForwardDir = GetActorForwardVector(); // Get the direction the character mesh is facing
    const FVector DashVelocity = ForwardDir * DashSpeed;

    UE_LOG(LogTemp, Log, TEXT("OnDash() - Calculated Values: Speed=%.2f, ForwardDir=%s, DashVelocity=%s"), DashSpeed, *ForwardDir.ToString(), *DashVelocity.ToString());

    // --- Execute Dash ---
    // Temporarily set gravity to 0 for horizontal flight
    MoveComp->GravityScale = 0.0f;

    // Launch the character
    // bXYOverride = true: Force overrides current horizontal velocity.
    // bZOverride = true: Force overrides current vertical velocity (using DashVelocity.Z, which is 0 here).
    LaunchCharacter(DashVelocity, true, true);
    UE_LOG(LogTemp, Log, TEXT("OnDash() - LaunchCharacter called with Velocity=%s. GravityScale set to 0.0."), *DashVelocity.ToString());

    // --- Set Timer to Finish Dash ---
    // Clear any previous timer
    GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashFinished);

    // Set a new timer to call OnDashFinished after DashDuration
    GetWorld()->GetTimerManager().SetTimer(
       TimerHandle_DashFinished,          // Timer handle
       this,                              // Object to call function on
       &AColorMageCharacter::OnDashFinished, // Function to call
       DashDuration,                      // Delay
       false                              // Don't loop
    );
     UE_LOG(LogTemp, Log, TEXT("OnDash() - OnDashFinished timer set for %.2f seconds."), DashDuration);
     // --- [!! End of Debug Logs !!] ---
}
void AColorMageCharacter::OnDashFinished()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;
	MoveComp->GravityScale = DefaultGravityScale;
	MoveComp->StopMovementImmediately();
}