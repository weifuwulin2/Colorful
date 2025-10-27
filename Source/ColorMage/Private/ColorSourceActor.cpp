// ColorSourceActor.cpp
#include "ColorSourceActor.h"
#include "Components/StaticMeshComponent.h"
#include "ColorComponent.h" // 包含新组件的头文件

AColorSourceActor::AColorSourceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// --- [!! 新增 !!] ---
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));
}

// --- [!! 已修改 !!] ---
EColor AColorSourceActor::GetColor() const
{
	return ColorComponent->GetColor();
}