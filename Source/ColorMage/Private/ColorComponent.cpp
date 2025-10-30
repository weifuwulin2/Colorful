#include "ColorComponent.h"
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
	
	// 2. 播放 VFX
	UParticleSystem* VFXToSpawn = nullptr;
	if (CurrentColor == EColor::EC_None)
	{
		VFXToSpawn = DefaultPaintVFX;
	}
	else
	{
		if (TObjectPtr<UParticleSystem>* FoundVFX = ColorPaintVFX.Find(CurrentColor))
		{
			VFXToSpawn = *FoundVFX;
		}
	}
	if (VFXToSpawn)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), VFXToSpawn, GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation());
	}

	// 3. 调用蓝图事件 (用于物理效果)
	K2_OnColorEffectChanged(CurrentColor, PreviousColor);

	// 4. 广播C++委托 (用于C++移动)
	OnColorChanged.Broadcast(CurrentColor, PreviousColor);

	// 5. 更新“上一次”的颜色
	PreviousColor = CurrentColor;
}