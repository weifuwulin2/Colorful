#include "ColorableActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ColorComponent.h"

AColorableActor::AColorableActor()
{
	// 默认不需要 Tick
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootSceneComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootSceneComponent);

	// [!! 关键 !!]
	// 在 C++ 构造函数中自动创建 ColorComponent
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));
}

/** 获取颜色的快捷方式 */
EColor AColorableActor::GetColor() const
{
	return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None;
}

/** 设置颜色的快捷方式 */
void AColorableActor::SetColor(EColor NewColor)
{
	if (ColorComponent)
	{
		ColorComponent->SetColor(NewColor);
	}
}