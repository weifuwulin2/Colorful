// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RevealableInterface.h"
#include "GameFramework/Actor.h"
#include "ColorReactorActor.generated.h"

UCLASS()
class COLORMAGE_API AColorReactorActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AColorReactorActor();

protected:
	/** 作为 Actor 根基的场景组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

};
