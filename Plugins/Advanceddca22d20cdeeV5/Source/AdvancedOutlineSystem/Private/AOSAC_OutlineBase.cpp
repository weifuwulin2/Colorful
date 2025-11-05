// Copyright 2023 Dev Levy. All Rights Reserved.

#include "AOSAC_OutlineBase.h"

#include "AOS_GlobalPreset.h"
#include <Camera/CameraComponent.h>
#include <Components/MeshComponent.h>
#include <DrawDebugHelpers.h>
#include <Engine/HitResult.h>
#include <TimerManager.h>
#include <EngineUtils.h>
#include <string>
#include <Components/CapsuleComponent.h>
#include <Engine/DataTable.h>
#include <Engine/EngineTypes.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>

#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Kismet/GameplayStatics.h"


UAOSAC_OutlineBase::UAOSAC_OutlineBase(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	
	bAutoActivate = true;
	
}

TArray<AActor*> UAOSAC_OutlineBase::LvGetAllInteractableActors(FName TargetTag)
{
	return LvGetInteractableActors(TargetTag);
}

TArray<AActor*> UAOSAC_OutlineBase::LvGetInteractableActors(FName TargetTag)
{
	TArray<AActor*> Arr_CurActors;
	const FName ConditionTag{!TargetTag.IsNone()?TargetTag:TargetActorTag};
	
	if (GetWorld() && GetOwner()->GetWorld())
	{
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			if (AActor* Actor{*It}; Actor->ActorHasTag(ConditionTag))
			{
				Arr_CurActors.Add(Actor);
			}
		}
	}
	return Arr_CurActors;
}


void UAOSAC_OutlineBase::LvEnableOutlineOnActor(AActor* ActorToOutline, EAOSCheckOption ECheckOption, bool EnableOutline, bool IsToggle, FLinearColor Color)
{
	if (GetWorld() && GetOwner()->GetWorld())
	{
		if (!IsValid(ActorToOutline))
		{
			return;
		}
		TArray<UMeshComponent*> Arr_MeshComps;
		ActorToOutline->GetComponents<UMeshComponent>(Arr_MeshComps);
	
		if (Arr_MeshComps.IsEmpty())
		{
			return;
		}

		//Changed
		const bool IsActivated{IsToggle ? ActorToOutline->ActorHasTag(ActiveTag):!EnableOutline};
	
		switch (AOSCheckOption)
		{
		case EAOSCheckOption::AllComps:
			Arr_MeshComps.RemoveAll([this](UMeshComponent* Elem)
			{
				return Elem->ComponentHasTag(ToIgnoreTag);
			});
			break;
		case EAOSCheckOption::AllCompsWithTag:
			for (UMeshComponent* Elem: Arr_MeshComps)
			{
				if (!Elem->ComponentHasTag(TargetComponentTag) || Elem->ComponentHasTag(ToIgnoreTag))
				{
					Arr_MeshComps.Remove(Elem);
				}
			}
			break;
		}

		for (UMeshComponent* Elem : Arr_MeshComps)
		{
			LvEnableOutline(Elem, !IsActivated, Color);
		}
	
		!IsActivated ? ActorToOutline->Tags.AddUnique(ActiveTag) : ActorToOutline->Tags.RemoveSingle(ActiveTag);
	}
	
}




void UAOSAC_OutlineBase::LvChangeOutlineColor(FLinearColor InColor, bool ChangeDefault)
{
	//In Inherited Classes.
}


void UAOSAC_OutlineBase::LvEnableOutlineByTag(AActor* ActorToOutline, FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color)
{
	if (ActorToOutline == nullptr)
	{
		return;
	}
	
	if (TargetTag.IsNone())
	{
		TargetTag = TargetActorTag;
	}

	
	if (ActorToOutline->ActorHasTag(TargetTag) && !ActorToOutline->ActorHasTag(ToIgnoreTag))
	{
		TArray<AActor*> ActorsWithTargetTag;
		ActorsWithTargetTag.AddUnique(ActorToOutline);
		
		// Optional: Update the color based on some condition (e.g., data table entry).
		FLinearColor TargetColor = Color;
		if (FLinearColor CustomColor; LvCheckTagFromDT(TargetTag, CustomColor))
		{
			TargetColor = CustomColor;
		}

		FAOSWorldOutlineInfo NewInfo{FAOSWorldOutlineInfo(EAOSCheckOption::AllComps, TargetTag, EnableOutline, IsToggle, TargetColor)};
		ToggleOutlines(ActorsWithTargetTag, NewInfo);
	}
}

