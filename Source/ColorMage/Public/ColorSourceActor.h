// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Actor.h"
#include "ColorSourceActor.generated.h"
class UColorComponent;
class UNiagaraComponent;

UCLASS()
class COLORMAGE_API AColorSourceActor : public AActor
{
	GENERATED_BODY()
    
public: 
	AColorSourceActor();

	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColorToProvide() const { return ColorToProvide; }

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override; // <--- [!! ADDED !!]

	protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// --- [!! ADDED !!] ---
	/** The Niagara particle system component for the effect */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	/** An extra multiplier to adjust the effect's scale relative to the mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float EffectScaleMultiplier = 1.0f;
	// --- [!! ADDED END !!] ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	EColor ColorToProvide = EColor::EC_Red;
};
