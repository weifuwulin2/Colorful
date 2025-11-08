// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
// PawnControlType.h
UENUM(BlueprintType)
enum class EPawnControlType : uint8
{
	Unknown         UMETA(DisplayName = "Unknown"),
	Player          UMETA(DisplayName = "Player"),
	HighJumper      UMETA(DisplayName = "High Jumper"),     // [!! 新增 !!]
	WallBreaker     UMETA(DisplayName = "Wall Breaker"),   // [!! 新增 !!]
	Character     UMETA(DisplayName = "Character"), 
};


UENUM(BlueprintType)
enum class ECreatureState : uint8
{
	/** 默认状态，巡逻、攻击玩家 */
	Hostile		UMETA(DisplayName = "Hostile"),
	
	/** 所有部位颜色已统一，变为中立，等待被附身 */
	Unified		UMETA(DisplayName = "Unified / Tamed")
};