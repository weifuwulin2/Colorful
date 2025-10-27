// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Pawn.h"
#include "PossessablePawn.generated.h"

UCLASS()
class COLORMAGE_API APossessablePawn : public APawn
{
	GENERATED_BODY()

public:
	APossessablePawn();

	/** 获取此Pawn的当前颜色 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const { return CurrentColor; }

	/**
	 * 设置此Pawn的颜色 (这个函数现在会自动更新材质)
	 * @param NewColor 要设置的新颜色
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

protected:
	/** 此Pawn当前持有的颜色 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Color Magic")
	EColor CurrentColor = EColor::EC_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// --- [!! 新增: 材质引用 !!] ---

	/** 当颜色为 EC_None (灰色) 时使用的默认材质 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic|Materials")
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	/**
	 * 存储所有彩色材质的字典 (TMap)。
	 * 键 (Key) 是 EColor 枚举，值 (Value) 是对应的材质。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic|Materials")
	TMap<EColor, TObjectPtr<UMaterialInterface>> ColorMaterials;

	// [!! 已移除 !!] - OnColorChanged(EColor NewColor)
	// 我们不再需要蓝图事件了，C++会处理所有逻辑。
	bool bCanBePossessed = false;
};
