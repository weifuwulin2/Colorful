// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Actor.h"
#include "ColorSourceActor.generated.h"

UCLASS()
class COLORMAGE_API AColorSourceActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AColorSourceActor();

	/** Gets the color this actor provides. */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const { return ColorToGive; }

protected:
	/** The color this source provides (set in the editor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	EColor ColorToGive = EColor::EC_Yellow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

};
