// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "Components/ActorComponent.h"
#include "ColorComponent.generated.h"

class UNiagaraSystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorChanged, EColor, NewColor, EColor, OldColor);
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COLORMAGE_API UColorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UColorComponent();

	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const { return CurrentColor; }

	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor);

	/** C++ 委托，当颜色改变时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Color Magic|Events")
	FOnColorChanged OnColorChanged;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** RepNotify 函数，当 CurrentColor 改变时自动调用 */
	UFUNCTION()
	virtual void OnRep_CurrentColor();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Color Magic")
	TObjectPtr<UStaticMeshComponent> MeshToControl;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentColor, Category = "Color Magic")
	EColor CurrentColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	EColor DefaultColor = EColor::EC_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	TMap<EColor, TObjectPtr<UMaterialInterface>> ColorMaterials;
	
	/** 蓝图事件，用于实现物理效果等 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Color Magic", meta=(DisplayName="On Color Effect Changed (Blueprint)"))
	void K2_OnColorEffectChanged(EColor NewColor, EColor OldColor);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Effects")
	TMap<EColor, TObjectPtr<UNiagaraSystem>> ColorPaintVFX;

	/** 当被染成 EC_None (灰色) 时播放的 Niagara VFX */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Effects")
	TObjectPtr<UNiagaraSystem> DefaultPaintVFX;

private:
	UPROPERTY()
	EColor PreviousColor;
};
