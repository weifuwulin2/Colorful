// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Actor.h"
#include "ColorSourceActor.generated.h"
class UColorComponent;

UCLASS()
class COLORMAGE_API AColorSourceActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AColorSourceActor();

	/** [!! 已修改 !!] 函数现在“转发”给组件 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// --- [!! 新增 !!] ---
	/** 我们新的颜色逻辑组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UColorComponent> ColorComponent;

};
