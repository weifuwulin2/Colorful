// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "Components/ActorComponent.h"
#include "ColorComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COLORMAGE_API UColorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UColorComponent();

	/**
	 * 获取此组件的当前颜色。
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const { return CurrentColor; }

	/**
	 * [!! 已修改 !!] 设置此组件的新颜色。
	 * 这将更新 CurrentColor 变量，并自动触发 OnRep_CurrentColor 来更新材质。
	 * @param NewColor 要设置的新颜色。
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

protected:
	virtual void BeginPlay() override;
	
	/** [!! 新增 !!] 用于网络复制。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * [!! 这就是你的“自动呼叫” !!]
	 * 当 CurrentColor 变量被设置（或复制）时自动调用的函数。
	 * 我们把 OnColorUpdated 的逻辑移到了这里。
	 */
	UFUNCTION()
	virtual void OnRep_CurrentColor();

	/**
	 * 我们要控制的网格体。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Color Magic")
	TObjectPtr<UStaticMeshComponent> MeshToControl;

	/** * [!! 已修改 !!] 此组件的当前颜色状态
	 * 现在使用了 "ReplicatedUsing" 来自动调用 OnRep_CurrentColor
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentColor, Category = "Color Magic")
	EColor CurrentColor;

	// --- 你的材质属性 (保持不变) ---

	/** 此Actor在游戏开始时的默认颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	EColor DefaultColor = EColor::EC_None;

	/** 当颜色为 EC_None (灰色) 时使用的默认材质 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	/** 存储所有彩色材质的字典 (TMap) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	TMap<EColor, TObjectPtr<UMaterialInterface>> ColorMaterials;
};
