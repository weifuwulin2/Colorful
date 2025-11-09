#include "ColorMageController.h"

#include "ColorableActor.h"
#include "ColorManagerSubsystem.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ColorMageCharacter.h"
#include "ColorMagePlayerState.h"
#include "PossessablePawn.h"
#include "ColorSourceActor.h" 
#include "CreatureCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AColorMageController::AColorMageController()
{
	bShowMouseCursor = false;
	PrimaryActorTick.bCanEverTick = true;
}
void AColorMageController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. 检查是否在控制角色
    AColorMageCharacter* MyCharacter = Cast<AColorMageCharacter>(GetPawn());
    if (!MyCharacter)
    {
        // 清理状态
        if (CurrentHighlightedActor.IsValid() && CurrentHighlightedActor->Implements<UHighlightableInterface>())
        {
            IHighlightableInterface::Execute_OnUnhighlight(CurrentHighlightedActor.Get());
        }
        CurrentHighlightedActor = nullptr;
        
        if (CurrentTargetType != EReticleTargetType::None)
        {
            CurrentTargetType = EReticleTargetType::None;
            OnReticleTargetChanged.Broadcast(CurrentTargetType);
        }
        return;
    }

    // 2. 设置射线
    FVector CameraLocation; FRotator CameraRotation;
    GetPlayerViewPoint(CameraLocation, CameraRotation);
    FVector TraceStart = CameraLocation;
    FVector TraceEnd = TraceStart + (CameraRotation.Vector() * ReticleTraceDistance);
    
    EReticleTargetType NewTargetType = EReticleTargetType::None;
    AActor* NewHitActor = nullptr;

    // 3. 执行射线检测 - 恢复原始的单次检测
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetPawn());

    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Pawn, QueryParams))
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor)
        {
            // [!! 按优先级检查各种类型 !!]
            
            // 优先级1: AColorSourceActor (用于提取颜色)
            if (HitActor->IsA<AColorSourceActor>())
            {
                NewTargetType = EReticleTargetType::Extractable;
                if (HitActor->Implements<UHighlightableInterface>())
                {
                    NewHitActor = HitActor;
                }
            }
            // 优先级2: APossessablePawn (可附身平台)
            else if (APossessablePawn* PossPawn = Cast<APossessablePawn>(HitActor))
            {
                AColorMagePlayerState* PS = GetPlayerState<AColorMagePlayerState>();
                if (PS && PS->GetCurrentColor() != EColor::EC_None && PS->GetCurrentColor() == PossPawn->GetColor())
                {
                    NewTargetType = EReticleTargetType::Possessable;
                }
                if (PossPawn->Implements<UHighlightableInterface>())
                {
                    NewHitActor = PossPawn;
                }
            }
            else if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(HitActor))
            {
            	AColorMagePlayerState* PS = GetPlayerState<AColorMagePlayerState>();
            	if (PS && PS->GetCurrentColor() != EColor::EC_None && 
					PS->GetCurrentColor() == Creature->GetColor() && 
					Creature->CanBePossessed())
            	{
            		NewTargetType = EReticleTargetType::Possessable;
            	}
            	if (Creature->Implements<UHighlightableInterface>())
            	{
            		NewHitActor = Creature;
            	}
            }
            // 优先级3: AColorableActor (可染色物体)
            else if (AColorableActor* ColorActor = Cast<AColorableActor>(HitActor))
            {
                NewTargetType = EReticleTargetType::Paintable;
                if (ColorActor->Implements<UHighlightableInterface>())
                {
                    NewHitActor = ColorActor;
                }
            }
            // 其他可高亮的物体
            else if (HitActor->Implements<UHighlightableInterface>())
            {
                NewHitActor = HitActor;
            }
        }
    }

   
    // 5. 更新准星UI
    if (NewTargetType != CurrentTargetType)
    {
        CurrentTargetType = NewTargetType;
        OnReticleTargetChanged.Broadcast(CurrentTargetType);
    }

    // 6. 更新高亮
    if (NewHitActor != CurrentHighlightedActor.Get())
    {
        if (CurrentHighlightedActor.IsValid() && CurrentHighlightedActor->Implements<UHighlightableInterface>())
        {
            IHighlightableInterface::Execute_OnUnhighlight(CurrentHighlightedActor.Get());
        }

        if (NewHitActor)
        {
            IHighlightableInterface::Execute_OnHighlight(NewHitActor);
        }

        CurrentHighlightedActor = NewHitActor;
    }
}


