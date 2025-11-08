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
		}
		else if (APossessablePawn* PossPawn = Cast<APossessablePawn>(InPawn)) 
		{ 
			PossessedType = PossPawn->GetControlType(); 
		}
		// [!! 新增：支持CreatureCharacter !!]
		else if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(InPawn))
		{
			PossessedType = Creature->GetControlType();
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
void AColorMageController::OnPossessInteract()
{
	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	FVector TraceStart = CamLoc;
	FVector TraceEnd = TraceStart + (CamRot.Vector() * InteractionDistance);
	TArray<AActor*> ActorsToIgnore;
	APawn* MyPawn = GetPawn(); if (MyPawn) { ActorsToIgnore.Add(MyPawn); }
	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this, TraceStart, TraceEnd, ETraceTypeQuery::TraceTypeQuery1,
		false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true
	);
	if (bHit && HitResult.GetActor())
	{
		// [!! 简化：统一处理两种类型 !!]
		APawn* TargetPawn = Cast<APawn>(HitResult.GetActor());
		if (TargetPawn && 
			(TargetPawn->IsA<APossessablePawn>() || TargetPawn->IsA<ACreatureCharacter>()))
		{
			UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
			if (ColorManager)
			{
				ColorManager->AttemptPossession(this, TargetPawn); // [!! 统一调用 !!]
			}
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
		FTransform ExitTransform = CharacterToRepossess->GetActorTransform(); 
        
		APossessablePawn* Possessable = nullptr;
		ACreatureCharacter* Creature = nullptr; // [!! 新增 !!]
		if (CurrentPossessedPawn)
		{
			Possessable = Cast<APossessablePawn>(CurrentPossessedPawn);
			if (Possessable) 
			{ 
				ExitTransform = Possessable->GetCharacterExitTransform(); 
			}
			// [!! 新增：处理CreatureCharacter !!]
			else if (Creature == Cast<ACreatureCharacter>(CurrentPossessedPawn))
			{
				ExitTransform = Creature->GetCharacterExitTransform();
			}
			else 
			{ 
				/* ... (备用退出点) ... */ 
			}
		}
        
		// --- [!! 关键修复：VFX !!] ---
		// 1. 在"当前Pawn" (平台/生物) 的位置播放解除附身特效
		if (Possessable)
		{
			Possessable->PlayUnpossessEffect();
		}
		// [!! 新增：CreatureCharacter的特效 !!]
		else if (Creature)
		{
			Creature->PlayUnpossessEffect();
		}
        
		Super::Possess(CharacterToRepossess);
		// [!! 关键 !!] 先传送，再播放特效
		CharacterToRepossess->TeleportTo(ExitTransform.GetLocation(), ExitTransform.GetRotation().Rotator(), false, true);
		CharacterToRepossess->PlayPossessEffect(); // 在新位置播放
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
