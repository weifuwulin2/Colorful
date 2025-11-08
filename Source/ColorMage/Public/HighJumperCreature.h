// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CreatureCharacter.h"
#include "HighJumperCreature.generated.h"

/**
 * 
 */
UCLASS()
class COLORMAGE_API AHighJumperCreature : public ACreatureCharacter
{
	GENERATED_BODY()
public:
	AHighJumperCreature();
protected:
	// [!! 重写父类的跳跃能力 !!]
	virtual void OnJumpPressed() override;
	// [!! 可配置的跳跃力度 !!]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "High Jumper")
	float SuperJumpForce = 3000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "High Jumper")
	float NormalJumpForce = 1000.0f;
};
