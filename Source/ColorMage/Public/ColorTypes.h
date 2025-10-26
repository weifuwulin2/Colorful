// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Global definition for all color types in the game.
 */
UENUM(BlueprintType)
enum class EColor : uint8
{
	EC_None		UMETA(DisplayName = "None"),   // Represents Gray / Uncolored
	EC_Red		UMETA(DisplayName = "Red"),
	EC_Yellow	UMETA(DisplayName = "Yellow"),
	EC_Blue		UMETA(DisplayName = "Blue")
	// You can add more colors here
};