void UAOSAC_OutlineBase::LvEnableOutline(UMeshComponent* TargetComp, bool EnableOutline, FLinearColor Color)
{
	//This Function is in Inherited Classes
}


void UAOSAC_OutlineBase::LvOutlinerInitSetting()
{

	UE_LOG(LogTemp, Log, TEXT("AOS Initialized"));
	
	AOSInfo = FAOSWorldOutlineInfo();
	OutlinedActorsList.Empty();
	Arr_OutlineActors.Empty();
	TargetedActorsList.Empty();
	CurrentActor = nullptr;
	CurHitActor = nullptr;
	CurrentInfo  = FAOSWorldOutlineInfo();
	bNeedDisable = false;


	if (UWorld* CurWorld = GetOwner()->GetWorld(); CurWorld && GetWorld())
	{
		CurWorld->GetTimerManager().ClearTimer(AosTimerHandle);
	}
	
	TArray<UActorComponent*> CameraComps;
	GetOwner()->GetComponents(UCameraComponent::StaticClass(), CameraComps);
	
	if (CameraComps.Num() > 0)
	{
		for (auto elem : CameraComps)
		{
			if (!elem->ComponentHasTag(ToIgnoreTag))
			{
				elem->ComponentTags.Add(ToIgnoreTag);
			}
		}
	}

}


void UAOSAC_OutlineBase::LvOutlinerDestroy()
{
	if (!GetWorld()){return;}
	
	if (UWorld* CurWorld{GetOwner()->GetWorld()})
	{
		if (CurWorld->GetTimerManager().TimerExists(AosTimerHandle))
		{
			CurWorld->GetTimerManager().ClearTimer(AosTimerHandle);
		}
	}

	UGameplayStatics::GetAllActorsWithTag(GetOwner()->GetWorld(), ActiveTag, Arr_OutlineActors);
	if (!Arr_OutlineActors.IsEmpty())
	{
		for (AActor* Elem : Arr_OutlineActors)
		{
			if (!Elem)
			{
				continue;
			}
			if (Elem->ActorHasTag(ActiveTag))
			{
				Elem->Tags.Remove(ActiveTag);
			}
		}
	}
	AOSInfo = FAOSWorldOutlineInfo();
	OutlinedActorsList.Empty();
	Arr_OutlineActors.Empty();
	TargetedActorsList.Empty();
	CurrentActor = nullptr;
	CurHitActor = nullptr;
	CurrentInfo  = FAOSWorldOutlineInfo();
	bNeedDisable = false;
}

void UAOSAC_OutlineBase::BeginPlay()
{
	Super::BeginPlay();
	LvOutlinerInitSetting();

}

void UAOSAC_OutlineBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LvOutlinerDestroy();
	Super::EndPlay(EndPlayReason);
}

void UAOSAC_OutlineBase::DestroyComponent(bool bPromoteChildren)
{

	if (UWorld* CurWorld{GetOwner()->GetWorld()}; CurWorld && GetWorld())
	{
		CurWorld->GetTimerManager().ClearTimer(AosTimerHandle);
	}
	
	Super::DestroyComponent(bPromoteChildren);
}


bool UAOSAC_OutlineBase::LvCheckTagFromDT(const FName TargetTag, FLinearColor& TargetColor)
{
	if (!OutlineGlobalPreset || !OutlineGlobalPreset->GetRowNames().Contains(TargetTag))
	{
		return false;
	}
	
	FAOS_GlobalPreset* TargetPreset{OutlineGlobalPreset->FindRow<FAOS_GlobalPreset>(TargetTag, "Find For Target Tag Color")};
	TargetColor = TargetPreset->HighlightColor;
	return true;
}

