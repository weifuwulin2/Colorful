// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialPopupActor.generated.h"

// 向前声明
class UBoxComponent;
class UTutorialWidgetBase; // [!! 关键 !!] 使用我们的新基类
class AColorMageCharacter;
UCLASS()
class COLORMAGE_API ATutorialPopupActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATutorialPopupActor();

protected:
	/** 用于检测玩家进入的触发区域 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/**
	 * [!! 关键 !!] 在蓝图中指定你想要弹出的教学 UI。
	 * (必须是继承自 UTutorialWidgetBase 的蓝图)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSubclassOf<UTutorialWidgetBase> TutorialWidgetClass;

	/**
	 * [!! 关键 !!] 确保这个触发器只运行一次。
	 * (标记为 SaveGame 以便在关卡持久化，例如存盘)
	 */
	UPROPERTY(VisibleInstanceOnly, SaveGame, Category = "Tutorial")
	bool bHasBeenTriggered = false;

	/** 当有 Actor 进入触发区域时调用 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	 * [!! 关键 !!] 当 UI 广播“关闭”事件时，这个函数会被调用
	 */
	UFUNCTION()
	void OnTutorialClosed();

private:
	/** 存储对当前激活的 UI 控件的引用，以便我们稍后可以移除它 */
	UPROPERTY()
	TObjectPtr<UTutorialWidgetBase> ActiveTutorialWidget;

	/** 存储触发此 UI 的玩家控制器，以便恢复输入 */
	UPROPERTY()
	TObjectPtr<APlayerController> TriggeringPlayer;

};
