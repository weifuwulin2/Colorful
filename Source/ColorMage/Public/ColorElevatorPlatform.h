// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorableActor.h"
#include "ColorElevatorPlatform.generated.h"

/**
 * 
 */
UCLASS()
class COLORMAGE_API AColorElevatorPlatform : public AColorableActor
{
	GENERATED_BODY()
	
public:	
	AColorElevatorPlatform();

	/** 每一帧调用 */
	virtual void Tick(float DeltaTime) override;

protected:
	/** 游戏开始时调用 */
	virtual void BeginPlay() override;

	/** 当 ColorComponent 颜色改变时调用的 C++ 函数 */
	UFUNCTION()
	void HandleColorChange(EColor NewColor, EColor OldColor);

	/** 平台上升/下降的距离 (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveDistance = 500.0f;

	/** 平台自动移动的平滑速度 (值越大越快) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveSpeed = 5.0f;

private:
	/** 平台的起始/“家”的位置 */
	FVector HomeLocation;
	
	/** 平台自动移动的目标位置 */
	FVector TargetLocation;
	
	/** 平台当前是否正在自动移动 */
	bool bIsMovingAutomatically = false;
};