FString GetEnumText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "ERROR";
	}
}


void UAOSAC_OutlineBase::ToggleOutlines(const TArray<AActor*>& InTargetActors, FAOSWorldOutlineInfo InOutlineInfo)
{
	if (!ensure(GetOwner())){return;}


	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		Server_RequestToggleOutline(InTargetActors, InOutlineInfo);
	}
	
	if (GetOwner()->HasAuthority())
	{
		UpdateOutlineInfo(InTargetActors, InOutlineInfo);

		OnRep_IsOutlineEnabled(); // Direct call to ensure immediate effect on the server
	}
	
	// UE_LOG(LogTemp, Warning, TEXT("Is Run On : %s. : %hhd. : Local List: %i. / Rep List : %i."), *GetEnumText(GetOwnerRole()), AOSInfo.bEnableOutline, TargetedActorsList.Num(), OutlinedActorsList.Num());
}



void UAOSAC_OutlineBase::Server_RequestToggleOutline_Implementation(const TArray<AActor*>& InTargetActors,
																	FAOSWorldOutlineInfo InOutlineInfo)
{
	UpdateOutlineInfo(InTargetActors, InOutlineInfo);
	// UE_LOG(LogTemp, Warning, TEXT("Is Run On Rep Autonomous  : %s."), *GetEnumText(GetOwnerRole()));
	OnRep_IsOutlineEnabled(); // Direct call to ensure immediate effect on the server
	
}

bool UAOSAC_OutlineBase::Server_RequestToggleOutline_Validate(const TArray<AActor*>& InTargetActors, FAOSWorldOutlineInfo InOutlineInfo)
{
	return true;
}

void UAOSAC_OutlineBase::ApplyOutlineEffect(const bool bEnable)
{
	// UE_LOG(LogTemp, Log, TEXT("Is Run On : %s. : %hhd. : Local List: %i. / Rep List : %i."), *GetEnumText(GetOwnerRole()), AOSInfo.bEnableOutline, TargetedActorsList.Num(), OutlinedActorsList.Num());
	TArray<AActor*> ApplyActors{TargetedActorsList};
	
	for (AActor* Elem : ApplyActors)
	{
		CurrentActor = Elem;
		OnActiveOutlinerEvent.Broadcast(CurrentActor, CurrentInfo.CheckOption,  bEnable, CurrentInfo.bIsToggle, CurrentInfo.OutlineColor);
		
		TargetedActorsList.Remove(CurrentActor);
	}

}

void UAOSAC_OutlineBase::UpdateOutlineInfo(const TArray<AActor*>& InTargetActors, FAOSWorldOutlineInfo InOutlineInfo)
{

	AOSInfo = InOutlineInfo;
	OutlinedActorsList.Empty();
	for (AActor* a : InTargetActors)
	{
		OutlinedActorsList.Add(a);
	}
	TargetedActorsList = OutlinedActorsList;
	bIsOutlineEnabled = !bIsOutlineEnabled;

}

void UAOSAC_OutlineBase::OnRep_IsOutlineEnabled()
{
	if (GetWorld() && GetOwner()->GetWorld())
	{
		CurrentInfo = AOSInfo;
		TargetedActorsList = OutlinedActorsList;

		if (GetOwnerRole() == ROLE_AutonomousProxy)
		{
			if (CurrentInfo != AOSInfo)
			{
				CurrentInfo = AOSInfo;
			}

			if (CurrentInfo.bIsToggle)
			{
				ApplyOutlineEffect(bIsOutlineEnabled);
			}
			else
			{
				ApplyOutlineEffect(CurrentInfo.bEnableOutline);
			}
		}
	
		if (AOSInfo.bIsToggle)
		{
			ApplyOutlineEffect(bIsOutlineEnabled);
		}
		else
		{
			ApplyOutlineEffect(AOSInfo.bEnableOutline);
		}
	}
	
}




