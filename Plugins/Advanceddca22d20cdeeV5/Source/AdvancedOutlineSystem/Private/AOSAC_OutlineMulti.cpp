// Copyright 2023 Dev Levy. All Rights Reserved.


#include "AOSAC_OutlineMulti.h"

#include "AOSA_WorldOutliner.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include <TimerManager.h>
#include <Engine/World.h>
#include <Materials/MaterialInterface.h>
#include <UObject/ObjectHandle.h>
#include <UObject/ConstructorHelpers.h>
#include <Materials/MaterialInstance.h>



#include "Net/UnrealNetwork.h"

UAOSAC_OutlineMulti::UAOSAC_OutlineMulti(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}


TArray<AActor*> UAOSAC_OutlineMulti::LvGetAllInteractableActors(FName TargetTag)
{
	return Super::LvGetAllInteractableActors(TargetTag);
}


void UAOSAC_OutlineMulti::LvChangeOutlineColor(FLinearColor InColor, bool ChangeDefault)
{
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		bChangeDefault = ChangeDefault;
		Server_SetNewOutlineColor(InColor);
	}
	
	if (GetOwner()->HasAuthority())
	{
		NewOutlineColor = InColor;
		bChangeDefault = ChangeDefault;
		OnRep_NewOutlineColor();
	}
	LvSwitchColor(InColor);
}

void UAOSAC_OutlineMulti::LvSwitchColor(const FLinearColor& InColor)
{
	if (AAOSA_WorldOutliner* CurPPVActor{GetPPVActor()})
	{
		CurPPVActor->SetUsingSceneDepth(bUseSceneDepth);
	}
	//@TODO :check this if change default should be true.

	LvChangeColor_Internal(InColor, bChangeDefault);
}

void UAOSAC_OutlineMulti::OnRep_NewOutlineColor()
{
	LvSwitchColor(NewOutlineColor);

	if (!GetOwner()->HasAuthority())
	{
		LvSwitchColor(NewOutlineColor);
	}
}
void UAOSAC_OutlineMulti::Server_SetNewOutlineColor_Implementation(const FLinearColor InColor)
{
	NewOutlineColor = InColor;
	
	OnRep_NewOutlineColor();
	
	LvSwitchColor(NewOutlineColor);
}

bool UAOSAC_OutlineMulti::Server_SetNewOutlineColor_Validate(const FLinearColor InColor)
{
	return true;
}

void UAOSAC_OutlineMulti::LvChangeColor_Internal(const FLinearColor InColor, const bool ChangeDefault)
{
	AAOSA_WorldOutliner* CurPPVActor{GetPPVActor()};
	if (CurPPVActor == nullptr)
	{
		return;
	}
	if (InColor == FLinearColor::Transparent || InColor.IsAlmostBlack())
	{
		CurPPVActor->UpdateMaterialColor(HighlightColor);
	}
	else
	{
		if (ChangeDefault)
		{
			CurPPVActor->UpdateMaterialColor(InColor);
			//Set Defualt Value
			HighlightColor = InColor;
			CurPPVActor->UpdateHighlightColor(InColor);
		}
		else
		{
			CurPPVActor->UpdateMaterialColor(InColor);
		}
	}
}

void UAOSAC_OutlineMulti::OnRegister()
{
	Super::OnRegister();
	OnActiveOutlinerEvent.AddUniqueDynamic(this, &UAOSAC_OutlineMulti::LvEnableOutlineOnActor);
	OnOutlineByTagEvent.AddUniqueDynamic(this, &UAOSAC_OutlineMulti::LvEnableOutlineByTag);

}

void UAOSAC_OutlineMulti::LvOutlinerInitSetting()
{
	Super::LvOutlinerInitSetting();
	
	if (UWorld* CurWorld{GetOwner()->GetWorld()}; CurWorld && GetWorld())
	{
		if (UGameplayStatics::GetActorOfClass(CurWorld, AAOSA_WorldOutliner::StaticClass()) == nullptr)
		{
			if (AOS_PPVClass)
			{
				CurWorld->SpawnActor<AAOSA_WorldOutliner>(AOS_PPVClass);
				return;
			}
			CurWorld->SpawnActor<AAOSA_WorldOutliner>(AAOSA_WorldOutliner::StaticClass());
			
			UE_LOG(LogTemp, Warning, TEXT("[%s] : AOS PostProcess Volume Is Not Valid. Set AOS_PPVClass"), *GetNameSafe(this));
		}
	}
	
	OnMultiColorChanged.AddUniqueDynamic(this, &UAOSAC_OutlineMulti::LvSwitchColor);



}

AAOSA_WorldOutliner* UAOSAC_OutlineMulti::GetPPVActor()
{
	if (GetOwner()->GetWorld() != nullptr && GetWorld())
	{
		PPVActor = nullptr;
		PPVActor = Cast<AAOSA_WorldOutliner>(UGameplayStatics::GetActorOfClass(GetWorld(), AAOSA_WorldOutliner::StaticClass()));
	}
	return PPVActor;
}

