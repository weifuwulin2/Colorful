// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorableActor.h"
#include "Components/SphereComponent.h"
#include "ColorableFlower.generated.h"

class USphereComponent;
class UPointLightComponent;
class AHiddenPathActor;
/**
 * 
 */
UCLASS()
class COLORMAGE_API AColorableFlower : public AColorableActor
{
	GENERATED_BODY()
public:
	AColorableFlower();
protected:
	virtual void BeginPlay() override;
	// [!! 光照体积组件 !!]
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
	USphereComponent* LightVolume;
	// [!! 点光源组件 !!]
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Light")
	UPointLightComponent* PointLight;
	// [!! 记录当前显示的路径 !!]
	UPROPERTY()
	TArray<AHiddenPathActor*> RevealedPaths;
	// [!! 颜色改变处理函数 !!]
	UFUNCTION()
	void HandleColorChange(EColor NewColor, EColor OldColor);
public:
	// 可选：蓝图中可调整光照半径
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float LightRadius = 500.0f;
	// 可选：蓝图中可调整光照强度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
	float LightIntensity = 5000.0f;
	
};
