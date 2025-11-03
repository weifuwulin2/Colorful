// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "InputAction.h"
#include "PawnControlType.h"
#include "GameFramework/Pawn.h"
#include "PossessablePawn.generated.h"

class UColorComponent; 
UCLASS()
class COLORMAGE_API APossessablePawn : public APawn
{
	GENERATED_BODY()
public:
	APossessablePawn();
	EColor GetColor() const;
	void SetColor(EColor NewColor);
	FTransform GetCharacterExitTransform() const;
	
	UFUNCTION(BlueprintPure, Category = "UI")
	EPawnControlType GetControlType() const { return ControlType; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UColorComponent> ColorComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Possession")
	TObjectPtr<USceneComponent> CharacterExitPoint;

	// --- [!! GDD 修正 !!] ---
	/** "附身" 动作 (F 键)，在附身时用于 "解除附身" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PossessAction; // <-- 重命名以匹配新按键
	
	/** 由子类在构造函数中设置 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPawnControlType ControlType = EPawnControlType::Unknown;
	// --- [!! GGDD 修正结束 !!] ---

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	void OnUnpossess();
	/** 当 Actor 被认为掉出世界时由引擎调用 */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

	bool bCanBePossessed = false;
};
