// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialWidgetBase.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialClosedSignature);
UCLASS()
class COLORMAGE_API UTutorialWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * [!! 关键 !!]
	 * 这是我们的“关闭”事件。
	 * 当这个事件被广播时，任何订阅它的 Actor (比如 ATutorialPopupActor) 都会收到通知。
	 */
	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialClosedSignature OnTutorialClosedDelegate;

	/**
	 * [!! 关键 !!]
	 * 在你的“WBP_Tutorial”蓝图中，让你的“关闭”按钮的 OnClicked 事件调用这个函数。
	 * 这个函数会负责广播上面的委托。
	 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void NotifyTutorialClosed()
	{
		// 广播事件，通知所有监听者
		OnTutorialClosedDelegate.Broadcast();
	}
};
