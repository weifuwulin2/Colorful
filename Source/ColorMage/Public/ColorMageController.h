// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ColorMageController.generated.h"

/**
 * 
 */
UCLASS()
class COLORMAGE_API AColorMageController : public APlayerController
{
	GENERATED_BODY()

public:
	AColorMageController();

	// --- Enhanced Input 资产引用 ---
	
	/** 我们的默认输入映射上下文 (在蓝图中指定) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	/** "移动" 输入动作 (在蓝图中指定) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	/** Interact Action (RMB). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Max distance for the RMB interaction line trace. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	float InteractionDistance = 10000.0f;
	
	protected:
	/** 当控制器附身到一个Pawn时调用 */
	virtual void OnPossess(APawn* InPawn) override;

	/** 设置输入组件 */
	virtual void SetupInputComponent() override;

private:
	// --- 输入处理函数 ---

	/**
	 * 绑定的 "MoveAction" (IA_Move) 的触发函数
	 * @param Value IA_Move 传递过来的值 (在我们的例子里是一个 FVector2D)
	 */
	void HandleMove(const FInputActionValue& Value);

	/** Called to handle RMB interaction. */
	void OnInteract();
};
