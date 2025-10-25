// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/Character.h"
#include "ColorMageCharacter.generated.h"

UCLASS()
class COLORMAGE_API AColorMageCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AColorMageCharacter();

protected:
	// --- Enhanced Input 资产 ---

	/** 跳跃 (Space) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	/** 冲刺 (Shift) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;

	// --- 冲刺 (Dash) 设定 ---

	/** 冲刺时播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage;

	/** 冲刺的距离 (单位: cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 1000.0f;

	/** 完成冲刺所需的时长 (单位: 秒) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.25f;

private:
	/** 用来存储角色原始的重力大小 */
	float DefaultGravityScale;

	/** 用来在冲刺结束后恢复设置的计时器句柄 */
	FTimerHandle TimerHandle_DashFinished;


protected:
	/** 在游戏开始时调用 */
	virtual void BeginPlay() override;

	/** 绑定输入 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- 输入处理函数 ---

	/** 处理跳跃 (已有的 ACharacter::Jump) */
	// ACharacter 已经内置了 Jump 函数, 我们直接绑定即可

	/** 处理冲刺 (我们自己实现) */
	void OnDash();

	/** 当冲刺时长结束后，用来恢复重力和停止冲刺的函数 */
	void OnDashFinished();
};
