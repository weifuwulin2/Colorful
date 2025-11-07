// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ColorMageGameMode.generated.h"

class UNiagaraSystem;
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

	/** [!! 新增 !!] 检查玩家是否已在重生过程中 */
	UFUNCTION(BlueprintPure, Category = "Game Flow")
	bool IsPlayerRespawning(AController* PlayerController) const;
	
protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Flow|Effects")
	TObjectPtr<UNiagaraSystem> RespawnVFX;
	
	/** [!! 新增 !!] 1秒延迟后，真正执行重生逻辑的函数 */
	UFUNCTION()
	void DelayedRespawnLogic(AController* PlayerController);
private:
	/** 存储最后激活的 Checkpoint 的变换 */
	FTransform LastCheckpointTransform;

	UPROPERTY()
	TSet<TWeakObjectPtr<AController>> RespawningPlayers;
};
