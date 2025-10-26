// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Actor.h"
#include "ColorProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
UCLASS()
class COLORMAGE_API AColorProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AColorProjectile();

	/** Sets the color of this projectile (called by the character who fires it). */
	void SetProjectileColor(EColor NewColor);

protected:
	/** The collision sphere. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** The component that makes the projectile move. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	/** The color this projectile will apply on impact. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Color Magic")
	EColor ProjectileColor = EColor::EC_None;

	/** Called when the projectile hits something. */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Blueprint event for visual effects (e.g., changing particle color). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Color Magic")
	void OnColorSet(EColor NewColor);
};
