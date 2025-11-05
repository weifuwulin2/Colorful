// Copyright 2023 Dev Levy. All Rights Reserved.

#include "AOSAC_OutlineOverlay.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "Components/StaticMeshComponent.h"
#include <Engine/World.h>
#include <TimerManager.h>
#include <Materials/MaterialInterface.h>
#include <UObject/ObjectHandle.h>
#include <UObject/ConstructorHelpers.h>
#include <Materials/MaterialInstance.h>

#include "Net/UnrealNetwork.h"


UAOSAC_OutlineOverlay::UAOSAC_OutlineOverlay(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	struct FConstructorAOS
	{
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> OutlineMaterial;

		FConstructorAOS()
			:OutlineMaterial(TEXT("/Script/Engine.MaterialInstanceConstant'/AdvancedOutlineSystem/Materials/MI_Outline_Overlay'"))
		{}
	};
	
	static FConstructorAOS ConsAOS;

	if (ConsAOS.OutlineMaterial.Succeeded())
	{
		AOSMaterialInstance = ConsAOS.OutlineMaterial.Get();
	}
	OutlineGlobalPreset = nullptr;
}

void UAOSAC_OutlineOverlay::LvCreateMID(const FLinearColor& InColor)
{
	if (CurrentHighLightColor == InColor || InColor.IsAlmostBlack())
	{
		return;
	}
	if (AOSMaterialInstance != nullptr)
	{
		AOSCustomMID = nullptr;
		AOSCustomMID = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), AOSMaterialInstance);
		AOSCustomMID->SetVectorParameterValue("Main Color", InColor);
		// UE_LOG(LogTemp, Warning, TEXT("Color Change Valid."));
	}
	CurrentHighLightColor = InColor;
}

void UAOSAC_OutlineOverlay::LvOutlinerInitSetting()
{
	Super::LvOutlinerInitSetting();
}


void UAOSAC_OutlineOverlay::OnRegister()
{
	Super::OnRegister();

	OnActiveOutlinerEvent.AddUniqueDynamic(this, &UAOSAC_OutlineOverlay::LvEnableOutlineOnActor);
	OnOutlineByTagEvent.AddUniqueDynamic(this, &UAOSAC_OutlineOverlay::LvEnableOutlineByTag);

}

void UAOSAC_OutlineOverlay::TickComponent(const float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
}

void UAOSAC_OutlineOverlay::BeginPlay()
{
	Super::BeginPlay();
	
	OnOverlayColorChanged.AddUniqueDynamic(this, &UAOSAC_OutlineOverlay::LvCreateMID);

	if (GetWorld() && GetOwner()->GetWorld())
	{
		if (bUseAOSDebugLine)
		{
			GetWorld()->GetTimerManager().SetTimer(AosTimerHandle, FTimerDelegate::CreateLambda([this]()
			{
				LvAOSLineTrace(TargetActorTag, HighlightColor, true, true, TraceTypeQuery1,AOSDebugLineDistance, true);
			}), AOSLineTraceRate, true);
		}	
	}
}

void UAOSAC_OutlineOverlay::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UAOSAC_OutlineOverlay::DestroyComponent(bool bPromoteChildren)
{
	Super::DestroyComponent(bPromoteChildren);
}


void UAOSAC_OutlineOverlay::LvChangeColor(FName TargetTag, FLinearColor InColor, bool ChangeDefault)
{
	LvEnableOutlineOnAllActorsWithTag(TargetTag, true, false, InColor);
}

void UAOSAC_OutlineOverlay::LvChangeOutlineColor(FLinearColor InColor, bool ChangeDefault)
{
	if (InColor == FLinearColor::Transparent || InColor.IsAlmostBlack())
	{
		OnOverlayColorChanged.Broadcast(HighlightColor);
		return;
	}

	if (CurrentHighLightColor != InColor)
	{
		if (!ChangeDefault)
		{
			OnOverlayColorChanged.Broadcast(InColor);
		}
		else
		{
			HighlightColor = InColor;
		}
	}
}

