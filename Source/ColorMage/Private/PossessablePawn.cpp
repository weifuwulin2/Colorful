#include "PossessablePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "ColorComponent.h"
#include "ColorMageController.h" // 需要包含它来调用解除附身
#include "EnhancedInputComponent.h"
#include "InputAction.h"



APossessablePawn::APossessablePawn()
{
	// 基类默认不 Tick，除非子类需要
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));

	CharacterExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterExitPoint"));
	CharacterExitPoint->SetupAttachment(RootComponent);
	// 设置一个默认的相对位置 (比如在 Mesh 上方 1 米)
	CharacterExitPoint->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	// 确保 Pawn 可以被附身
	bCanBePossessed = true;
}

EColor APossessablePawn::GetColor() const
{
	// 添加健壮性检查，防止 ColorComponent 无效时崩溃
	return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None;
}

void APossessablePawn::SetColor(EColor NewColor)
{
	// 添加健壮性检查
	if (ColorComponent)
	{
		ColorComponent->SetColor(NewColor);
	}
}

FTransform APossessablePawn::GetCharacterExitTransform() const
{
	if (CharacterExitPoint)
	{
		// 返回场景组件的世界变换 (位置和旋转)
		return CharacterExitPoint->GetComponentTransform();
	}

	// 如果退出点无效，提供一个备用位置 (比如 Pawn 当前位置上方)
	FTransform ExitTransform = GetActorTransform();
	ExitTransform.AddToTranslation(FVector(0,0,100)); // 向上偏移 1 米
	return ExitTransform;
}

void APossessablePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 基类只负责绑定“解除附身”输入
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (UnpossessAction)
		{
			EnhancedInputComp->BindAction(UnpossessAction, ETriggerEvent::Started, this, &APossessablePawn::OnUnpossess);
		}
		// 移动输入由子类 (如 APlatformPawn) 负责绑定
	}
}

void APossessablePawn::OnUnpossess()
{
	// 获取当前控制这个 Pawn 的控制器
	AController* MyController = GetController();
	if (MyController)
	{
		// 尝试将其转换为我们的特定玩家控制器
		AColorMageController* MageController = Cast<AColorMageController>(MyController);
		if (MageController)
		{
			// 调用控制器上的函数来请求解除附身并返回原始角色
			MageController->RequestRepossessOriginalCharacter();
		}
	}
}