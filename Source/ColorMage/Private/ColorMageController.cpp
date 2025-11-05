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

AColorMageController::AColorMageController()
{
	bShowMouseCursor = false;
	PrimaryActorTick.bCanEverTick = true;
}
void AColorMageController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 只在控制角色时才需要更新准星
	if (!Cast<AColorMageCharacter>(GetPawn()))
	{
		// 如果不在控制角色 (比如在附身平台)，
		// 确保准星变回 "None" 状态
		if (CurrentTargetType != EReticleTargetType::None)
		{
			CurrentTargetType = EReticleTargetType::None;
			OnReticleTargetChanged.Broadcast(CurrentTargetType);
		}
		return;
	}

	// 1. 设置射线
	FVector CameraLocation;
	FRotator CameraRotation;
	GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector TraceStart = CameraLocation;
	// [!! 注意 !!] 我们使用一个合理的交互距离，而不是 10000 (射击距离)
	// 否则你可能会瞄准到 100 米外的花
	float ReticleTraceDistance = 15500.0f; // 20 米 (你可以在.h设为UPROPERTY)
	FVector TraceEnd = TraceStart + (CameraRotation.Vector() * ReticleTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetPawn()); // 忽略玩家自己

	EReticleTargetType NewTargetType = EReticleTargetType::None; // 默认为 "无"

	// 2. 执行射线检测
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// 3. 分析击中的物体 (优先级从高到低)

			// 检查 1: 是否为可附身物体 (PossessablePawn)？
			if (APossessablePawn* PossPawn = Cast<APossessablePawn>(HitActor))
			{
				AColorMagePlayerState* PS = GetPlayerState<AColorMagePlayerState>();
				if (PS && PS->GetCurrentColor() != EColor::EC_None && PS->GetCurrentColor() == PossPawn->GetColor())
				{
					// 颜色匹配！最高优先级：可附身
					NewTargetType = EReticleTargetType::Possessable;
				}
			}
			// 检查 2: 是否为可上色的环境物体 (ColorableActor)？
			else if (AColorableActor* ColorActor = Cast<AColorableActor>(HitActor))
			{
				// 它是灰色的，“可上色”
				NewTargetType = EReticleTargetType::Paintable;
			}
			// 检查 3: 是否为颜色源 (ColorSourceActor)？
			else if (AColorSourceActor* SourceActor = Cast<AColorSourceActor>(HitActor))
			{
				// “可汲取”
				NewTargetType = EReticleTargetType::Extractable;
			}
			// (如果击中了其他东西，保持 NewTargetType = EReticleTargetType::None)
		}
	}

	// 4. [!! 关键 !!] 仅在状态 *改变* 时才广播委托
	if (NewTargetType != CurrentTargetType)
	{
		CurrentTargetType = NewTargetType;
		OnReticleTargetChanged.Broadcast(CurrentTargetType);
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
		if (AColorMageCharacter* ColorMageCharacter = Cast<AColorMageCharacter>(InPawn)) { PossessedType = ColorMageCharacter->GetControlType(); }
		else if (APossessablePawn* PossPawn = Cast<APossessablePawn>(InPawn)) { PossessedType = PossPawn->GetControlType(); }
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
		APossessablePawn* TargetPawn = Cast<APossessablePawn>(HitResult.GetActor());
		if (TargetPawn)
		{
			UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
			if (ColorManager)
			{
				ColorManager->AttemptPossession(this, TargetPawn);
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
		if (CurrentPossessedPawn)
		{
			APossessablePawn* Possessable = Cast<APossessablePawn>(CurrentPossessedPawn);
			if (Possessable) { ExitTransform = Possessable->GetCharacterExitTransform(); }
			else { ExitTransform = CurrentPossessedPawn->GetActorTransform(); ExitTransform.AddToTranslation(FVector(0,0,100)); }
		}
		
		Super::Possess(CharacterToRepossess);

		CharacterToRepossess->TeleportTo(ExitTransform.GetLocation(), ExitTransform.GetRotation().Rotator(), false, true);
		HiddenCharacter = nullptr;
	}
	else { /* ... (Log Warning) ... */ }
}