// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Pawn.h"
#include "PossessablePawn.generated.h"

class UColorComponent; 
UCLASS()
class COLORMAGE_API APossessablePawn : public APawn
{
	GENERATED_BODY()

public:
	APossessablePawn();

	/** [!! 已修改 !!] 函数现在“转发”给组件 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const;

	/** [!! 已修改 !!] 函数现在“转发”给组件 */
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UColorComponent> ColorComponent;
	
	bool bCanBePossessed = false;
};
