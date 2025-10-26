// PossessablePawn.cpp
#include "PossessablePawn.h"

APossessablePawn::APossessablePawn()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
}

void APossessablePawn::SetColor(EColor NewColor)
{
	if (CurrentColor != NewColor)
	{
		CurrentColor = NewColor;
		// Call the Blueprint event to update the material
		OnColorChanged(NewColor);
	}
}

// This is where you would bind movement for the platform
void APossessablePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	// Example: You would need to get the MoveAction from the Controller
	// and bind it to a local "MovePlatform" function.
	// This ensures WASD works for the platform too.
}