// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Global definition for all color types in the game.
 */
UENUM(BlueprintType)
enum class EColor : uint8
{
	// --- 基础 ---
	EC_None		UMETA(DisplayName = "None / Grey"), // 默认/无色

	// --- [!!] 基础元素色 (Primary Elements) [!!] ---
	EC_Red		UMETA(DisplayName = "Red (Fire)"),
	EC_Yellow	UMETA(DisplayName = "Yellow (Light)"),
	EC_Blue		UMETA(DisplayName = "Blue (Ice)"),
	
	// --- 调节色 (Modifiers) ---
	EC_White	UMETA(DisplayName = "White (Lighten)"),
	EC_Black	UMETA(DisplayName = "Black (Heavify)"),

	// --- [!!] 第二级混合色 (元素 + 元素) [!!] ---
	EC_Orange	UMETA(DisplayName = "Orange (Electric)"), // Red + Yellow
	EC_Green	UMETA(DisplayName = "Green (Life)"),      // Yellow + Blue
	EC_Purple	UMETA(DisplayName = "Purple (Erase)"),    // Red + Blue

	// --- [!!] 第二级混合色 (元素 + 调节色) [!!] ---
	EC_LightRed		UMETA(DisplayName = "Light Red (Steam)"), // Red + White
	EC_DarkRed		UMETA(DisplayName = "Dark Red (Lava)"),   // Red + Black
	EC_LightYellow	UMETA(DisplayName = "Light Yellow (Glow)"), // Yellow + White
	EC_DarkYellow	UMETA(DisplayName = "Dark Yellow (Gold)"),  // Yellow + Black
	EC_LightBlue	UMETA(DisplayName = "Light Blue (Frost)"),// Blue + White
	EC_DarkBlue		UMETA(DisplayName = "Dark Blue (Solid Ice)"),// Blue + Black

	// --- [!!] 第二级混合色 (调节色 + 调节色) [!!] ---
	EC_Grey_Neutral	UMETA(DisplayName = "Grey (Neutral)")
};