void UAOSAC_OutlineMulti::LvEnableOutline(UMeshComponent* TargetComp, bool EnableOutline, FLinearColor Color)
{
	if (!TargetComp)
	{
		return;
	}
	if (EnableOutline)
	{
		if (UPrimitiveComponent* OutlineComp{Cast<UPrimitiveComponent>(TargetComp)})
		{
			// if (UStaticMeshComponent* CurComp{Cast<UStaticMeshComponent>(TargetComp)}; CurComp)
			// {
			// 	CurComp->bDisallowNanite = true;
			// }
			OutlineComp->SetRenderCustomDepth(true);
			OutlineComp->SetCustomDepthStencilValue(1);
		}
	}
	else
	{
		if (UPrimitiveComponent* OutlineComp{Cast<UPrimitiveComponent>(TargetComp)})
		{
			// if (UStaticMeshComponent* CurComp{Cast<UStaticMeshComponent>(TargetComp)}; CurComp)
			// {
			// 	CurComp->bDisallowNanite = false;
			// }
			OutlineComp->SetRenderCustomDepth(false);
		}
	}
	
}


void UAOSAC_OutlineMulti::LvEnableOutlineOnActor(AActor* ActorToOutline, EAOSCheckOption ECheckOption,
	bool EnableOutline, bool IsToggle, FLinearColor Color)
{
	LvChangeOutlineColor(Color, false);
	Super::LvEnableOutlineOnActor(ActorToOutline, ECheckOption, EnableOutline, IsToggle, Color);
}


void UAOSAC_OutlineMulti::LvEnableOutlineOnAllActorsWithTag(FName TargetTag, bool EnableOutline, bool IsToggle,
                                                            FLinearColor Color)
{
	Super::LvEnableOutlineOnAllActorsWithTag(TargetTag, EnableOutline, IsToggle, Color);
}

void UAOSAC_OutlineMulti::LvEnableOutlineByTag(AActor* ActorToOutline, FName TargetTag, bool EnableOutline,
	bool IsToggle, FLinearColor Color)
{
	Super::LvEnableOutlineByTag(ActorToOutline, TargetTag, EnableOutline, IsToggle, Color);
}

void UAOSAC_OutlineMulti::LvAOSLineTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float LineDistance, const bool UseCameraFocus)
{
	Super::LvAOSLineTrace(TargetTag, InHighlightColor, IsDebug, IsTraceMulti, TraceTypeQuery, LineDistance, UseCameraFocus);
}

AActor* UAOSAC_OutlineMulti::LvAOSLineTrace2(FName TargetTag, const FLinearColor InHighlightColor,
	const FVector CustomStartLocation, const bool IsDebug, const ECollisionChannel CollisionChannel, float LineDistance, const bool UseCameraFocus)
{
	return Super::LvAOSLineTrace2(TargetTag, InHighlightColor, CustomStartLocation, IsDebug, CollisionChannel,
	                              LineDistance, UseCameraFocus);
}


void UAOSAC_OutlineMulti::LvAOS_SphereTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug,
                                            bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float InRadius)
{
	Super::LvAOS_SphereTrace(TargetTag, InHighlightColor, IsDebug, IsTraceMulti, TraceTypeQuery, InRadius);
}

void UAOSAC_OutlineMulti::LvDisableAllOutline()
{
	Super::LvDisableAllOutline();
}

TArray<AActor*> UAOSAC_OutlineMulti::LvGetInteractableActors(const FName TargetTag)
{
	return Super::LvGetInteractableActors(TargetTag);
}


void UAOSAC_OutlineMulti::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UAOSAC_OutlineMulti, NewOutlineColor);
	DOREPLIFETIME(UAOSAC_OutlineMulti, bUseSceneDepth);
	DOREPLIFETIME(UAOSAC_OutlineMulti, bChangeDefault);
	DOREPLIFETIME(UAOSAC_OutlineMulti, PPVActor);
}


void UAOSAC_OutlineMulti::BeginPlay()
{
	Super::BeginPlay();
	GetPPVActor();
	
	const FString ResultMsg{PPVActor ? TEXT("Valid") : TEXT("Not Valid")};
	UE_LOG(LogTemp, Log, TEXT("Owner %s.'s AOS PPV : %s."), *GetOwner()->GetName(), *ResultMsg);
	
	if (bUseAOSDebugLine)
	{
		GetWorld()->GetTimerManager().SetTimer(AosTimerHandle, FTimerDelegate::CreateLambda([this]()
		{
			LvAOSLineTrace(TargetActorTag, HighlightColor, true, false, TraceTypeQuery1,AOSDebugLineDistance, true);
		}), AOSLineTraceRate, true);
	}
}

void UAOSAC_OutlineMulti::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PPVActor != nullptr && GetOwner()->HasAuthority())
	{
		PPVActor->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}

