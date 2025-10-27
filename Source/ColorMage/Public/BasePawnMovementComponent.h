// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BasePawnMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COLORMAGE_API UBasePawnMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	/**
	 * Base function called by the owning Pawn to provide movement input.
	 * Child classes override this to implement specific movement logic.
	 * @param InputValue The raw FVector2D input value from the Input Action.
	 */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void AddMovementInput(const FInputActionValue& InputValue);

protected:
	/** Speed at which the Pawn moves (units per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveSpeed = 500.0f;
};
