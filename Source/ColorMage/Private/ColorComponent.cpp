#include "ColorComponent.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

UColorComponent::UColorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	DefaultColor = EColor::EC_None;
	CurrentColor = DefaultColor;
	PreviousColor = DefaultColor;
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
		PreviousColor = CurrentColor;
		CurrentColor = NewColor;
		OnRep_CurrentColor();
	}
}

void UColorComponent::OnRep_CurrentColor()
{
	if (!MeshToControl)
	{
		MeshToControl = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
		if (!MeshToControl) { return; }
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
	
	// 2. 播放 VFX (使用 Niagara)
	UNiagaraSystem* VFXToSpawn = nullptr; // [!! 修改 !!] 类型为 UNiagaraSystem
	if (CurrentColor == EColor::EC_None)
	{
		VFXToSpawn = DefaultPaintVFX; // [!! 修改 !!] 使用 Niagara 属性
	}
	else
	{
		// [!! 修改 !!] 从 TMap<EColor, UNiagaraSystem*> 中查找
		if (TObjectPtr<UNiagaraSystem>* FoundVFX = ColorPaintVFX.Find(CurrentColor))
		{
			VFXToSpawn = *FoundVFX;
		}
	}

	if (VFXToSpawn)
	{
		// [!! 修改 !!] 使用 UNiagaraFunctionLibrary::SpawnSystemAtLocation 来播放
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			VFXToSpawn,
			GetOwner()->GetActorLocation(),
			GetOwner()->GetActorRotation()
		);
	}

	// 3. 调用蓝图事件 (用于物理效果)
	K2_OnColorEffectChanged(CurrentColor, PreviousColor);

	// 4. 广播C++委托 (用于C++移动)
	OnColorChanged.Broadcast(CurrentColor, PreviousColor);

	// 5. 更新“上一次”的颜色
	PreviousColor = CurrentColor;
}