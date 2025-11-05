#include "PossessablePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "ColorComponent.h"
#include "ColorMageController.h"
#include "ColorMageGameMode.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Kismet/GameplayStatics.h"

APossessablePawn::APossessablePawn()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));
	CharacterExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterExitPoint"));
	CharacterExitPoint->SetupAttachment(RootComponent);
	CharacterExitPoint->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	bCanBePossessed = true;
	ControlType = EPawnControlType::Unknown; // 基类默认为 Unknown
}

EColor APossessablePawn::GetColor() const 
{ 
	return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None; 
}
void APossessablePawn::SetColor(EColor NewColor) 
{ 
	if (ColorComponent) { ColorComponent->SetColor(NewColor); } 
}
FTransform APossessablePawn::GetCharacterExitTransform() const 
{ 
	if (CharacterExitPoint) { return CharacterExitPoint->GetComponentTransform(); }
	FTransform ExitTransform = GetActorTransform();
	ExitTransform.AddToTranslation(FVector(0,0,100));
	return ExitTransform;
}

void APossessablePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// --- [!! GDD 修正 !!] ---
		// 绑定 PossessAction (F 键) 到 OnUnpossess 函数
		if (PossessAction) 
		{
			EnhancedInputComp->BindAction(PossessAction, ETriggerEvent::Started, this, &APossessablePawn::OnUnpossess);
		}
		// --- [!! GDD 修正结束 !!] ---
	}
}

void APossessablePawn::OnUnpossess()
{
	// (函数实现保持不变)
	AController* MyController = GetController();
	if (MyController)
	{
		AColorMageController* MageController = Cast<AColorMageController>(MyController);
		if (MageController)
		{
			MageController->RequestRepossessOriginalCharacter();
		}
	}
}

void APossessablePawn::FellOutOfWorld(const class UDamageType& dmgType)
{
	UE_LOG(LogTemp, Warning, TEXT("PossessablePawn %s 掉出世界!"), *GetName());

	// 获取 GameMode
	AGameModeBase* CurrentGameModeBase = UGameplayStatics::GetGameMode(this);
	AColorMageGameMode* MyGameMode = Cast<AColorMageGameMode>(CurrentGameModeBase); // 确保名字是 AColorMageGameMode

	if (MyGameMode)
	{
		// 获取控制这个 Pawn 的 Controller
		AController* MyController = GetController();
		if (MyController)
		{
			// [!! 关键 !!]
			// 调用 GameMode 的重生函数。
			// GameMode 中的 RespawnPlayer 逻辑已经知道如何处理
			// “玩家正附身于一个 Pawn” 的情况。
			MyGameMode->RespawnPlayer(MyController);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("FellOutOfWorld (Pawn): 无法获取 Controller 来重生!"));
			// 备用方案：如果由于某种原因没有控制器，就销毁自己
			Destroy(); 
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FellOutOfWorld (Pawn): 无法获取 MyGameMode!"));
		// 备用方案：调用基类实现（销毁 Actor）
		Super::FellOutOfWorld(dmgType);
	}
}

void APossessablePawn::OnHighlight_Implementation()
{
	IHighlightableInterface::OnHighlight_Implementation();
}

void APossessablePawn::OnUnhighlight_Implementation()
{
	IHighlightableInterface::OnUnhighlight_Implementation();
}

