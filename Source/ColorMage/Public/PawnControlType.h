// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class EPawnControlType : uint8
{
	Unknown			UMETA(DisplayName = "Unknown"),
	Character       UMETA(DisplayName = "Character"),
	Platform        UMETA(DisplayName = "Platform"), // General platform type
	Creature      UMETA(DisplayName = "Creature")
};