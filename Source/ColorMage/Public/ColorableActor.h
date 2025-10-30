// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ColorableActor.generated.h"

class UColorComponent;
class UStaticMeshComponent;
class USceneComponent;

UCLASS(Abstract)
class COLORMAGE_API AColorableActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AColorableActor();

	/**
	 * 获取此 Actor 颜色的快捷方式
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const;

	/**
	 * 设置此 Actor 颜色的快捷方式
	 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

protected:
	/** 根组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	/** 视觉网格体 (子类可以在蓝图中设置) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** [!! 关键 !!]
	 * 所有 AColorableActor 都会自动拥有这个组件
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UColorComponent> ColorComponent;

};
