#include "ColorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UColorComponent::UColorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	DefaultColor = EColor::EC_None;
	CurrentColor = DefaultColor;
	PreviousColor = DefaultColor; // 初始化
}

void UColorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UColorComponent, CurrentColor);
}

void UColorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!MeshToControl)
	{
		MeshToControl = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	}

	// 强制应用默认颜色
	if (GetOwner()->HasAuthority())
	{
		PreviousColor = DefaultColor; 
		CurrentColor = DefaultColor;
		OnRep_CurrentColor();
	}
}

void UColorComponent::SetColor(EColor NewColor)
{
	if (GetOwner()->HasAuthority())
	{
		if (NewColor == CurrentColor) return;
		
		PreviousColor = CurrentColor; // 记录旧颜色
		CurrentColor = NewColor;
		OnRep_CurrentColor();
	}
}

void UColorComponent::OnRep_CurrentColor()
{
	if (!MeshToControl)
	{
		MeshToControl = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
		if (!MeshToControl)
		{
			UE_LOG(LogTemp, Warning, TEXT("ColorComponent 在 %s 上找不到 MeshToControl!"), *GetOwner()->GetName());
			return;
		}
	}

	// 1. 更新材质
	UMaterialInterface* MaterialToApply = DefaultMaterial; 
	if (CurrentColor != EColor::EC_None)
	{
		if (TObjectPtr<UMaterialInterface>* FoundMaterial = ColorMaterials.Find(CurrentColor))
		{
			MaterialToApply = *FoundMaterial; 
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ColorComponent 在 %s 上缺少 EColor %d 对应的材质。"), *GetOwner()->GetName(), (int32)CurrentColor);
		}
	}
	MeshToControl->SetMaterial(0, MaterialToApply);
	
	// 2. [!! GDD 修正 !!] 调用蓝图事件以应用物理/元素效果
	OnColorEffectChanged(CurrentColor, PreviousColor);

	// 3. 更新“上一次”的颜色
	PreviousColor = CurrentColor;
}