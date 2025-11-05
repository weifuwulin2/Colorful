// Copyright 2023 Dev Levy. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "AOSAC_OutlineBase.h"
#include "UObject/Object.h"
#include "AOSAC_OutlineOverlay.generated.h"

class AActor;
class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

struct FAOS_GlobalPreset;
struct FAOSTraceParam;

UCLASS(HideCategories=(Mobility), Blueprintable, EditInlineNew, meta=(BlueprintSpawnableComponent))
class ADVANCEDOUTLINESYSTEM_API UAOSAC_OutlineOverlay : public UAOSAC_OutlineBase
{
	GENERATED_BODY()

public:

	UAOSAC_OutlineOverlay(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, Category="AOS|InitSetting")
	TObjectPtr<UMaterialInterface> AOSMaterialInstance{nullptr};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="AOS|InitSetting")
	FLinearColor HighlightColor{FLinearColor::Yellow};
	
	
	/*Main Functions for Highlight*/

	/** Get All Actors with Outline Tag.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay")
	virtual TArray<AActor*> LvGetAllInteractableActors(FName TargetTag) override;

	/** Outline Actor by checking Tag.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay",meta=(EnableOutline="true",IsToggle = "true"))
	virtual void LvEnableOutlineByTag(AActor* ActorToOutline, FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color) override;

	/** Get All actors and Outline by checking Tag.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay",meta=(EnableOutline="true",IsToggle = "true"))
	virtual void LvEnableOutlineOnAllActorsWithTag(FName TargetTag, bool EnableOutline, bool IsToggle, FLinearColor Color) override;

	/** Outline Actor with Tag when Line Trace hit.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay", DisplayName="Lv AOS LineTrace")
	virtual void LvAOSLineTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float LineDistance, const bool UseCameraFocus) override;

	/** Can Set Start Location. Outline Actor with Tag when Line Trace hit.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay", DisplayName="Lv AOS LineTrace 2")
	virtual AActor* LvAOSLineTrace2(FName TargetTag, const FLinearColor InHighlightColor, const FVector CustomStartLocation, const bool IsDebug, const ECollisionChannel CollisionChannel, float LineDistance, const bool UseCameraFocus) override;

	/** Outline Actor with Tag when Overlapped on Sphere.*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay")
	virtual void LvAOS_SphereTrace(FName TargetTag, FLinearColor InHighlightColor, bool IsDebug, bool IsTraceMulti, ETraceTypeQuery TraceTypeQuery, float InRadius = 100.f) override;

	/** Outline Chosen Actor*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay",meta=(EnableOutline="true",IsToggle = "true"))
	virtual void LvEnableOutlineOnActor(AActor* ActorToOutline, EAOSCheckOption ECheckOption, bool EnableOutline, bool IsToggle, FLinearColor Color) override;

	/** Change Outline Color on Runtime*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay", meta=(Keywords="ChangeColor"), DisplayName="AOS Overlay_Change Color")
	virtual void LvChangeColor(FName TargetTag, FLinearColor InColor, bool ChangeDefault);

	/** Get all Actors and Disable outline*/
	UFUNCTION(BlueprintCallable, Category="AOS|Overlay", meta=(Keywords="Disable"))
	virtual void LvDisableAllOutline() override;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category="AOS|Runtime")
	FOnHighlightColorChanged OnOverlayColorChanged;

	virtual void LvChangeOutlineColor(FLinearColor InColor, bool ChangeDefault) override;
	
	virtual TArray<AActor*> LvGetInteractableActors(const FName TargetTag) override;
	virtual void LvEnableOutline(UMeshComponent* TargetComp, const bool EnableOutline, FLinearColor Color) override;

protected:
	
	UFUNCTION()
	virtual void LvCreateMID(const FLinearColor& InColor);
	virtual void LvOutlinerInitSetting() override;

	//ActorComponent Interface
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DestroyComponent(bool bPromoteChildren) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ActorComponent Interface

	UPROPERTY()
	bool LineTraceActivated{false};

	UPROPERTY()
	FLinearColor CurrentHighLightColor{FLinearColor::Black};

private:
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AOSCustomMID{nullptr};
	
	
};