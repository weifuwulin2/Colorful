// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ColorManagerSubsystem.generated.h"

// Forward declarations to avoid circular dependencies
class APlayerController;
class AActor;
class AColorSourceActor;
class APossessablePawn;
/**
 * 
 */
UCLASS()
class COLORMAGE_API UColorManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/** (RMB) 处理汲取和混合逻辑 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void HandleAcquireColor(APlayerController* Player, AColorSourceActor* ColorSource);

	/** (F) 尝试附身到"可附身物体" */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void AttemptPossession(APlayerController* Player,  APawn* TargetPawn);

protected:
	// --- [!! 混合逻辑辅助函数 (已重构) !!] ---

	/** 根据 GDD 规则计算新颜色 */
	UFUNCTION()
	EColor GetMixedColor(EColor PlayerCurrentColor, EColor SourceColor) const;
	
	/** 检查颜色是否为基础元素色 (红, 黄, 蓝) */
	bool IsPrimaryElement(EColor Color) const;
	/** 检查颜色是否为调节色 (白, 黑) */
	bool IsModifier(EColor Color) const;
	/** 检查颜色是否为 *任何* 类型的混合色 (橙, 绿, 紫, 浅红, 深红...) */
	bool IsMixedColor(EColor Color) const;
};
