// ColorSourceActor.h
#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "GameFramework/Actor.h"
#include "ColorSourceActor.generated.h"

class UColorComponent;
class UNiagaraComponent;
class UBoxComponent;
class USceneComponent; // [!! ADDED !!]

UCLASS()
class COLORMAGE_API AColorSourceActor : public AActor
{
	GENERATED_BODY()
    
public: 
	AColorSourceActor();

	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColorToProvide() const { return ColorToProvide; }

protected:
	virtual void BeginPlay() override;

	// === 组件 ===
	// [!! ADDED !!] Root Scene Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	// === 配置 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	float EffectScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic")
	FVector CollisionBoxExtent = FVector(150.0f, 150.0f, 150.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	EColor ColorToProvide = EColor::EC_Red;
	
};
