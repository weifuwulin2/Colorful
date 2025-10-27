#include "ColorMageController.h"
#include "ColorManagerSubsystem.h" // 需要包含它来进行交互
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/KismetSystemLibrary.h" // 用于射线检测
#include "ColorMageCharacter.h"        // 需要包含角色头文件
#include "PossessablePawn.h"           // 需要包含 Pawn 头文件以获取退出点

AColorMageController::AColorMageController()
{
	bShowMouseCursor = false;
}

void AColorMageController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 添加输入映射上下文
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		if (DefaultInputMappingContext)
		{
			Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
		}
	}
	
    // 设置相机限制
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -70.0f;
		PlayerCameraManager->ViewPitchMax = 20.0f;
	}
	
	// 设置输入模式并隐藏鼠标
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true); 
	SetInputMode(InputMode);
}

void AColorMageController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComp = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		// 绑定移动、交互、观看
		if (MoveAction)
		{
			EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleMove);
		}
		if (InteractAction)
		{
			EnhancedInputComp->BindAction(InteractAction, ETriggerEvent::Started, this, &AColorMageController::OnInteract);
		}
		if (LookAction)
		{
			EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleLook);
		}
	}
}

void AColorMageController::HandleMove(const FInputActionValue& Value)
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	const FVector2D MoveVector = Value.Get<FVector2D>();

	// --- [!! FIX !!] ---
	// Rename the local variable from 'ControlRotation' to 'Rotation'
	const FRotator Rotation = GetControlRotation();
	// --- [!! END FIX !!] ---

	const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f); // Use the new variable name here too
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (MoveVector.Y != 0.0f) { MyPawn->AddMovementInput(ForwardDirection, MoveVector.Y); }
	if (MoveVector.X != 0.0f) { MyPawn->AddMovementInput(RightDirection, MoveVector.X); }
}

void AColorMageController::OnInteract()
{
    // 执行射线检测
	FVector CameraLocation;
	FRotator CameraRotation;
	GetPlayerViewPoint(CameraLocation, CameraRotation);
	FVector TraceStart = CameraLocation;
	FVector TraceEnd = TraceStart + (CameraRotation.Vector() * InteractionDistance);
	TArray<AActor*> ActorsToIgnore;
	APawn* MyPawn = GetPawn();
	if (MyPawn) { ActorsToIgnore.Add(MyPawn); }
	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this, TraceStart, TraceEnd, ETraceTypeQuery::TraceTypeQuery1, 
		false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true
	);

	if (bHit && HitResult.GetActor())
	{
		// 将交互逻辑委托给子系统
		UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
		if (ColorManager)
		{
			ColorManager->HandlePlayerInteraction(this, HitResult.GetActor());
		}
	}
}

void AColorMageController::HandleLook(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    if (LookVector.X != 0.0f) { AddYawInput(LookVector.X); }
    if (LookVector.Y != 0.0f) { AddPitchInput(-LookVector.Y); } // 反转 Pitch
}

/** 处理重新附身请求 */
void AColorMageController::RequestRepossessOriginalCharacter()
{
	// 检查是否有有效的隐藏角色引用
	if (HiddenCharacter.IsValid())
	{
		AColorMageCharacter* CharacterToRepossess = HiddenCharacter.Get();
		
		// 获取当前附身的 Pawn (应该是 APossessablePawn 或其子类)
		APawn* CurrentPossessedPawn = GetPawn(); 
		FTransform ExitTransform = CharacterToRepossess->GetActorTransform(); // 准备一个默认变换

		if (CurrentPossessedPawn)
		{
			// 尝试将其转换为 APossessablePawn 以获取退出点
			APossessablePawn* Possessable = Cast<APossessablePawn>(CurrentPossessedPawn);
			if (Possessable)
			{
				// 从 Pawn 获取预设的退出变换
				ExitTransform = Possessable->GetCharacterExitTransform();
			}
			else
			{
				// 如果当前 Pawn 不是 APossessablePawn (理论上不应该发生)，
				// 使用 Pawn 当前位置上方作为备用退出点
				ExitTransform = CurrentPossessedPawn->GetActorTransform();
				ExitTransform.AddToTranslation(FVector(0,0,100));
			}
			
			// 解除对当前 Pawn 的附身
			UnPossess();
		}
		
		// 重新附身到原始角色
		// 这将触发 AColorMageCharacter::PossessedBy() 来取消隐藏
		Possess(CharacterToRepossess); 

		// 在附身之后，将角色传送到指定的退出点
		// 使用 TeleportTo 可以确保位置和旋转都正确应用
		CharacterToRepossess->TeleportTo(ExitTransform.GetLocation(), ExitTransform.GetRotation().Rotator(), false, true);

		// 清除存储的弱指针引用
		HiddenCharacter = nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("无法重新附身：找不到有效的 HiddenCharacter。可能是重复按下解除键或状态错误。"));
	}
}