void UAOSAC_OutlineBase::LvEnableOutlineOnAllActorsWithTag(const FName TargetTag, const bool EnableOutline, const bool IsToggle, FLinearColor Color)
{
	// Attempt to retrieve all interactable actors with the specified tag.
	TArray<AActor*> ActorsWithTargetTag = LvGetInteractableActors(TargetTag);
    
	if (ActorsWithTargetTag.IsEmpty())
	{
		// UE_LOG(LogTemp, Warning, TEXT("No actors found with the specified tag."));
		return;
	}

	// Optional: Update the color based on some condition (e.g., data table entry).
	FLinearColor TargetColor = Color;
	if (FLinearColor CustomColor; LvCheckTagFromDT(TargetTag, CustomColor))
	{
		TargetColor = CustomColor;
	}

	// Create a new outline info structure to update world outline info.
	FAOSWorldOutlineInfo NewOutlineInfo(EAOSCheckOption::AllComps, TargetTag, EnableOutline, IsToggle, TargetColor);
	// Update the world outline with the new info and the list of actors to be outlined.
	ToggleOutlines(ActorsWithTargetTag, NewOutlineInfo);
}

void UAOSAC_OutlineBase::LvDisableAllOutline()
{
	if (GetWorld() && GetOwner()->GetWorld())
	{
		TArray<AActor*> ActiveActors{nullptr};
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), ActiveTag, ActiveActors);
		if (ActiveActors.IsEmpty())
		{
			return;
		}
		for (AActor* Actor : ActiveActors)
		{
			OnActiveOutlinerEvent.Broadcast(Actor, EAOSCheckOption::AllComps, false, false, FLinearColor::Black);
		}
	}
}


AActor* UAOSAC_OutlineBase::LvTraceFunction(bool InIsHit, bool InIsMulti, TArray<FHitResult> InHitResults, FHitResult InHitResult,
                                            FName TargetTag, FLinearColor InHighlightColor)
{
	auto GetTargetActors
	{
		[InHitResults](const FName CurTag, TArray<AActor*>& OutActors)
		{
			if (InHitResults.IsEmpty()) return;
			
			for (FHitResult Elem : InHitResults)
			{
				if (AActor* Actor = Elem.HitObjectHandle.FetchActor(); Actor)
				{
					if (Actor->ActorHasTag(CurTag))
					{
						OutActors.AddUnique(Actor);
					}
				}
				else
				{
					OutActors.Empty();
					break;
				}
			}
		}
	};
	

	bool NeedDisable{false};
	if (InIsHit)
	{
		TArray<AActor*> HitActors;
		if (InIsMulti)
		{
			GetTargetActors(TargetTag,HitActors);
			HitActors.Append(HitActors);
		}
		else
		{
			HitActors.AddUnique(InHitResult.HitObjectHandle.FetchActor());
		}

		if (HitActors.IsEmpty() || !HitActors.IsValidIndex(0))
		{
			return nullptr;
		}

		for ( AActor* HitActor : HitActors)
		{
			if (HitActor == nullptr || !HitActor->ActorHasTag(TargetTag) || (HitActor->ActorHasTag(ActiveTag) && CurHitActor != HitActor) || CurHitActor != nullptr && CurHitActor != HitActor)
			{
				NeedDisable = true;
			}
			else
			{
				LvEnableOutlineOnActor(HitActor, EAOSCheckOption::AllComps, true, false, InHighlightColor);
				CurHitActor = HitActor;
				return CurHitActor;
			}
		}
	}
	else
	{
		NeedDisable = true;
	}
	bNeedDisable = NeedDisable;
	
	if (bNeedDisable)
	{
		AActor* PreviousActor{CurHitActor};
		CurHitActor = nullptr;
		FTimerHandle TimerHandle;
		
		if (!GetOwner()->GetWorld() || !GetWorld()){return nullptr;}
		GetOwner()->GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, PreviousActor, TargetTag]()->void
		{
			if (!IsValid(PreviousActor))
			{
				bNeedDisable = false;
				return;
			}
			LvEnableOutlineOnActor(PreviousActor, EAOSCheckOption::AllComps, false, false, FLinearColor::Black);
		}, 0.2f, false);
		bNeedDisable = false;
	}
	return nullptr;
}


