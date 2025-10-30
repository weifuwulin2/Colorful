// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RevealableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class URevealableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class COLORMAGE_API IRevealableInterface
{
	GENERATED_BODY()

public:
	virtual void Reveal() = 0;
	virtual void Hide() = 0;
};
