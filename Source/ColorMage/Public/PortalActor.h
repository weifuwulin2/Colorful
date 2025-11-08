// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalActor.generated.h"

// 向前声明
class UBoxComponent;
class UStaticMeshComponent;
class AColorMageCharacter;

UCLASS()
class COLORMAGE_API APortalActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// 构造函数
	APortalActor();

protected:
	/** 用于显示传送门视觉效果的网格体 (可选) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 用于检测玩家进入的触发区域 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** * [!! 关键 !!]
	 * 在编辑器中设置要加载的下一关卡的【确切】名称 
	 * (例如: "Level_2" 或 "UEDPIE_0_Level_2")
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal")
	FName NextLevelName;
	virtual void BeginPlay() override;
	/** 当有 Actor 进入触发区域时调用 */
	UFUNCTION() // 必须是 UFUNCTION() 才能绑定到委托
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
