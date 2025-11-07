#include "ColorComponent.h"

#include "NiagaraComponent.h"
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
	CurrentAmbientVFXComponent = nullptr;
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
	
	if (PreviousColor != CurrentColor)
	{
		// 检查 DefaultPaintVFX 资产是否被指定
		if (DefaultPaintVFX)
		{
			// Use SpawnSystemAttached to attach the VFX to the MeshComponent
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				DefaultPaintVFX,                  // The Niagara System to spawn
				MeshToControl,                    // The component to attach to (our mesh)
				NAME_None,                        // Optional socket name
				FVector(0.f),                     // Location offset (relative to attach point)
				FRotator(0.f),                    // Rotation offset
				EAttachLocation::SnapToTarget,    // Snap to the component
				true                              // Auto-destroy when finished
			);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ColorComponent %s: 想要播放VFX，但 DefaultPaintVFX 未在蓝图中指定!"), *GetOwner()->GetName());
		}
	}

	if (CurrentAmbientVFXComponent)
	{
		// 销毁它
		CurrentAmbientVFXComponent->DestroyComponent();
		CurrentAmbientVFXComponent = nullptr;
	}

	// B. [!! 查找并附加新的VFX !!]
	// 检查新颜色是否在我们的 TMap 中有对应的持续特效
	if (CurrentColor != EColor::EC_None)
	{
		if (TObjectPtr<UNiagaraSystem>* FoundAmbientVFX = AmbientColorVFX.Find(CurrentColor))
		{
			if (*FoundAmbientVFX)
			{
				// [!! 关键 !!] 生成新的持续VFX
				CurrentAmbientVFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
					*FoundAmbientVFX,                 // 你的 (火焰/光晕) 特效
					MeshToControl,                    // 附加到网格体
					NAME_None,
					FVector(0.f),                     // 相对位置
					FRotator(0.f),                    // 相对旋转
					EAttachLocation::SnapToTarget,
					false, // [!! 关键 !!] 自动销毁 (bAutoDestroy = false) - 它会一直播放
					true   // 自动激活
				);
				UE_LOG(LogTemp, Log, TEXT("ColorComponent %s: 已附加持续VFX"), *GetOwner()->GetName());
			}
		}
	}
	// 3. 调用蓝图事件 (用于物理效果)
	K2_OnColorEffectChanged(CurrentColor, PreviousColor);

	// 4. 广播C++委托 (用于C++移动)
	OnColorChanged.Broadcast(CurrentColor, PreviousColor);

	// 5. 更新“上一次”的颜色
	PreviousColor = CurrentColor;
}