void AColorMageController::OnPossess(APawn* InPawn)
{
	
	Super::OnPossess(InPawn);
    
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		if (DefaultInputMappingContext) { Subsystem->AddMappingContext(DefaultInputMappingContext, 0); }
	}
	if (PlayerCameraManager) { PlayerCameraManager->ViewPitchMin = -70.0f; PlayerCameraManager->ViewPitchMax = 80.0f; }
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true); 
	SetInputMode(InputMode);
    
	EPawnControlType PossessedType = EPawnControlType::Unknown;
	if (InPawn)
	{
		if (AColorMageCharacter* ColorMageCharacter = Cast<AColorMageCharacter>(InPawn)) 
		{ 
			PossessedType = ColorMageCharacter->GetControlType(); 
			// [!! 重置为法师角色的默认摄像机设置 !!]
			SetCameraForCharacterType(ColorMageCharacter);
		}
		else if (APossessablePawn* PossPawn = Cast<APossessablePawn>(InPawn)) 
		{ 
			PossessedType = PossPawn->GetControlType(); 
			// 平台使用默认设置
		}
		// [!! 新增：支持CreatureCharacter并设置摄像机 !!]
		else if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(InPawn))
		{
			PossessedType = Creature->GetControlType();
			// [!! 为大型生物设置摄像机 !!]
			SetCameraForCreature(Creature);
		}
	}
	OnPawnControlChanged.Broadcast(PossessedType);
	CurrentTargetType = EReticleTargetType::None;
	OnReticleTargetChanged.Broadcast(CurrentTargetType);
}

void AColorMageController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInputComp = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction) { EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleMove); }
		if (LookAction) { EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleLook); }

		// --- [!! GDD 修正：绑定 !!] ---
		if (AcquireAction) // RMB
		{
			EnhancedInputComp->BindAction(AcquireAction, ETriggerEvent::Started, this, &AColorMageController::OnAcquire);
		}
		if (PossessAction) // F
		{
			EnhancedInputComp->BindAction(PossessAction, ETriggerEvent::Started, this, &AColorMageController::OnPossessInteract);
		}
		// --- [!! GDD 修正结束 !!] ---
	}
}

void AColorMageController::HandleMove(const FInputActionValue& Value)
{
    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;
    const FVector2D MoveVector = Value.Get<FVector2D>();
    const FRotator Rotation = GetControlRotation(); 
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    if (MoveVector.Y != 0.0f) { MyPawn->AddMovementInput(ForwardDirection, MoveVector.Y); }
    if (MoveVector.X != 0.0f) { MyPawn->AddMovementInput(RightDirection, MoveVector.X); }
}
void AColorMageController::HandleLook(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    if (LookVector.X != 0.0f) { AddYawInput(LookVector.X); }
    if (LookVector.Y != 0.0f) { AddPitchInput(-LookVector.Y); } // 反转 Pitch
}

// --- [!! GDD 修正：新函数 !!] ---
/** (RMB) 处理汲取/混合请求 */
void AColorMageController::OnAcquire()
{
	AColorMageCharacter* MyCharacter = Cast<AColorMageCharacter>(GetPawn());

	// 2. 检查它是否是我们的法师角色 (平台/生物不能汲取颜色)
	if (MyCharacter)
	{
		// 3. 告诉角色去执行“汲取”动作（播放动画并设置计时器）
		MyCharacter->RequestAcquireColor();
	}
	else
	{
		// (如果正附身在平台上按 RMB，什么也不做)
		UE_LOG(LogTemp, Log, TEXT("OnAcquire: 只有法师角色才能汲取颜色。"));
	}
}

