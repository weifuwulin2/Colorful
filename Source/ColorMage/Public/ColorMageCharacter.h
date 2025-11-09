// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "PawnControlType.h"
#include "GameFramework/Character.h"
#include "ColorMageCharacter.generated.h"

class UNiagaraSystem;
class USpringArmComponent;
class AColorProjectile;
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class COLORMAGE_API AColorMageCharacter : public ACharacter
{

	GENERATED_BODY()

public:
	AColorMageCharacter();
	// [!! 已修改 !!] Tick 已移除
	// virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "UI")
	EPawnControlType GetControlType() const { return ControlType; }
	void RequestAcquireColor();
	float DefaultGravityScale;

	UFUNCTION(BlueprintCallable, Category = "Effects")
	virtual void PlayUnpossessEffect();

	/** 播放“附身” (返回身体) 的特效 */
	UFUNCTION(BlueprintCallable, Category = "Effects")

	virtual void PlayPossessEffect();

	void RestoreFullHealth()
	{
		CurrentHealth = MaxHealth;
	}
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> UnpossessVFX;

	/** [!! 新增 !!] 当玩家“返回”此角色时播放的VFX */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
	TObjectPtr<UNiagaraSystem> PossessVFX;
	
	// --- [!! GDD 修正：输入 !!] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DashAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireProjectileAction; // (LMB)
	// [!! 已移除 !!] AimAction
	// --- [!! GDD 修正结束 !!] ---

	// --- [!! GDD 修正：简化摄像机 !!] ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float DefaultCameraDist = 600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	FVector DefaultCameraOffset = FVector(0.0f, 150.0f, 60.0f);
	// [!! 已移除 !!] AimingCameraDist, AimingCameraOffset, ZoomInterpSpeed, HipFireRotationSpeed
	// --- [!! GDD 修正结束 !!] ---

	// ... (Dash 和 Projectile Config 保持不变) ...
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDistance = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash")
	float DashDuration = 0.25f;
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AColorProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FName ProjectileSpawnSocketName = "BrushTip";

	/** 角色 UI 控制类型 */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	EPawnControlType ControlType = EPawnControlType::Character;

	/** 玩家按下“射击”时播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> FireProjectileMontage;

	/** 播放蒙太奇后，延迟多少秒才真正发射投射物 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float ProjectileSpawnDelay = 0.3f; // 0.3秒
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> AcquireColorMontage;

	/** 播放蒙太奇后，延迟多少秒才真正执行射线检测 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AcquireColorDelay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float HipFireRotationDuration = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AcquireRotationDuration = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Effects")
	TObjectPtr<UNiagaraSystem> AcquireColorVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Color")
	TMap<EColor, FLinearColor> ColorMapping;

	// --- 头发设置 (保持不变) ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Color")
	int32 HairMaterialSlotIndex = 0; 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Color")
	FName HairColorParameterName = "HairTint";
	/** 玩家被击中时播放的动画 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* GetHitMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* DeathMontage;
	// --- 毛笔设置 (已修改) ---
	/**
	 * [!! 新增 !!]
	 * 毛笔的静态网格体组件。我们将在 C++ 中创建它。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BrushMeshComponent;
	
	/** [!! 新增 !!] 毛笔附加到骨骼上的插槽名称 (例如 "Hand_R_Socket") */
	UPROPERTY(EditDefaultsOnly, Category = "Visuals|Color")
	FName BrushAttachmentSocketName = "Hand_R_Socket"; // (请在编辑器中确认这个插槽名)
	
	/** [!! 已移除 !!] BrushTipMaterialSlotIndex 不再需要 */
	// int32 BrushTipMaterialSlotIndex = 1; 

	/** 材质中控制毛笔尖颜色的参数名称 (保持不变) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visuals|Color")
	FName BrushTipColorParameterName = "BrushTipColor";
private:
	// --- [!! GDD 修正：已清理 !!] ---
	TObjectPtr<USpringArmComponent> CameraSpringArm;
	
	FTimerHandle TimerHandle_DashFinished;
	// [!! 已移除 !!] bIsManuallyAiming, bIsLerpingRotation, TargetRotation, TimerHandle_AutoAimReset
	// [!! 已移除 !!] TargetArmLength, TargetSocketOffset
	// --- [!! GDD 修正结束 !!] ---
	FTimerHandle TimerHandle_SpawnProjectile;
	FTimerHandle TimerHandle_AcquireColor;
	FTimerHandle TimerHandle_ResetFireRotation;
	FTimerHandle TimerHandle_ResetAcquireRotation;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HairDMI;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BrushTipDMI;

	float AcquireDistance = 15500.f;
protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;

	// --- Input Handlers ---
	void OnDash();
	void OnDashFinished();
	/** 当 Actor 被认为掉出世界时由引擎调用 */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	
	void OnFireProjectile();
	void SpawnProjectile_Internal();
	void AcquireColor_Internal();
	void SetAimRotation(bool bLockRotationToCamera);
	void ResetActionState();
	
	UFUNCTION()
	void OnPlayerColorChanged(EColor NewColor);

public:
	// [!! 新增：摄像机设置获取函数 !!]
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCameraDistance() const { return DefaultCameraDist; }
    
	UFUNCTION(BlueprintPure, Category = "Camera") 
	FVector GetCameraOffset() const { return DefaultCameraOffset; }
    
	UFUNCTION(BlueprintPure, Category = "Camera")
	float GetCameraLagSpeed() const { return 3.0f; }

public:
	/** 血量系统 */
	UFUNCTION(BlueprintCallable, Category = "Health")
	void TakeDamage(int32 DamageAmount);
    
	UFUNCTION(BlueprintPure, Category = "Health")
	int32 GetCurrentHealth() const { return CurrentHealth; }
    
	UFUNCTION(BlueprintPure, Category = "Health")
	int32 GetMaxHealth() const { return MaxHealth; }
    
	/** 血量变化委托 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, int32, CurrentHealth, int32, MaxHealth);
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;
protected:
	/** 血量设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 MaxHealth = 3;
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	int32 CurrentHealth = 3;
    
	/** 自动回血设置 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float HealthRegenDelay = 5.0f; // 5秒后开始回血
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float HealthRegenRate = 1.0f; // 每秒回复多少血
    
	private:
	FTimerHandle HealthRegenTimerHandle;
    
	void StartHealthRegeneration();
	void RegenerateHealth();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> AIPerceptionStimuliSourceComponent;
};
