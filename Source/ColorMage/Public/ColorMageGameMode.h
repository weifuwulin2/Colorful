// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ColorMageGameMode.generated.h"

/**
 * 
 */
UCLASS()
class COLORMAGE_API AColorMageGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AColorMageGameMode();

	/**
	 * 更新当前激活的 Checkpoint (由 CheckpointActor 调用)
	 * @param NewCheckpointTransform 新重生点的位置和旋转
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void UpdateCheckpoint(const FTransform& NewCheckpointTransform);

	/**
	 * 重生指定的玩家 (由角色或 KillZVolume 触发)
	 * @param PlayerController 需要重生的玩家控制器
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Flow")
	void RespawnPlayer(AController* PlayerController);

protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

private:
	/** 存储最后激活的 Checkpoint 的变换 */
	FTransform LastCheckpointTransform;	
};
