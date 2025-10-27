// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Actor.h"
#include "ColorProjectile.generated.h"

class UColorComponent;
class USphereComponent;
class UProjectileMovementComponent;
UCLASS()
class COLORMAGE_API AColorProjectile : public AActor
{
	GENERATED_BODY()
public:	
	AColorProjectile();
	void BeginPlay();
	void SetProjectileColor(EColor NewColor);
	EColor GetProjectileColor() const;

protected:
	// ... (CollisionComponent, ProjectileMovementComponent) ...
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UColorComponent> ColorComponent;
	
	/** * [!! 已修改 !!]
	 * 当投射物开始与其他组件重叠时调用。
	 * 函数签名已更改以匹配 OnComponentBeginOverlap。
	 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
