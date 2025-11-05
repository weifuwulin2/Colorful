// Copyright 2023 Dev Levy. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include <Engine/HitResult.h>

#include "AOS_GlobalPreset.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"


#include "AOSAC_OutlineBase.generated.h"

class APostProcessVolume;
class UDataTable;
class USphereComponent;
class AAOSA_WorldOutliner;
class UMeshComponent;
class AActor;
class UObject;
class UMaterialInstance;

enum class EAOSCheckOption:uint8;


struct FCollisionQueryParams;
struct FAOS_GlobalPreset;
struct FAOSTraceParam;
struct FAOSWorldOutlineInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnOutlineEvent, AActor*, ActorToOutline, EAOSCheckOption, ECheckOption, bool, EnableOutline, bool, IsToggle, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnOutlineByTagEvent, AActor*, ActorToOutline, FName, Target, bool, EnableOutline, bool, IsToggle, FLinearColor, Color);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHighlightColorChanged, const FLinearColor&, InColor);

UCLASS(EditInlineNew, abstract)
class ADVANCEDOUTLINESYSTEM_API UAOSAC_OutlineBase : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UAOSAC_OutlineBase(const FObjectInitializer& ObjectInitializer);
	
	
	/** Optional : Give This Tag to the Component you want to Outline*/
	UPROPERTY(BlueprintReadOnly,EditDefaultsOnly,Category="AOS|InitSetting")
	FName TargetComponentTag{"AOS"};

	/**Give This Tag to the Actor you want to Outline*/
	UPROPERTY(BlueprintReadOnly,EditDefaultsOnly,Category="AOS|InitSetting")
	FName TargetActorTag{"AOS"};

	/**Optional : Give This Tag to the Components or Actors to ignore when outline*/
	UPROPERTY(BlueprintReadOnly,EditDefaultsOnly,Category="AOS|InitSetting", DisplayName="Ignore Tag")
	FName ToIgnoreTag{"IgnoreAOS"};

	/**Use Quick Debugging with Line Trace*/
	UPROPERTY(EditDefaultsOnly,Category="AOS|InitSetting")
	bool bUseAOSDebugLine{false};

	/**Set Line Trace Distance From Character*/
	UPROPERTY(EditDefaultsOnly,Category="AOS|InitSetting")
	float AOSDebugLineDistance{300.f};

	/**Sets LineTrace Rate In Quick Debugging*/
	UPROPERTY(EditDefaultsOnly,Category="AOS|InitSetting", DisplayName="Line Trace Rate")
	float AOSLineTraceRate{0.1f};

	/** Choose wheather to Check Tag of Actor or Each Components of Actor*/
	UPROPERTY(EditDefaultsOnly, Category="AOS|InitSetting")
	EAOSCheckOption AOSCheckOption{EAOSCheckOption::AllComps};

	/** Optional DataTable Preset for Tags & Color */
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category="AOS|InitSetting")
	TObjectPtr<UDataTable> OutlineGlobalPreset;

	
	/*Delegate*/
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="AOS|Runtime")
	FOnOutlineEvent OnActiveOutlinerEvent;
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="AOS|Runtime")
	FOnOutlineByTagEvent OnOutlineByTagEvent;
	
	
	/*Main Functions for Highlight*/
	virtual TArray<AActor*> LvGetAllInteractableActors(FName TargetTag);
	
	virtual void LvEnableOutlineByTag(AActor* ActorToOutline, FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color);
	virtual void LvEnableOutlineOnActor(AActor* ActorToOutline, EAOSCheckOption ECheckOption, bool EnableOutline, bool IsToggle, FLinearColor Color);
	
	virtual void LvChangeOutlineColor(FLinearColor InColor, bool ChangeDefault);
	virtual void LvEnableOutlineOnAllActorsWithTag(FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color);

	virtual void LvAOSLineTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float LineDistance = 300.f, const bool UseCameraFocus = false);
	virtual AActor* LvAOSLineTrace2(FName TargetTag, const FLinearColor InHighlightColor, FVector CustomStartLocation, const bool IsDebug, const ECollisionChannel CollisionChannel = ECC_Visibility, float LineDistance = 300.f, const bool UseCameraFocus = true);
	
	virtual void LvAOS_SphereTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float InRadius = 100.f);
	virtual void LvDisableAllOutline();

	UFUNCTION()
	virtual AActor* LvTraceFunction(bool InIsHit, bool InIsMulti, TArray<FHitResult> InHitResults, FHitResult InHitResult, FName TargetTag, FLinearColor InHighlightColor);
	virtual void LvLineTraceAim(AActor* InSourceActor, FVector& InTraceStart, FVector& OutTraceEnd, float InMaxDistance, const FAOSTraceParam InQuery);
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	
protected:
	
	virtual TArray<AActor*> LvGetInteractableActors(FName TargetTag);
	virtual void LvEnableOutline(UMeshComponent* TargetComp, bool EnableOutline, FLinearColor Color);
	virtual bool LvCheckTagFromDT(const FName TargetTag, FLinearColor& TargetColor);
	
	//This Function will be Edited
	virtual void LvOutlinerInitSetting();
	virtual void LvOutlinerDestroy();
	

	static void AosLineTrace(FHitResult& OutHitResult, const UWorld* InWorld, const FVector& InStart, const FVector& InEnd, const FCollisionQueryParams& InParams, FAOSTraceParam InQueryType);
	static bool ClipCameraRayToAbilityRange(const FVector& InCameraLocation, const FVector& InCameraDirection, const FVector& InAbilityCenter, float InAbilityRange, FVector& OutClippedPosition);

	//ActorComponent Interface~
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DestroyComponent(bool bPromoteChildren) override;
	//~ActorComponent Interface
	
	// Does the trace affect the aiming pitch
	bool bTraceAffectsAimPitch{true};
	
	FTimerHandle AosTimerHandle;
	

	//Main Function for Replication
	UFUNCTION()
	virtual void UpdateOutlineInfo(const TArray<AActor*>& InTargetActors, FAOSWorldOutlineInfo InOutlineInfo);
	UFUNCTION()
	void ToggleOutlines(const TArray<AActor*>& InTargetActors, FAOSWorldOutlineInfo InOutlineInfo);
	// Function to actually apply the outline effect
	void ApplyOutlineEffect(const bool bEnable);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestToggleOutline(const TArray<AActor*>& InTargetActors, FAOSWorldOutlineInfo InOutlineInfo);
	// Replicated function to ensure outline state is the same across clients
	UFUNCTION()
	void OnRep_IsOutlineEnabled();


	
private:
	
	//Replicated Variables~
	// Replicated property to track the state of the outline
	UPROPERTY(ReplicatedUsing = OnRep_IsOutlineEnabled)
	bool bIsOutlineEnabled;
	UPROPERTY(Replicated)
	FAOSWorldOutlineInfo AOSInfo;
	UPROPERTY(Replicated)
	TArray<AActor*> OutlinedActorsList;
	UPROPERTY(Replicated)
	FName ActiveTag{FName("Active")};
	//~Replicated Variables
	
	UPROPERTY(meta=(AllowPrivateAccess))
	TArray<AActor*> Arr_OutlineActors;
	
	UPROPERTY(meta=(AllowPrivateAccess))
	TObjectPtr<AActor> CurHitActor{nullptr};

	
	//Variables below can be changed for refactoring.
	UPROPERTY()
	AActor* CurrentActor;
	UPROPERTY()
	TArray<AActor*> TargetedActorsList;
	FAOSWorldOutlineInfo CurrentInfo;
	UPROPERTY(Replicated)
	bool bNeedDisable{false};

	
	
};



