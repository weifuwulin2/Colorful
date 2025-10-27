// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CheckpointActor.generated.h"

class UBoxComponent;

UCLASS()
class COLORMAGE_API ACheckpointActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ACheckpointActor();

protected:
	/** 用于视觉表现的网格体 (可选) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 用于检测玩家进入的触发区域 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** 当有 Actor 进入触发区域时调用 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Checkpoint 激活后是否禁用 (防止重复触发) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	bool bDisableAfterActivation = true;

private:
	/** 标记此 Checkpoint 是否已被激活 */
	bool bActivated = false;
};