void UAOSAC_OutlineBase::LvAOSLineTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float LineDistance, const bool UseCameraFocus)
{
	if (GetOwner() == nullptr || !GetWorld() || !GetOwner()->GetWorld())
	{
		return;
	}

	APawn* CurOwner = Cast<APawn>(GetOwner());

	if (!IsValid(CurOwner) || !CurOwner->IsPlayerControlled())
	{
		return;
	}
	
	TArray<FHitResult> OutHitResults;
	FHitResult OutHitResult;
	bool bHit{false};

	float InMaxDist{ LineDistance <= 0 ? AOSDebugLineDistance : LineDistance };
	FName CurTargetTag{TargetTag.IsNone() ? TargetActorTag : TargetTag};
	
	FVector StartLoc{CurOwner->GetTransform().GetLocation()};
	FVector EndLoc{StartLoc};

	TArray<AActor*> IgnoreActor;
	IgnoreActor.AddUnique(GetOwner());
	
	/* If view is TP = From Actor loc !!!!*/
	
	FAOSTraceParam NewTraceParam{TraceTypeQuery, ECC_Camera, true, IgnoreActor, true, EDrawDebugTrace::ForDuration};
	if (UseCameraFocus)
	{
		LvLineTraceAim(CurOwner, StartLoc,  OUT EndLoc, InMaxDist, NewTraceParam);
	}
	else
	{
		EndLoc = StartLoc+(CurOwner->GetActorForwardVector() * (InMaxDist));
	}
	
	
	bHit = IsTraceMulti
	?
		UKismetSystemLibrary::LineTraceMulti(CurOwner->GetWorld(), StartLoc, EndLoc, TraceTypeQuery,true, IgnoreActor, IsDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, OutHitResults,true, FLinearColor::Red, FLinearColor::Green, 0.2)
	:
	UKismetSystemLibrary::LineTraceSingle(CurOwner->GetWorld(), StartLoc, EndLoc, TraceTypeQuery, true, IgnoreActor, IsDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, OutHitResult,true, FLinearColor::Red, FLinearColor::Green, 0.2);
	
	LvTraceFunction(bHit, IsTraceMulti, OutHitResults, OutHitResult, CurTargetTag, InHighlightColor);
}


AActor* UAOSAC_OutlineBase::LvAOSLineTrace2(FName TargetTag, const FLinearColor InHighlightColor, FVector CustomStartLocation, const bool IsDebug, const ECollisionChannel CollisionChannel, float LineDistance, const bool UseCameraFocus)
{
	if (GetOwner() == nullptr || !GetWorld() || !GetOwner()->GetWorld())
	{
		return nullptr;
	}

	APawn* CurOwner = Cast<APawn>(GetOwner());
	if (!IsValid(CurOwner) || !CurOwner->IsPlayerControlled())
	{
		return nullptr;
	}
	
	TArray<FHitResult> OutHitResults;
	FHitResult OutHitResult;
	bool bHit{false};

	float InMaxDist{ LineDistance <= 0 ? AOSDebugLineDistance : LineDistance };
	TargetTag = TargetTag.IsNone() ? TargetActorTag : TargetTag;
	
	FVector StartLoc{CustomStartLocation};
	FVector EndLoc{StartLoc};
	
	TArray<AActor*> IgnoreActor;
	IgnoreActor.AddUnique(CurOwner);
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CurOwner);
	Params.bTraceComplex = true;
	


	FAOSTraceParam NewTraceParam{TraceTypeQuery1, ECC_Camera, true, IgnoreActor, false, EDrawDebugTrace::ForDuration};
	if (UseCameraFocus)
	{
		LvLineTraceAim(CurOwner, StartLoc, OUT EndLoc, InMaxDist, NewTraceParam);
	}
	else
	{
		EndLoc = StartLoc+(CurOwner->GetActorForwardVector() * (InMaxDist));
	}

	NewTraceParam.TargetCollisionChannel = CollisionChannel;
	AosLineTrace(OutHitResult, CurOwner->GetWorld(), StartLoc, EndLoc, Params,NewTraceParam);

