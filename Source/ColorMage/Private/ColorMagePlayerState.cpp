// ColorMagePlayerState.cpp
#include "ColorMagePlayerState.h"
#include "Net/UnrealNetwork.h" // Needed for DOREPLIFETIME

AColorMagePlayerState::AColorMagePlayerState()
{
	CurrentColor = EColor::EC_None;
}

void AColorMagePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate the CurrentColor variable
	DOREPLIFETIME(AColorMagePlayerState, CurrentColor);
}

// This function executes on the server
void AColorMagePlayerState::Server_SetCurrentColor_Implementation(EColor NewColor)
{
	if (CurrentColor != NewColor)
	{
		CurrentColor = NewColor;
		// Manually call the OnRep function on the server
		OnRep_CurrentColor();
	}
}

// This function executes on clients when the server updates CurrentColor
void AColorMagePlayerState::OnRep_CurrentColor()
{
	// You can broadcast an event here for UI widgets to update
	OnPlayerColorChanged.Broadcast(CurrentColor);
	UE_LOG(LogTemp, Warning, TEXT("PlayerState Color updated to: %d"), (int32)CurrentColor);
}