void UAOSAC_OutlineOverlay::LvDisableAllOutline()
{
	Super::LvDisableAllOutline();
}

TArray<AActor*> UAOSAC_OutlineOverlay::LvGetInteractableActors(FName TargetTag)
{
	return Super::LvGetInteractableActors(TargetTag);
}

TArray<AActor*> UAOSAC_OutlineOverlay::LvGetAllInteractableActors(FName TargetTag)
{
	return Super::LvGetAllInteractableActors(TargetTag);
}

void UAOSAC_OutlineOverlay::LvEnableOutlineByTag(AActor* ActorToOutline, FName TargetTag, bool EnableOutline,
                                                 bool IsToggle, FLinearColor Color)
{
	FLinearColor OverrideColor = Color;
	if (OverrideColor == FLinearColor::Transparent || OverrideColor.IsAlmostBlack())
	{
		OverrideColor = HighlightColor;
	}
	Super::LvEnableOutlineByTag(ActorToOutline, TargetTag, EnableOutline, IsToggle, OverrideColor);
}

void UAOSAC_OutlineOverlay::LvEnableOutline(UMeshComponent* TargetComp, const bool EnableOutline, FLinearColor Color)
{
	if (!TargetComp)
	{
		return;
	}

	if (EnableOutline)
	{
		if (UStaticMeshComponent* CurComp{Cast<UStaticMeshComponent>(TargetComp)}; CurComp)
		{
			CurComp->bDisallowNanite = true;
		}
		TargetComp->OverlayMaterial = nullptr;
		TargetComp->SetOverlayMaterial(MoveTemp(AOSCustomMID));
	}
	else
	{
		if (UStaticMeshComponent* CurComp{Cast<UStaticMeshComponent>(TargetComp)}; CurComp)
		{
			CurComp->bDisallowNanite = false;
		}
		TargetComp->SetOverlayMaterial(nullptr);
	}

}

void UAOSAC_OutlineOverlay::LvEnableOutlineOnAllActorsWithTag(FName TargetTag, bool EnableOutline, bool IsToggle,
	FLinearColor Color)
{
	Super::LvEnableOutlineOnAllActorsWithTag(TargetTag, EnableOutline, IsToggle, Color);
}

void UAOSAC_OutlineOverlay::LvAOSLineTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti,
	ETraceTypeQuery TraceTypeQuery, float LineDistance, const bool UseCameraFocus)
{
	Super::LvAOSLineTrace(TargetTag, InHighlightColor, IsDebug, IsTraceMulti, TraceTypeQuery, LineDistance, UseCameraFocus);
}

AActor* UAOSAC_OutlineOverlay::LvAOSLineTrace2(FName TargetTag, const FLinearColor InHighlightColor,
	const FVector CustomStartLocation, const bool IsDebug, const ECollisionChannel CollisionChannel, float LineDistance, const bool UseCameraFocus)
{
	return Super::LvAOSLineTrace2(TargetTag, InHighlightColor, CustomStartLocation, IsDebug, CollisionChannel,
	                              LineDistance, UseCameraFocus);
}


void UAOSAC_OutlineOverlay::LvAOS_SphereTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug,
                                              bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float InRadius)
{
	Super::LvAOS_SphereTrace(TargetTag, InHighlightColor, IsDebug, IsTraceMulti, TraceTypeQuery, InRadius);
}

void UAOSAC_OutlineOverlay::LvEnableOutlineOnActor(AActor* ActorToOutline, EAOSCheckOption ECheckOption, bool EnableOutline, bool IsToggle, FLinearColor Color)
{
	LvChangeOutlineColor(Color, false);
	Super::LvEnableOutlineOnActor(ActorToOutline, ECheckOption, EnableOutline, IsToggle, Color);
}



