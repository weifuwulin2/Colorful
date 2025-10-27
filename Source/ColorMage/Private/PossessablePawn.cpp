// PossessablePawn.cpp
#include "PossessablePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h" // 包含材质的头文件

APossessablePawn::APossessablePawn()
{
	PrimaryActorTick.bCanEverTick = true; 

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	// 确保我们的Pawn可以被附身
	bCanBePossessed = true;
}

// --- [!! 关键逻辑 !!] ---
void APossessablePawn::SetColor(EColor NewColor)
{
	// 1. 如果颜色没有变化，就什么都不做 (优化)
	if (NewColor == CurrentColor)
	{
		return;
	}

	// 2. 更新Pawn的颜色状态
	CurrentColor = NewColor;

	// 3. 检查MeshComponent是否存在
	if (!MeshComponent)
	{
		return;
	}

	// 4. 根据新颜色应用材质
	if (NewColor == EColor::EC_None)
	{
		// --- 情况 A: 颜色是“灰色” ---
		// 应用默认材质
		MeshComponent->SetMaterial(0, DefaultMaterial);
	}
	else
	{
		// --- 情况 B: 颜色是彩色的 ---
		// 尝试在我们的 TMap 中查找这个颜色
		if (TObjectPtr<UMaterialInterface>* FoundMaterial = ColorMaterials.Find(NewColor))
		{
			// 找到了！应用这个材质。
			MeshComponent->SetMaterial(0, *FoundMaterial);
		}
		else
		{
			// 没找到 (比如你设置了 EC_Yellow，但在TMap里忘了加)
			// 最好是打个Log，然后恢复到默认材质
			UE_LOG(LogTemp, Warning, TEXT("Pawn %s: 缺少 EColor %d 对应的材质，将使用默认材质。"), *GetName(), (int32)NewColor);
			MeshComponent->SetMaterial(0, DefaultMaterial);
		}
	}
}