#if ENABLE_DRAW_DEBUG
	if (IsDebug)
	{
		FColor DebugColor{OutHitResult.bBlockingHit ? FColor::Green : FColor::Red};
		if (OutHitResult.bBlockingHit)
		{
			DrawDebugLine(CurOwner->GetWorld(), StartLoc, OutHitResult.Location, DebugColor, false, AOSLineTraceRate);
			DrawDebugSphere(CurOwner->GetWorld(), OutHitResult.Location, 2, 16, DebugColor, false, AOSLineTraceRate);
		}
		else
		{
			DrawDebugLine(CurOwner->GetWorld(), StartLoc, EndLoc, DebugColor, false, AOSLineTraceRate);
		}
	}
#endif // ENABLE_DRAW_DEBUG

	if (AActor* TargetedActor = LvTraceFunction(OutHitResult.bBlockingHit, false, OutHitResults, OutHitResult, TargetTag, InHighlightColor); TargetedActor != nullptr)
	{
		return TargetedActor;
	}
	return nullptr;
}


void UAOSAC_OutlineBase::LvAOS_SphereTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug,
                                           bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float InRadius)
{
	if (GetOwner() == nullptr || !GetWorld() || !GetOwner()->GetWorld())
	{
		return;
	}
	TArray<FHitResult> OutHitResults;
	FHitResult OutHitResult;
	bool bHit{false};
	
	TargetTag = TargetTag.IsNone() ? TargetActorTag: TargetTag;
	
	FVector StartLoc{FVector::ZeroVector};
	FVector EndLoc{FVector::ZeroVector};

	TArray<AActor*> IgnoreActor;
	IgnoreActor.AddUnique(GetOwner());
	
	
	if (GetOwner()->GetComponentByClass(UCapsuleComponent::StaticClass()))
	{
		StartLoc = GetOwner()->GetActorLocation()+FVector(0.0f, 0.0f, GetOwner()->GetSimpleCollisionHalfHeight()/4);
	}
	else
	{
		StartLoc = GetOwner()->GetActorLocation()*FVector(1.0f, 1.0f, 1.1f);
	}
	
	EndLoc = StartLoc+(GetOwner()->GetActorForwardVector());
	
	bHit = IsTraceMulti ? UKismetSystemLibrary::SphereTraceMulti(GetWorld(), StartLoc, EndLoc, InRadius > 0.f ? InRadius : 100.0f,TraceTypeQuery, true, IgnoreActor, IsDebug ? EDrawDebugTrace::Persistent : EDrawDebugTrace::None, OutHitResults,true) : UKismetSystemLibrary::SphereTraceSingle(GetWorld(), StartLoc, EndLoc, InRadius > 0.f ? InRadius : 100.0f, TraceTypeQuery, true, IgnoreActor, IsDebug ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None, OutHitResult,true);
	
	LvTraceFunction(bHit, IsTraceMulti, OutHitResults, OutHitResult, TargetTag, InHighlightColor);
}


