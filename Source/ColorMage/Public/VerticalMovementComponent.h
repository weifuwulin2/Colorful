// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BasePawnMovementComponent.h"
#include "VerticalMovementComponent.generated.h"

struct FInputActionValue;
/**
 * 
 */
UCLASS()
class COLORMAGE_API UVerticalMovementComponent : public UBasePawnMovementComponent
{
	GENERATED_BODY()

	virtual void AddMovementInput(const FInputActionValue& InputValue) override;
};
