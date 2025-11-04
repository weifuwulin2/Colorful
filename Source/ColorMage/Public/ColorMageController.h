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
	// ... (OnPawnControlChanged, RequestRepossessOriginalCharacter, InteractionDistance) ...
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FPawnControlChanged OnPawnControlChanged;
	void RequestRepossessOriginalCharacter();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Color Magic")
	float InteractionDistance = 10000.0f; // [!! 注意 !!] 汲取现在使用自己的距离
	UPROPERTY()
	TWeakObjectPtr<AColorMageCharacter> HiddenCharacter = nullptr;
protected:
	// ... (Input Actions: DefaultInputMappingContext, MoveAction, LookAction, AcquireAction, PossessAction) ...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AcquireAction; 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> PossessAction;
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	// ... (HandleMove, HandleLook, OnPossessInteract) ...
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void OnPossessInteract(); // (F 键)

	/** [!! 已修改 !!] (RMB) 现在只转发命令给角色 */
	void OnAcquire(); 

private:
	// ... (HiddenCharacter) ...
	
	friend class UColorManagerSubsystem;
};