// --- [!! GDD 修正：新函数 !!] ---
/** (F) 处理附身请求 */
/** (F) 处理附身请求 */
/** (F) 处理附身/取消附身请求 */
void AColorMageController::OnPossessInteract()
{
	UE_LOG(LogTemp, Error, TEXT("=== OnPossessInteract 被调用 ==="));
    
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("当前没有控制任何Pawn"));
		return;
	}
    
	// [!! 检查当前是否在控制非法师角色 !!]
	if (!CurrentPawn->IsA<AColorMageCharacter>())
	{
		UE_LOG(LogTemp, Warning, TEXT("当前控制非法师角色: %s，执行取消附身"), *CurrentPawn->GetName());
        
		// 取消附身，返回法师角色
		RequestRepossessOriginalCharacter();
		return;
	}
    
	// [!! 如果控制的是法师角色，尝试附身其他对象 !!]
	UE_LOG(LogTemp, Error, TEXT("当前控制法师角色，检查附身目标"));
	UE_LOG(LogTemp, Error, TEXT("当前准星类型: %d"), (int32)CurrentTargetType);
    
	// 使用Tick中已经检测到的高亮对象
	if (CurrentTargetType == EReticleTargetType::Possessable && CurrentHighlightedActor.IsValid())
	{
		APawn* TargetPawn = Cast<APawn>(CurrentHighlightedActor.Get());
		if (TargetPawn)
		{
			UE_LOG(LogTemp, Error, TEXT("尝试附身目标: %s"), *TargetPawn->GetName());
            
			UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
			if (ColorManager)
			{
				ColorManager->AttemptPossession(this, TargetPawn);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("找不到ColorManagerSubsystem"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("高亮对象不是Pawn: %s"), *CurrentHighlightedActor.Get()->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("没有可附身目标 - 准星类型: %d, 高亮对象存在: %s"), 
			(int32)CurrentTargetType, 
			CurrentHighlightedActor.IsValid() ? TEXT("是") : TEXT("否"));
        
		if (CurrentHighlightedActor.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("高亮对象类型: %s"), *CurrentHighlightedActor.Get()->GetClass()->GetName());
		}
	}
}

// --- [!! GDD 修正结束 !!] ---
void AColorMageController::RequestRepossessOriginalCharacter()
{
	if (HiddenCharacter.IsValid())
	{
		AColorMageCharacter* CharacterToRepossess = HiddenCharacter.Get();
		APawn* CurrentPossessedPawn = GetPawn(); 
       
		// [!! 修复：默认退出变换现在是“当前 Pawn”的位置 !!]
		FTransform ExitTransform; 
		if (!CurrentPossessedPawn)
		{
			// 极端情况：Pawn 不见了？
			ExitTransform = CharacterToRepossess->GetActorTransform();
		}
		else
		{
			// 默认退出点是当前 Pawn 的位置
			ExitTransform = CurrentPossessedPawn->GetActorTransform();
		}
		// [!! 修复结束 !!]

		APossessablePawn* Possessable = nullptr;
		ACreatureCharacter* Creature = nullptr;

		if (CurrentPossessedPawn)
		{
			Possessable = Cast<APossessablePawn>(CurrentPossessedPawn);
			Creature = Cast<ACreatureCharacter>(CurrentPossessedPawn);
           
			// [!! 关键 !!] 检查 Pawn/Creature 是否设置了“自定义”退出点
			// 如果是，则使用它。如果不是，ExitTransform 保持为 Pawn 的当前位置。
			if (Possessable) 
			{ 
				ExitTransform = Possessable->GetCharacterExitTransform(); 
				Possessable->PlayUnpossessEffect();
			}
			else if (Creature) 
			{
				ExitTransform = Creature->GetCharacterExitTransform();
				Creature->PlayUnpossessEffect();
			}
		}
        
		Super::Possess(CharacterToRepossess);

		// [!! 关键 !!] 传送角色到（现在正确的）退出点
		CharacterToRepossess->TeleportTo(ExitTransform.GetLocation(), ExitTransform.GetRotation().Rotator(), false, true);
		CharacterToRepossess->PlayPossessEffect(); 
		HiddenCharacter = nullptr;
	}
}

