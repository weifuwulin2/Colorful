// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PossessablePawn.h"
#include "GameFramework/Pawn.h"
#include "PlatformPawn.generated.h"

class UBasePawnMovementComponent;
class UInputAction;
UCLASS()
class COLORMAGE_API APlatformPawn : public APossessablePawn
{
	GENERATED_BODY()

public:
	APlatformPawn();

protected:
	/** 移动的输入动作 (需要在蓝图中设置为 IA_Move) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	/** 平台使用的移动组件 (可以在 C++ 或蓝图中指定为 Horizontal 或 Vertical) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	TObjectPtr<UBasePawnMovementComponent> PlatformMovementComponent;

	/** 在 BeginPlay 中查找移动组件 */
	virtual void BeginPlay() override;

	/** 覆盖基类函数以添加移动输入绑定 */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 处理平台移动输入的函数 */
	void HandlePlatformMove(const FInputActionValue& Value);
};
