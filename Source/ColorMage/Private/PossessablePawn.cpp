#include "PossessablePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "ColorComponent.h"
#include "ColorMageController.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"

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