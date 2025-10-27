// ColorComponent.cpp
#include "ColorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h" // [!! 新增 !!] 包含网络头文件

UColorComponent::UColorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// [!! 新增 !!] 设置组件为可复制，这样 RepNotify 才能工作
	SetIsReplicatedByDefault(true);

	DefaultColor = EColor::EC_None;
	CurrentColor = DefaultColor;
}

// [!! 新增 !!] 设置要复制的变量
void UColorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 告诉引擎复制 CurrentColor 变量
	DOREPLIFETIME(UColorComponent, CurrentColor);
}

void UColorComponent::BeginPlay()
{
	Super::BeginPlay();

	// 1. 尝试自动寻找我们要控制的网格体
	if (!MeshToControl)
	{
		MeshToControl = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	}

	// 2. [!! 关键 !!] 在游戏开始时，应用你设置的“默认颜色”
	if (GetOwner()->HasAuthority()) 
	{
		// 2a. 直接将数据设置为默认颜色
		// (DefaultColor 是你在蓝图中配置的值)
		CurrentColor = DefaultColor; 
        
		// 2b. [!! 核心 !!] 强制调用一次 OnRep 函数
		// 这将绕过 SetColor() 中的“优化检查”
		// 并确保在游戏开始时 100% 会设置正确的默认材质
		OnRep_CurrentColor(); 
	}
}
// --- [!! 已修改 !!] ---
void UColorComponent::SetColor(EColor NewColor)
{
	// 仅在服务器（或权威端）上设置颜色
	if (GetOwner()->HasAuthority())
	{
		// 这里的优化检查是正确的，
		// 它可以防止在游戏过程中不必要的材质更新
		if (NewColor == CurrentColor)
		{
			return; 
		}

		CurrentColor = NewColor;
		OnRep_CurrentColor(); 
	}
}

// --- [!! 这就是你的“自动呼叫”函数 !!] ---
// 当 CurrentColor 改变时，它会在服务器和所有客户端上自动运行
void UColorComponent::OnRep_CurrentColor()
{
	// 1. 检查我们是否有一个有效的网格体来设置材质
	if (!MeshToControl)
	{
		// 第一次复制时，MeshToControl 可能还未设置
		// 再次尝试寻找它
		MeshToControl = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
		
		if (!MeshToControl)
		{
			UE_LOG(LogTemp, Warning, TEXT("ColorComponent 在 %s 上找不到 MeshToControl!"), *GetOwner()->GetName());
			return;
		}
	}

	// 2. 根据新颜色应用材质
	if (CurrentColor == EColor::EC_None)
	{
		// --- 情况 A: 颜色是“灰色” ---
		MeshToControl->SetMaterial(0, DefaultMaterial);
	}
	else
	{
		// --- 情况 B: 颜色是彩色的 ---
		if (TObjectPtr<UMaterialInterface>* FoundMaterial = ColorMaterials.Find(CurrentColor))
		{
			// 找到了！应用这个材质。
			MeshToControl->SetMaterial(0, *FoundMaterial);
		}
		else
		{
			// 没找到
			UE_LOG(LogTemp, Warning, TEXT("ColorComponent 在 %s 上缺少 EColor %d 对应的材质。"), *GetOwner()->GetName(), (int32)CurrentColor);
			MeshToControl->SetMaterial(0, DefaultMaterial);
		}
	}
}