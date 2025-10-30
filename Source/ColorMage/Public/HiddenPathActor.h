// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorReactorActor.h"
#include "GameFramework/Actor.h"
#include "HiddenPathActor.generated.h"

class UBoxComponent;
UCLASS()
class COLORMAGE_API AHiddenPathActor : public AColorReactorActor,public IRevealableInterface
{
	GENERATED_BODY()
	
public:	
	AHiddenPathActor();

	// [!! 重写纯虚函数 !!]
	virtual void Reveal() override;
	virtual void Hide() override;

protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> DetectionVolume;
	

};
