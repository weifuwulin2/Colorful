#include "VerticalMovementComponent.h"

#include "InputActionValue.h"
#include "GameFramework/Pawn.h" // Needed to get the owner and move it
#include "GameFramework/Controller.h" // Needed for GetControlRotation (though maybe not strictly necessary here)
#include "Kismet/KismetMathLibrary.h" // May be useful for rotation if needed later

void UVerticalMovementComponent::AddMovementInput(const FInputActionValue& InputValue)
{
	// Get the Pawn that owns this component
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		// Log an error if we can't get the owner
		UE_LOG(LogTemp, Error, TEXT("VerticalMovementComponent owner is not a Pawn!"));
		return;
	}

	// Get the raw 2D input vector (from WASD or joystick)
	const FVector2D MoveVector = InputValue.Get<FVector2D>();

	// --- Vertical Movement Logic ---
	// We only care about the Y-axis input (W/S or Up/Down on stick)
	if (MoveVector.Y != 0.0f)
	{
		// Define the direction of movement: World Z-axis (Up/Down)
		const FVector MoveDirection = FVector::UpVector;

		// Calculate the movement delta for this frame:
		// Direction * InputScale * Speed * FrameTime
		const FVector DeltaMovement = MoveDirection * MoveVector.Y * MoveSpeed * GetWorld()->GetDeltaSeconds();

		// Move the owning Pawn in world space.
		// Set bSweep to true so it collides with things.
		OwnerPawn->AddActorWorldOffset(DeltaMovement, true);
	}
	// X-axis input (A/D or Left/Right stick) is ignored for vertical movement.
}