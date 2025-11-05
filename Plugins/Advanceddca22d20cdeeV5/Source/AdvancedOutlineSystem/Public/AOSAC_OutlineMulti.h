// Copyright 2023 Dev Levy. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AOSAC_OutlineBase.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Engine/DataTable.h"
#include "AOSA_WorldOutliner.h"
#include "AOSAC_OutlineMulti.generated.h"

struct FAOS_GlobalPreset;
struct FAOSTraceParam;

class AActor;
class AAOSA_WorldOutliner;

UCLASS(HideCategories=(Mobility), Blueprintable, EditInlineNew, meta=(BlueprintSpawnableComponent))
class ADVANCEDOUTLINESYSTEM_API UAOSAC_OutlineMulti : public UAOSAC_OutlineBase
{
	GENERATED_BODY()

public:

	UAOSAC_OutlineMulti(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="AOS|InitSetting")
	TSubclassOf<AAOSA_WorldOutliner> AOS_PPVClass{AAOSA_WorldOutliner::StaticClass()};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="AOS|InitSetting")
	FLinearColor HighlightColor{FLinearColor::Yellow};

	/** If true, The Outline is Visible over Blocking Object*/
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere, Category="AOS|InitSetting")
	bool bUseSceneDepth{true};

	/*Main Functions for Highlight*/

	/** Get All Actors with Outline Tag.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi")
	virtual TArray<AActor*> LvGetAllInteractableActors(FName TargetTag) override;

	/** Outline Actor by checking Tag.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi",meta=(EnableOutline="true",IsToggle = "true"))
	virtual void LvEnableOutlineByTag(AActor* ActorToOutline, FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color) override;
	
	/** Get All actors and Outline by checking Tag.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi",meta=(EnableOutline="true",IsToggle = "true"))
	virtual void LvEnableOutlineOnAllActorsWithTag(FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color) override;

	/** Outline Actor with Tag when Line Trace hit.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi", DisplayName="Lv AOS LineTrace")
	virtual void LvAOSLineTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float LineDistance, const bool UseCameraFocus) override;

	/** Can Set Start Location. Outline Actor with Tag when Line Trace hit.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi", DisplayName="Lv AOS LineTrace 2")
	virtual AActor* LvAOSLineTrace2(FName TargetTag, const FLinearColor InHighlightColor, const FVector CustomStartLocation, const bool IsDebug, const ECollisionChannel CollisionChannel, float LineDistance, const bool UseCameraFocus) override;

	/** Outline Actor with Tag when Overlapped on Sphere.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi")
	virtual void LvAOS_SphereTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float InRadius = 100.f) override;

	/** Outline Chosen Actor*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi",meta=(EnableOutline="true",IsToggle = "true"))
	virtual void LvEnableOutlineOnActor(AActor* ActorToOutline, EAOSCheckOption ECheckOption, bool EnableOutline, bool IsToggle, FLinearColor Color) override;

	/** Change Outline Color on Runtime*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi", meta=(Keywords="ChangeColor"), DisplayName="AOS Multi_Change Color")
	virtual void LvChangeOutlineColor(FLinearColor InColor, bool ChangeDefault) override;

	/** Get all Actors and Disable outline*/
	UFUNCTION(BlueprintCallable, Category="AOS|Multi")
	virtual void LvDisableAllOutline() override;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="AOS|Runtime")
	FOnHighlightColorChanged OnMultiColorChanged;

	virtual void LvEnableOutline(UMeshComponent* TargetComp, bool EnableOutline, FLinearColor Color) override;
	virtual TArray<AActor*> LvGetInteractableActors(const FName TargetTag) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	
	virtual AAOSA_WorldOutliner* GetPPVActor();
	virtual void LvOutlinerInitSetting() override;
	
	UFUNCTION()
	virtual void LvSwitchColor(const FLinearColor& InColor);
	virtual void LvChangeColor_Internal(const FLinearColor InColor, const bool ChangeDefault);
	
	//ActorComponent Interface
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ActorComponent Interface
	
	UFUNCTION()
	virtual void OnRep_NewOutlineColor();
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetNewOutlineColor(const FLinearColor InColor);

private:

	UPROPERTY(Replicated)
	bool bChangeDefault{false};
	
	UPROPERTY(ReplicatedUsing = OnRep_NewOutlineColor)
	FLinearColor NewOutlineColor{FLinearColor::Black};
	
	UPROPERTY(Replicated, meta=(AllowPrivateAccess))
	TObjectPtr<AAOSA_WorldOutliner> PPVActor;
	
};
