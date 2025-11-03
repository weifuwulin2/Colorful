// BurnableWoodActor.h - 移除不需要的函数
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BurnableWoodActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UNiagaraComponent;

UCLASS()
class COLORMAGE_API ABurnableWoodActor : public AActor
{
	GENERATED_BODY()
    
public:    
	ABurnableWoodActor();

	UFUNCTION(BlueprintCallable, Category = "Fire")
	void StartBurning();

	UFUNCTION(BlueprintCallable, Category = "Fire")
	bool IsBurning() const { return bIsBurning; }

protected:
	virtual void BeginPlay() override;
	// [!! 移除 !!] 不需要Tick函数

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNiagaraComponent> BurnVFXComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float BurnDuration = 3.0f;

private:
	bool bIsBurning = false;
	FTimerHandle BurnTimerHandle;

	UFUNCTION()
	void OnBurnFinished();

	// [!! 移除 !!] StopBurning, IgniteFromColorableActor 等不需要的函数
};
