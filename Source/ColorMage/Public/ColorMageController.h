// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ColorMageController.generated.h"

class AColorMageCharacter;
/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPawnControlChanged, EPawnControlType, NewPawnType);
UCLASS()
class COLORMAGE_API AColorMageController : public APlayerController
{
	GENERATED_BODY()
public:
	AColorMageController();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FPawnControlChanged OnPawnControlChanged;

	void RequestRepossessOriginalCharacter();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	float InteractionDistance = 10000.0f;

	UPROPERTY()
	TWeakObjectPtr<AColorMageCharacter> HiddenCharacter = nullptr;
protected:
	// --- [!! GDD 修正：输入 !!] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	/** "汲取/混合" 动作 (RMB) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AcquireAction; 
	/** "附身" 动作 (F) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PossessAction;
	// --- [!! GDD 修正结束 !!] ---

	protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	// --- [!! GDD 修正：输入函数 !!] ---
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	/** (RMB) 处理汲取/混合请求 */
	void OnAcquire(); 
	/** (F) 处理附身请求 */
	void OnPossessInteract();
	// --- [!! GDD 修正结束 !!] ---
	private:
	
	friend class UColorManagerSubsystem;
};
