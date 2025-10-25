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
	
	// (未来可以在这里添加 IA_Possess, IA_ExtractColor 等)
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	// TObjectPtr<UInputAction> PossessAction;

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

	// (未来可以添加)
	// void HandlePossess(const FInputActionValue& Value);
};
