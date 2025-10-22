// Fill out your copyright notice in the Description page of Project Settings.

#include "ColorMageController.h"
#include "GameFramework/Pawn.h" // For GetPawn() and AddMovementInput
// Enhanced Input includes
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

AColorMageController::AColorMageController()
{
	// Set the tick function to not execute every frame
	PrimaryActorTick.bCanEverTick = false;
}

void AColorMageController::BeginPlay()
{
	Super::BeginPlay();

	// Get the Enhanced Input Subsystem for the local player
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// Add the Default Mapping Context if it is valid
		if (DefaultMappingContext)
		{
			// Add the IMC to the player's Input System at priority 0
			Subsystem->AddMappingContext(DefaultMappingContext, 10); 
            // DEBUG LOG: Confirm the Mapping Context was added
            UE_LOG(LogTemp, Log, TEXT("ColorMageController: Successfully added Default Mapping Context."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ColorMageController: DefaultMappingContext not set! Please assign it in the Blueprint derived from this controller."));
		}
	}
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ColorMageController: Enhanced Input Subsystem is NULL in BeginPlay. Input will not work."));
    }
}

void AColorMageController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// CRITICAL CHECK: Ensure the base InputComponent exists before we proceed.
	if (!InputComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ColorMageController: Base InputComponent is NULL. Cannot set up input bindings."));
		return;
	}

	// Cast the standard InputComponent to the Enhanced version
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// 1. Bind the unified 2D Move Input Action
		if (MoveAction)
		{
			// Bind the action to the Move handler function, triggered when the input is active (for continuous movement)
			EnhancedInputComponent->BindAction(
				MoveAction, 
				ETriggerEvent::Triggered, 
				this, 
				&AColorMageController::Move
			);
            
            // ** DEBUG BINDING: BIND TO STARTED **
            // This fires on the first frame a key is pressed. If this log appears, the issue is purely the Trigger in the asset.
            EnhancedInputComponent->BindAction(
                MoveAction,
                ETriggerEvent::Started,
                this,
                &AColorMageController::MoveStartTest
            );
            // DEBUG LOG: Confirm the action binding was attempted
            UE_LOG(LogTemp, Log, TEXT("ColorMageController: MoveAction successfully bound to Move function."));
		}
		
		// Log warnings if the action is not set
		if (!MoveAction)
		{
			UE_LOG(LogTemp, Warning, TEXT("ColorMageController: MoveAction Input Action is not set! Please assign it in the Blueprint."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ColorMageController: Enhanced Input Component cast failed! Check if Enhanced Input is enabled in Project Settings."));
	}
}

// --------------------------------------------------------------------------------------
// DEBUG INPUT HANDLER (Called on ETriggerEvent::Started)
// --------------------------------------------------------------------------------------
void AColorMageController::MoveStartTest(const FInputActionValue& Value)
{
    // This log should fire ONCE per key press/hold.
    UE_LOG(LogTemp, Warning, TEXT("--- KEY PRESS DETECTED! Input Action IS reaching the binding! ---"));
}

/**
 * Handles unified 2D movement input from a single FInputActionValue (Vector2D).
 * The Vector2D should be configured as: X=Forward/Backward, Y=Right/Left.
 */
void AColorMageController::Move(const FInputActionValue& Value)
{
	// --- DEBUG LOG: Check if this function is reached ---
	UE_LOG(LogTemp, Log, TEXT("Move function triggered! Movement Vector: %s"), *Value.Get<FVector2D>().ToString());
	// --- DEBUG LOG END ---

	// Extract the 2D vector value from the action
	const FVector2D MovementVector = Value.Get<FVector2D>();

	// Get the controlled Pawn
	APawn* ControlledPawn = GetPawn();

    // Secondary Debug Log for Pawn
    if (!ControlledPawn)
    {
        // This log only prints if the Move function fires but GetPawn() fails.
        UE_LOG(LogTemp, Warning, TEXT("Move function fired, but GetPawn() is NULL. Controller is not possessing a Pawn."));
    }

	// Check if input is non-zero and we have a pawn to move
	if ((!FMath::IsNearlyZero(MovementVector.X) || !FMath::IsNearlyZero(MovementVector.Y)) && ControlledPawn)
	{
		// Get the current control rotation 
		const FRotator Rotation = GetControlRotation();
		
		// Only use Yaw for directional movement (prevents tilting up/down based on camera pitch)
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// Get the Forward vector (X-axis) and Right vector (Y-axis) relative to the control rotation
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Apply forward/backward movement (MovementVector.X)
		if (!FMath::IsNearlyZero(MovementVector.X))
		{
			ControlledPawn->AddMovementInput(ForwardDirection, MovementVector.X);
		}

		// Apply right/left movement (MovementVector.Y)
		if (!FMath::IsNearlyZero(MovementVector.Y))
		{
			ControlledPawn->AddMovementInput(RightDirection, MovementVector.Y);
		}
	}
}