void UAOSAC_OutlineBase::LvLineTraceAim(AActor* InSourceActor, FVector& InTraceStart, FVector& OutTraceEnd,
	float InMaxDistance, const FAOSTraceParam InQuery)
{
	if (!InSourceActor)
	{
		return;
	}
	
	APawn* CurActor = Cast<APawn>(InSourceActor);
	if (!CurActor->IsPlayerControlled())
	{
		return;
	}
	
	APlayerController* CurController{Cast<APlayerController>(CurActor->GetController())};
	if (!CurController)
	{
		return;
	}
	
	FVector ViewStart;
	FRotator ViewRot;
	CurController->GetPlayerViewPoint(ViewStart, ViewRot);

	const FVector ViewDir{ViewRot.Vector()};
	FVector ViewEnd{ViewStart + (ViewDir * InMaxDistance)};

	ClipCameraRayToAbilityRange(ViewStart, ViewDir, InTraceStart, InMaxDistance, ViewEnd);

	FHitResult HitResult;
	constexpr bool bTraceComplex{false};
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(CurActor);
	Params.bTraceComplex = bTraceComplex;

	AosLineTrace(HitResult, CurActor->GetWorld(), ViewStart, ViewEnd, Params, InQuery);

	const bool bUseTraceResult{HitResult.bBlockingHit && (FVector::DistSquared(InTraceStart, HitResult.Location) <= (powf(InMaxDistance, 2)))};
	const FVector AdjustedEnd{(bUseTraceResult) ? HitResult.Location : ViewEnd};

	FVector AdjustedAimDir{(AdjustedEnd - InTraceStart).GetSafeNormal()};
	if (AdjustedAimDir.IsZero())
	{
		AdjustedAimDir = ViewDir;
	}

	if (!bTraceAffectsAimPitch && bUseTraceResult)
	{
		if (FVector OriginalAimDir{(ViewEnd - InTraceStart).GetSafeNormal()}; !OriginalAimDir.IsZero())
		{
			// Convert to angles and use original pitch
			const FRotator OriginalAimRot{OriginalAimDir.Rotation()};

			FRotator AdjustedAimRot{AdjustedAimDir.Rotation()};
			AdjustedAimRot.Pitch = OriginalAimRot.Pitch;

			AdjustedAimDir = AdjustedAimRot.Vector();
		}
	}

	OutTraceEnd = InTraceStart + (AdjustedAimDir * InMaxDistance);
}

void UAOSAC_OutlineBase::AosLineTrace(FHitResult& OutHitResult, const UWorld* InWorld,
	const FVector& InStart, const FVector& InEnd, const FCollisionQueryParams& InParams, FAOSTraceParam InQueryType)
{
	check(InWorld);

	OutHitResult = FHitResult();
	TArray<FHitResult> HitResults;
	const bool bHit{InWorld->LineTraceMultiByChannel(HitResults, InStart, InEnd, InQueryType.TargetCollisionChannel, InParams)};
	
	OutHitResult.TraceStart = InStart;
	OutHitResult.TraceEnd = InEnd;

	if (HitResults.Num() > 0)
	{
		OutHitResult = HitResults[0];
	}
}

bool UAOSAC_OutlineBase::ClipCameraRayToAbilityRange(const FVector& InCameraLocation, const FVector& InCameraDirection,
                                                     const FVector& InAbilityCenter, float InAbilityRange, FVector& OutClippedPosition)
{
	const FVector CameraToCenter{InAbilityCenter - InCameraLocation};

	if (const float DotToCenter{(float)FVector::DotProduct(CameraToCenter, InCameraDirection)}; DotToCenter >= 0)//If this fails, we're pointed away from the center, but we might be inside the sphere and able to find a good exit point.
	{
		const float DistanceSquared{(float)(CameraToCenter.SizeSquared() - (powf(DotToCenter, 2)))};
		if (const float RadiusSquared{(powf(InAbilityRange, 2))}; DistanceSquared <= RadiusSquared)
		{
			const float DistanceFromCamera{FMath::Sqrt(RadiusSquared - DistanceSquared)};
			const float DistanceAlongRay{DotToCenter + DistanceFromCamera};						//Subtracting instead of adding will get the other intersection point
			OutClippedPosition = InCameraLocation + (DistanceAlongRay * InCameraDirection);		//Cam aim point clipped to range sphere
			return true;
		}
	}
	
	return false;
}



void UAOSAC_OutlineBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UAOSAC_OutlineBase, ActiveTag);
	DOREPLIFETIME(UAOSAC_OutlineBase, OutlinedActorsList);
	DOREPLIFETIME(UAOSAC_OutlineBase, AOSInfo);
	DOREPLIFETIME(UAOSAC_OutlineBase, bIsOutlineEnabled);
	DOREPLIFETIME(UAOSAC_OutlineBase, OutlineGlobalPreset);
	DOREPLIFETIME(UAOSAC_OutlineBase, bNeedDisable);
}


