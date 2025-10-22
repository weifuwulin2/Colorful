// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
// Enhanced Input includes
#include "InputActionValue.h"
#include "ColorMageController.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * AColorMageController is a custom PlayerController designed to handle
 * player input using the Enhanced Input System, specifically utilizing a
 * single 2D vector input for WASD movement.
 */
UCLASS()
class COLORMAGE_API AColorMageController : public APlayerController
{
	GENERATED_BODY()

public:
	AColorMageController();

protected:
	// --- Enhanced Input Properties (Set in Blueprint) ---

	/** Input Mapping Context used for movement and abilities. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Input Action for 2D (WASD) movement. Value Type must be Vector 2D. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enhanced Input")
	TObjectPtr<UInputAction> MoveAction; // Unified 2D movement action

	// --- Overrides ---

	/** Called when the game starts or when spawned. */
	virtual void BeginPlay() override;

	/** Called to bind functionality to input. */
	virtual void SetupInputComponent() override;

	// --- Input Handlers ---

	// DEBUG: Function to test if the key press is detected at all (using Started event)
	void MoveStartTest(const FInputActionValue& Value);

	/** Handler for the unified 2D Move input action (Reads Vector2D). */
	void Move(const FInputActionValue& Value);
};
