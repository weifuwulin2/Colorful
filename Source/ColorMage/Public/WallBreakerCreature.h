// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CreatureCharacter.h"
#include "WallBreakerCreature.generated.h"

/**
 * 
 */
UCLASS()
class COLORMAGE_API AWallBreakerCreature : public ACreatureCharacter
{
	GENERATED_BODY()
public:
	AWallBreakerCreature();
protected:
	// [!! 重写父类的LMB能力 !!]
	virtual void OnLMBPressed() override;
	// [!! 可配置的破墙参数 !!]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Breaker")
	float BreakWallRange = 500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Breaker")
	float BreakWallDamage = 1000.0f;
private:
	// [!! 破墙逻辑 !!]
	void PerformWallBreak();
};
