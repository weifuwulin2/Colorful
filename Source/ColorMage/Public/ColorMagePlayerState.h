// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ColorTypes.h"
#include "ColorMagePlayerState.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerColorChanged, EColor, NewColor);
UCLASS()
class COLORMAGE_API AColorMagePlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	AColorMagePlayerState();

	/**
	 * Sets the player's current color. Called by the server.
	 * @param NewColor The new color to set.
	 */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Color Magic")
	void Server_SetCurrentColor(EColor NewColor);

	/** Gets the player's current color. */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetCurrentColor() const { return CurrentColor; }

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerColorChanged OnPlayerColorChanged;
	
protected:
	/** Replication notification function for when CurrentColor changes. */
	UFUNCTION()
	void OnRep_CurrentColor();

	/** The player's currently held color. Replicated so clients can see it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentColor, Category = "Color Magic")
	EColor CurrentColor = EColor::EC_None;

	/** Required for network replication. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
