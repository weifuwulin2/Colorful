// Copyright 2023 Dev Levy. All Rights Reserved.


#include "AOSA_WorldOutliner.h"

#include "Engine/World.h"
#include "Components/PostProcessComponent.h"
#include "Kismet/KismetMaterialLibrary.h"

#include <Materials/MaterialInterface.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <UObject/ObjectHandle.h>
#include <UObject/ConstructorHelpers.h>
#include <Materials/MaterialInstance.h>

struct FPostProcessSettings;
struct FWeightedBlendable;

AAOSA_WorldOutliner::AAOSA_WorldOutliner(const FObjectInitializer&ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	SetTickableWhenPaused(true);
	
    if (!AOS_PostProcess)
    {
	    AOS_PostProcess = CreateDefaultSubobject<UPostProcessComponent>("AOSPostProcess");
    	AOS_PostProcess->bAutoActivate = true;
    }
	this->SetRootComponent(AOS_PostProcess);
	RootComponent = AOS_PostProcess;
	
	bUseSceneDepth = true;

	
	struct FConstructorAOSActor
	{
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> OutlineMaterial1;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> OutlineMaterial2;
		
		FConstructorAOSActor()
			:OutlineMaterial1(TEXT("/Script/Engine.MaterialInstanceConstant'/AdvancedOutlineSystem/Materials/MI_Multi_Base'"))
		,OutlineMaterial2(TEXT("/Script/Engine.MaterialInstanceConstant'/AdvancedOutlineSystem/Materials/MI_Multi_SceneDepth'"))
		{
		}
	};
	static FConstructorAOSActor ConsAOS;
	
	if (ConsAOS.OutlineMaterial1.Succeeded() && ConsAOS.OutlineMaterial2.Succeeded())
	{
		AOS_OutlineMaterial1 = ConsAOS.OutlineMaterial1.Get();
		AOS_OutlineMaterial2 = ConsAOS.OutlineMaterial2.Get();
	}
}

void AAOSA_WorldOutliner::SetUsingSceneDepth(const bool UseSceneDepth)
{
	if (!AOS_PostProcess || AOS_OutlineMID == nullptr || AOS_Outline2MID == nullptr)
	{
		return;
	}
	
	FPostProcessSettings PostProcessSettings;
	
	PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, UseSceneDepth ? AOS_OutlineMID : AOS_Outline2MID));
	AOS_PostProcess->Settings = PostProcessSettings;

}

void AAOSA_WorldOutliner::SetInitialSettings(const bool UseSceneDepth)
{
	if (!AOS_PostProcess || AOS_OutlineMaterial1 == nullptr || AOS_OutlineMaterial2 == nullptr)
	{
		return;
	}
	OnOutlineColorChanged.AddUniqueDynamic(this, &AAOSA_WorldOutliner::SetMaterialColor);
	
	UObject* CurMaterial = UseSceneDepth ? AOS_OutlineMaterial1 : AOS_OutlineMaterial2;
	
	FPostProcessSettings PostProcessSettings;
	PostProcessSettings.bOverride_DepthOfFieldNearBlurSize = true;
	PostProcessSettings.bOverride_DepthOfFieldFocalRegion = true;
	PostProcessSettings.bOverride_DepthOfFieldScale = true;
	PostProcessSettings.DepthOfFieldNearBlurSize = 17.f;
	PostProcessSettings.bOverride_AmbientCubemapTint = true;
	
	PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, CurMaterial));
	AOS_PostProcess->Settings = PostProcessSettings;
}


void AAOSA_WorldOutliner::SetMaterialColor(const FLinearColor InFColor)
{
	// SetUsingSceneDepth(bUseSceneDepth);
	if (AOS_OutlineMaterial1)
	{
		UMaterialInstanceDynamic* NewMID = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), AOS_OutlineMaterial1);
		AOS_OutlineMID = NewMID;
		AOS_OutlineMID->SetVectorParameterValue(FName("Main Color"), InFColor);
	}
	if (AOS_OutlineMaterial2)
	{
		UMaterialInstanceDynamic* NewMID = UKismetMaterialLibrary::CreateDynamicMaterialInstance(GetWorld(), AOS_OutlineMaterial2);
		AOS_Outline2MID = NewMID;
		AOS_Outline2MID->SetVectorParameterValue(FName("Main Color"), InFColor);
	}
	CurrentHighLightColor = InFColor;
}

void AAOSA_WorldOutliner::UpdateMaterialColor(const FLinearColor InColor)
{
	OnOutlineColorChanged.Broadcast(InColor);
}

void AAOSA_WorldOutliner::UpdateHighlightColor(const FLinearColor InColor)
{
	if (HighLightColor != InColor)
	{
		HighLightColor = InColor;
		// OnOutlineColorChanged.Broadcast(InColor);
	}
}


void AAOSA_WorldOutliner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AAOSA_WorldOutliner::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		SetInitialSettings(bUseSceneDepth);
		OnOutlineColorChanged.Broadcast(HighLightColor);
	}

	
}

void AAOSA_WorldOutliner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	this->Destroy();
	Super::EndPlay(EndPlayReason);
}

void AAOSA_WorldOutliner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

