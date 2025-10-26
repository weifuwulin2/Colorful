// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Pawn.h"
#include "PossessablePawn.generated.h"

UCLASS()
class COLORMAGE_API APossessablePawn : public APawn
{
	GENERATED_BODY()

public:
	APossessablePawn();

	/** Gets the current color of this pawn. */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const { return CurrentColor; }

	/** Sets the color of this pawn. */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

protected:
	/** The pawn's current color. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Color Magic")
	EColor CurrentColor = EColor::EC_None;

	/** Blueprint event called when SetColor is. Use this to change the material. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Color Magic")
	void OnColorChanged(EColor NewColor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** * Binds inputs for this pawn (e.g., Move). 
	 * Note: You'll need to re-bind "MoveAction" here if you want platforms to be movable.
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
