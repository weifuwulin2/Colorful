// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UHighlightableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class COLORMAGE_API IHighlightableInterface
{
	GENERATED_BODY()

public:
	/**
	 * [!! 你的回调函数 1 !!]
	 * 当玩家的准星开始瞄准这个 Actor 时调用。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Highlight")
	void OnHighlight();

	/**
	 * [!! 你的回调函数 2 !!]
	 * 当玩家的准星移开这个 Actor 时调用。
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Highlight")
	void OnUnhighlight();
};
