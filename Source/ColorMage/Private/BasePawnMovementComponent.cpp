#include "BasePawnMovementComponent.h"

// The base class implementation is empty because the specific
// movement logic (Horizontal, Vertical) is handled by child classes.
void UBasePawnMovementComponent::AddMovementInput(const FInputActionValue& InputValue)
{
	// Child classes like UHorizontalMovementComponent override this function.
}