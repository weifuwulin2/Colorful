#include "ColorMageController.h"

#include "ColorManagerSubsystem.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h" // 包含 Subsystem
#include "EnhancedInputComponent.h"   // 包含 Enhanced Input Component
#include "Kismet/KismetSystemLibrary.h"

AColorMageController::AColorMageController()
{
	// 构造函数...
}

void AColorMageController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// --- 关键步骤：添加 Input Mapping Context ---

	// 1. 获取本地玩家的 Enhanced Input Subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// 2. 清除可能存在的旧映射 (可选，但通常是好习惯)
		Subsystem->ClearAllMappings();

		// 3. 检查我们的 IMC 资产是否有效
		if (DefaultInputMappingContext)
		{
			// 4. 添加我们的 IMC，优先级设为0
			Subsystem->AddMappingContext(DefaultInputMappingContext, 0);
		}
	}
}

void AColorMageController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// --- 关键步骤：绑定 Input Action ---

	// 1. 尝试将 PlayerInputComponent 转换为 UEnhancedInputComponent
	if (UEnhancedInputComponent* EnhancedInputComp = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		// 2. 检查我们的 IA_Move 资产是否有效
		if (MoveAction)
		{
			// 3. 绑定！
			// ETriggerEvent::Triggered 意味着在按键按下期间的每一帧都会触发
			EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleMove);
		}

		// Bind Interact (RMB). This is here so you can interact no matter what body you're in.
		if (InteractAction)
		{
			EnhancedInputComp->BindAction(InteractAction, ETriggerEvent::Started, this, &AColorMageController::OnInteract);
		}
	}
}

/**
 * 这是我们实际处理移动的地方
 */
void AColorMageController::HandleMove(const FInputActionValue& Value)
{
	// 1. 获取我们控制的Pawn
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	// 2. 从 Value 中获取 FVector2D
	const FVector2D MoveVector = Value.Get<FVector2D>();

	// 3. 获取控制器的旋转 (这通常代表摄像机的朝向)
	const FRotator Rotation = GetControlRotation();
	
	// 4. 我们只关心水平方向的旋转 (Yaw)，忽略摄像机的上下俯仰 (Pitch)
	const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);

	// 5. 从这个 Yaw 旋转中获取“前进”方向 (X轴)
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// 6. 从这个 Yaw 旋转中获取“右侧”方向 (Y轴)
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 7. 应用“前进/后退”移动输入
	if (MoveVector.Y != 0.0f)
	{
		MyPawn->AddMovementInput(ForwardDirection, MoveVector.Y);
	}
	
	// 8. 应用“左/右”移动输入 (这就是给A/D的)
	if (MoveVector.X != 0.0f)
	{
		MyPawn->AddMovementInput(RightDirection, MoveVector.X);
	}
}

// Interaction logic (RMB)
void AColorMageController::OnInteract()
{
	// 1. Perform Line Trace
	FVector CameraLocation;
	FRotator CameraRotation;
	GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector TraceStart = CameraLocation;
	FVector TraceEnd = TraceStart + (CameraRotation.Vector() * InteractionDistance);

	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this,
		TraceStart,
		TraceEnd,
		ETraceTypeQuery::TraceTypeQuery1, // Visibility channel
		false,
		TArray<AActor*>(),
		EDrawDebugTrace::None, // Change to ForDuration to debug
		HitResult,
		true
	);
	
	if (bHit && HitResult.GetActor())
	{
		// 2. Get the Color Manager Subsystem
		UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();

		if (ColorManager)
		{
			// 3. "Outsource" the logic to the subsystem
			ColorManager->HandlePlayerInteraction(this, HitResult.GetActor());
		}
	}
}