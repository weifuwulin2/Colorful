#include "PlatformPawn.h"
#include "BasePawnMovementComponent.h" // 包含移动组件基类
// 你可能还需要包含 HorizontalMovementComponent.h 或 VerticalMovementComponent.h
// 如果你想在 C++ 中直接创建它们的话
#include "EnhancedInputComponent.h"
#include "InputAction.h"

APlatformPawn::APlatformPawn()
{
	// 子类可以 Tick (如果移动组件需要)
	PrimaryActorTick.bCanEverTick = true;

	// [!! 关键 !!] 在构造函数中创建你想要的移动组件
	// 示例：默认创建基础组件，你可以在蓝图中替换它
	PlatformMovementComponent = CreateDefaultSubobject<UBasePawnMovementComponent>(TEXT("PlatformMovement"));
	// 或者，如果你确定所有平台都用水平移动：
	// #include "HorizontalMovementComponent.h" // 需要包含头文件
	// PlatformMovementComponent = CreateDefaultSubobject<UHorizontalMovementComponent>(TEXT("HorizontalMovement"));
}

void APlatformPawn::BeginPlay()
{
	// 调用父类的 BeginPlay (虽然父类现在是空的，但这是好习惯)
	Super::BeginPlay();

	// 再次尝试查找移动组件 (以防 C++ 构造函数未创建或蓝图替换了它)
	if (!PlatformMovementComponent)
	{
		PlatformMovementComponent = FindComponentByClass<UBasePawnMovementComponent>();
	}

	if (!PlatformMovementComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlatformPawn %s 无法找到 PlatformMovementComponent!"), *GetName());
	}
}


void APlatformPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// [!! 关键 !!] 先调用父类的 SetupInputComponent 来绑定 Unpossess
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 子类负责绑定移动
		if (MoveAction && PlatformMovementComponent)
		{
			EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlatformPawn::HandlePlatformMove);
		}
		else if (!MoveAction)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlatformPawn %s: MoveAction 未在蓝图中设置!"), *GetName());
		}
		else if (!PlatformMovementComponent)
		{
			UE_LOG(LogTemp, Warning, TEXT("PlatformPawn %s: PlatformMovementComponent 无效，无法绑定移动!"), *GetName());
		}
	}
}

void APlatformPawn::HandlePlatformMove(const FInputActionValue& Value)
{
	// 直接将输入传递给组件
	if (PlatformMovementComponent)
	{
		PlatformMovementComponent->AddMovementInput(Value);
	}
}