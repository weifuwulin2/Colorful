// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ColorMageController.generated.h"

class AColorMageCharacter;
/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPawnControlChanged, EPawnControlType, NewPawnType);
UCLASS()
class COLORMAGE_API AColorMageController : public APlayerController
{
	GENERATED_BODY()

public:
	AColorMageController();

	/** 远程交互/附身的最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	float InteractionDistance = 10000.0f;

	/** 请求控制器重新附身之前隐藏的角色 */
	UFUNCTION(BlueprintCallable, Category = "Possession")
	void RequestRepossessOriginalCharacter();

	UPROPERTY()
	TWeakObjectPtr<AColorMageCharacter> HiddenCharacter = nullptr;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FPawnControlChanged OnPawnControlChanged;
	
protected:
	// --- Input 资产 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	/** 移动 (WASD) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	
	/** 远程交互/附身 (E 键) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	/** 摄像机观看 (鼠标移动) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	// --- 输入处理函数 ---
	void HandleMove(const FInputActionValue& Value);
	void OnInteract();
	void HandleLook(const FInputActionValue& Value);

private:
	// 声明 UColorManagerSubsystem 为友元类，允许它访问私有成员 HiddenCharacter
	friend class UColorManagerSubsystem;
};
