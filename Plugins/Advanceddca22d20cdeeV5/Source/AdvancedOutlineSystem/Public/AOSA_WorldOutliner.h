// Copyright 2023 Dev Levy. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "AOSA_WorldOutliner.generated.h"

class UPostProcessComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutlineColorChanged, FLinearColor, InColor);

UCLASS()
class ADVANCEDOUTLINESYSTEM_API AAOSA_WorldOutliner : public AActor
{
	GENERATED_BODY()
	
public:
	
	AAOSA_WorldOutliner(const FObjectInitializer&ObjectInitializer);

	/*Except Initial Setting Vairables,
	 *All of the Functions and Variables here are controlled by "AOSAC_OutlineMulti" ActorComponent*/
	/*You DO NOT NEED to Edit Options of this Actor TO USE AOS PLUGIN*/

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="AOS|InitSetting")
	TObjectPtr<UPostProcessComponent> AOS_PostProcess;

	//Default Material without Scene Depth
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AOS|InitSetting")
	TObjectPtr<UMaterialInterface> AOS_OutlineMaterial1;

	//Default Material With Scene Depth
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AOS|InitSetting")
	TObjectPtr<UMaterialInterface> AOS_OutlineMaterial2;
	
	UPROPERTY()
	FOnOutlineColorChanged OnOutlineColorChanged;
	
	/*Do Not Change*/
	virtual void SetUsingSceneDepth(const bool UseSceneDepth);
	virtual void UpdateMaterialColor(const FLinearColor InColor);
	virtual void UpdateHighlightColor(const FLinearColor InColor);
	

protected:

	//Default Setting to use Scene Depth or Not
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="AOS|InitSetting")
	bool bUseSceneDepth;
	
	//Default Color Setting
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="AOS|InitSetting")
	FLinearColor HighLightColor{FLinearColor::Yellow};

	UPROPERTY()
	UMaterialInstanceDynamic* AOS_OutlineMID{nullptr};

	UPROPERTY()
	UMaterialInstanceDynamic* AOS_Outline2MID{nullptr};

	UPROPERTY()
	FLinearColor CurrentHighLightColor{FLinearColor::Black};

	virtual void SetInitialSettings(const bool UseSceneDepth);

	UFUNCTION()
	virtual void SetMaterialColor(const FLinearColor InFColor);

	//AActor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	//~AActor Interface

};

