// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ColorManagerSubsystem.generated.h"

// Forward declarations to avoid circular dependencies
class APlayerController;
class AActor;
class AColorSourceActor;
class APossessablePawn;
/**
 * 
 */
UCLASS()
class COLORMAGE_API UColorManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/**
	 * Main entry point for all player interactions (called by the Controller).
	 * @param Player The PlayerController triggering the interaction.
	 * @param HitActor The Actor that was hit by the player's line trace.
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void HandlePlayerInteraction(APlayerController* Player, AActor* HitActor);

	/**
	 * Logic for extracting color from a source.
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void ExtractColor(APlayerController* Player, AColorSourceActor* ColorSource);

	/**
	 * Logic for attempting to possess an object.
	 * This will ONLY succeed if the player's color matches the target's color.
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void AttemptPossession(APlayerController* Player, APossessablePawn* TargetPawn);
};