void AColorMageController::EnableInput(APlayerController* PlayerController)
{
	Super::EnableInput(PlayerController);
	
	// 确保输入模式设置回“仅游戏”
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true); 
	SetInputMode(InputMode);
	
}
void AColorMageController::SetCameraForCreature(ACreatureCharacter* Creature)
{
	if (!Creature || !HiddenCharacter.IsValid()) return;
	// [!! 关键：直接从隐藏的法师角色获取摄像机设置 !!]
	AColorMageCharacter* MageCharacter = HiddenCharacter.Get();

	UCharacterMovementComponent* CreatureMoveComp = Creature->GetCharacterMovement();
	UCharacterMovementComponent* MageMoveComp = MageCharacter->GetCharacterMovement();
    
	if (CreatureMoveComp && MageMoveComp)
	{
		// 直接复制所有关键设置
		CreatureMoveComp->bUseControllerDesiredRotation = MageMoveComp->bUseControllerDesiredRotation;
		CreatureMoveComp->bOrientRotationToMovement = MageMoveComp->bOrientRotationToMovement;
		CreatureMoveComp->RotationRate = MageMoveComp->RotationRate;
        
		UE_LOG(LogTemp, Warning, TEXT("生物移动设置已同步到法师设置"));
	}
	
	if (USpringArmComponent* SpringArm = Creature->FindComponentByClass<USpringArmComponent>())
	{
		// 使用法师角色的摄像机设置，而不是生物自己的设置
		SpringArm->TargetArmLength = MageCharacter->GetCameraDistance();     // 600.0f
		SpringArm->SocketOffset = MageCharacter->GetCameraOffset();          // (0, 150, 60)
		SpringArm->CameraLagSpeed = MageCharacter->GetCameraLagSpeed();      // 3.0f
		SpringArm->bUsePawnControlRotation = true;
		SpringArm->bEnableCameraLag = true;
		SpringArm->bDoCollisionTest = true;
     
		UE_LOG(LogTemp, Warning, TEXT("生物 %s 使用法师摄像机设置: 距离=%f, 偏移=(%s)"), 
			*Creature->GetName(), 
			MageCharacter->GetCameraDistance(),
			*MageCharacter->GetCameraOffset().ToString());
	}
}

void AColorMageController::SetCameraForCharacterType(AColorMageCharacter* ColorMageChar)
{
	if (!ColorMageChar) 
	{
		UE_LOG(LogTemp, Error, TEXT("SetCameraForCharacterType: ColorMageChar 为空!"));
		return;
	}

	// [!! 修改：从ColorMageCharacter类直接获取摄像机设置 !!]
	if (USpringArmComponent* SpringArm = ColorMageChar->FindComponentByClass<USpringArmComponent>())
	{
		// 保存旧值用于调试
		float OldLength = SpringArm->TargetArmLength;
		FVector OldOffset = SpringArm->SocketOffset;
        
		// 使用角色定义的摄像机设置
		SpringArm->TargetArmLength = ColorMageChar->GetCameraDistance(); 
		SpringArm->SocketOffset = ColorMageChar->GetCameraOffset(); // 使用完整的偏移向量 (X, Y, Z)
		SpringArm->CameraLagSpeed = ColorMageChar->GetCameraLagSpeed();
		SpringArm->bEnableCameraLag = true;
		SpringArm->bUsePawnControlRotation = true;
		SpringArm->bDoCollisionTest = true;
		
		UE_LOG(LogTemp, Warning, TEXT("法师摄像机设置: %f->%f, Offset(%s)->(%s), 速度=%f"), 
			OldLength, SpringArm->TargetArmLength,
			*OldOffset.ToString(), *SpringArm->SocketOffset.ToString(),
			ColorMageChar->GetCameraLagSpeed());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("法师角色 %s 没有SpringArmComponent!"), *ColorMageChar->GetName());
        
		// 调试：列出所有组件
		TArray<UActorComponent*> Components = ColorMageChar->GetComponents().Array();
		for (UActorComponent* Component : Components)
		{
			UE_LOG(LogTemp, Warning, TEXT("找到组件: %s (类型: %s)"), 
				*Component->GetName(), *Component->GetClass()->GetName());
		}
	}
}


FString AColorMageController::GetInteractionPrompt() const
{
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		return TEXT("");
	}
    
	// 如果控制的不是法师角色，显示"取消附身"
	if (!CurrentPawn->IsA<AColorMageCharacter>())
	{
		if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(CurrentPawn))
		{
			return FString::Printf(TEXT("按 [F] 离开 %s"), *Creature->GetName());
		}
		else if (APossessablePawn* Platform = Cast<APossessablePawn>(CurrentPawn))
		{
			return FString::Printf(TEXT("按 [F] 离开平台"));
		}
		return TEXT("按 [F] 返回法师");
	}
    
	// 如果控制法师角色，检查是否有可附身目标
	if (CurrentTargetType == EReticleTargetType::Possessable && CurrentHighlightedActor.IsValid())
	{
		if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(CurrentHighlightedActor.Get()))
		{
			return FString::Printf(TEXT("按 [F] 附身 %s"), *Creature->GetName());
		}
		else if (APossessablePawn* Platform = Cast<APossessablePawn>(CurrentHighlightedActor.Get()))
		{
			return TEXT("按 [F] 使用平台");
		}
	}
    
	return TEXT("");
}
