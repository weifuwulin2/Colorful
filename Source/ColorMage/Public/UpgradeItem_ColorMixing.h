// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UpgradeItem_ColorMixing.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class AColorMageCharacter;
UCLASS()
class COLORMAGE_API AUpgradeItem_ColorMixing : public AActor
{
	GENERATED_BODY()
	
public:	
	AUpgradeItem_ColorMixing();

protected:
	/** 用于视觉表现的网格体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 用于检测玩家进入的触发区域 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** 当有 Actor 进入触发区域时调用 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
