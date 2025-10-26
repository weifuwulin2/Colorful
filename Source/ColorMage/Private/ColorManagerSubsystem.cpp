// ColorManagerSubsystem.cpp
#include "ColorManagerSubsystem.h"
#include "ColorMagePlayerState.h"
#include "ColorSourceActor.h"
#include "PossessablePawn.h"
#include "GameFramework/PlayerController.h"

// This is the main "router" function
void UColorManagerSubsystem::HandlePlayerInteraction(APlayerController* Player, AActor* HitActor)
{
	if (!Player || !HitActor) return;

	// Case 1: Hit a Color Source?
	if (AColorSourceActor* ColorSource = Cast<AColorSourceActor>(HitActor))
	{
		ExtractColor(Player, ColorSource);
		return;
	}

	// Case 2: Hit a Possessable Pawn?
	if (APossessablePawn* PossessablePawn = Cast<APossessablePawn>(HitActor))
	{
		AttemptPossession(Player, PossessablePawn);
		return;
	}
}

// Logic for getting color
void UColorManagerSubsystem::ExtractColor(APlayerController* Player, AColorSourceActor* ColorSource)
{
	AColorMagePlayerState* PlayerState = Player->GetPlayerState<AColorMagePlayerState>();
	if (PlayerState && ColorSource)
	{
		// Tell the server to update the PlayerState's color
		PlayerState->Server_SetCurrentColor(ColorSource->GetColor());
		// (Play success sound/VFX)
	}
}

// *** MODIFIED LOGIC ***
// Logic for possessing
void UColorManagerSubsystem::AttemptPossession(APlayerController* Player, APossessablePawn* TargetPawn)
{
	AColorMagePlayerState* PlayerState = Player->GetPlayerState<AColorMagePlayerState>();
	if (!PlayerState || !TargetPawn) return;

	EColor PlayerColor = PlayerState->GetCurrentColor();
	EColor TargetColor = TargetPawn->GetColor();

	// NEW RULE: Possession ONLY works if the player has a color AND
	// that color EXACTLY matches the target's color.
	if (PlayerColor != EColor::EC_None && PlayerColor == TargetColor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ColorManager: Match success. Possessing."));
		// (Play possess VFX/SFX)
		Player->UnPossess();
		Player->Possess(TargetPawn);
	}
	else
	{
		// Player is wrong color, or target is gray, or player is gray.
		UE_LOG(LogTemp, Warning, TEXT("ColorManager: Possession failed. Colors do not match."));
		// (Play "fail" sound)
	}
}