// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "Components/ActorComponent.h"
#include "ColorComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COLORMAGE_API UColorComponent : public UActorComponent
{
	GENERATED_BODY()
public:	
	UColorComponent();
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	EColor GetColor() const { return CurrentColor; }
	UFUNCTION(BlueprintCallable, Category = "Color Magic")
	void SetColor(EColor NewColor); // Should only set CurrentColor on server

	protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called automatically when CurrentColor replicates */
	UFUNCTION()
	virtual void OnRep_CurrentColor();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Color Magic")
	TObjectPtr<UStaticMeshComponent> MeshToControl;

	/** Current color state, replicates and calls OnRep_CurrentColor */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentColor, Category = "Color Magic")
	EColor CurrentColor;

	/** The default color applied at BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	EColor DefaultColor = EColor::EC_None;

	/** Material for EC_None */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	TObjectPtr<UMaterialInterface> DefaultMaterial;

	/** Map of colors to materials */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Magic|Materials")
	TMap<EColor, TObjectPtr<UMaterialInterface>> ColorMaterials;
};
