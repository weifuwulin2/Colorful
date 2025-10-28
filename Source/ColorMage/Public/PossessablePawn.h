// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "InputAction.h"
#include "PawnControlType.h"
#include "GameFramework/Pawn.h"
#include "PossessablePawn.generated.h"

class UColorComponent; 
UCLASS()
class COLORMAGE_API APossessablePawn : public APawn
{
	GENERATED_BODY()

public:
	APossessablePawn();

	/** 获取此Pawn的当前颜色 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const;

	/** 设置此Pawn的颜色 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

	/** 获取角色解除附身后应该出现的世界变换（位置和旋转） */
	UFUNCTION(BlueprintCallable, Category = "Possession")
	FTransform GetCharacterExitTransform() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	EPawnControlType GetControlType() const { return ControlType; }

protected:
	/** 用于视觉表现的基础网格体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 处理颜色逻辑的组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UColorComponent> ColorComponent;

	/** 用于标记角色退出位置的场景组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Possession")
	TObjectPtr<USceneComponent> CharacterExitPoint;

	/** 解除附身的输入动作 (由子类或蓝图配置) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> UnpossessAction;

	/** 设置输入绑定 (现在只绑定解除附身) */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 处理解除附身请求的函数 */
	void OnUnpossess();
	
	bool bCanBePossessed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI") // EditDefaultsOnly allows setting in child C++ constructors/BPs
	EPawnControlType ControlType = EPawnControlType::Unknown; // Base class is Unknown
};
