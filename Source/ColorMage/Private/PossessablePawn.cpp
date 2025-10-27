// PossessablePawn.cpp
#include "PossessablePawn.h"
#include "Components/StaticMeshComponent.h"
#include "ColorComponent.h" // 包含新组件的头文件

APossessablePawn::APossessablePawn()
{
	PrimaryActorTick.bCanEverTick = true; 

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// --- [!! 新增 !!] ---
	// 创建我们的组件
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));

	bCanBePossessed = true;
}

// --- [!! 已修改 !!] ---
// 将“Get”请求转发给组件
EColor APossessablePawn::GetColor() const
{
	return ColorComponent->GetColor();
}

// --- [!! 已修改 !!] ---
// 将“Set”请求转发给组件
void APossessablePawn::SetColor(EColor NewColor)
{
	ColorComponent->SetColor(NewColor);
}