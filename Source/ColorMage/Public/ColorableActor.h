// ColorableActor.h
#pragma once

#include "CoreMinimal.h"
#include "HighlightableInterface.h"
#include "GameFramework/Actor.h"
#include "ColorableActor.generated.h"

class UNiagaraSystem;
class ABurnableWoodActor;
class UColorComponent;
class UStaticMeshComponent;
class USceneComponent;
class USphereComponent;
class UPointLightComponent;
class AHiddenPathActor;

UCLASS()
class COLORMAGE_API AColorableActor : public AActor, public IHighlightableInterface
{
    GENERATED_BODY()
    
public:    
    AColorableActor();

    UFUNCTION(BlueprintCallable, Category = "Color Magic")
    EColor GetColor() const;

    UFUNCTION(BlueprintCallable, Category = "Color Magic")
    void SetColor(EColor NewColor);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // === 基础组件 ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UColorComponent> ColorComponent;

    // === 光照组件 ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> LightVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UPointLightComponent> PointLight;

    // === 配置参数 ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MoveSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
    float LightRadius = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light")
    float LightIntensity = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Fire")
    TObjectPtr<UNiagaraSystem> FireDamageVFX;
    // [!! 添加这些变量 !!] 火焰相关
    UPROPERTY()
    TArray<TObjectPtr<ABurnableWoodActor>> BurningWoods;

    // 颜色改变处理
    UFUNCTION()
    virtual void HandleColorChange(EColor NewColor, EColor OldColor);

    // === 移动相关 ===
    FVector HomeLocation;
    FVector TargetLocation;
    bool bIsMovingAutomatically = false;
    bool bIsScalingAutomatically = false;

    FVector HomeScale;
    FVector TargetScale;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (AllowPrivateAccess = "true"))
    float ScaleSpeed = 2.0f;
    
    // === 光照相关 ===
    UPROPERTY()
    TArray<TObjectPtr<AHiddenPathActor>> RevealedPaths;
    
    // [!! 新增 !!] 红色火焰伤害
    FTimerHandle FireDamageTimer;

    // --- [!! 3. 声明回调函数的 C++ 实现 !!] ---
    virtual void OnHighlight_Implementation() override;
    virtual void OnUnhighlight_Implementation() override;
public:
    UFUNCTION()
    void DealFireDamageToPlayer();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire", meta = (AllowPrivateAccess = "true"))
    float FireDamageInterval = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scale", meta = (AllowPrivateAccess = "true"))
    FVector2D StretchAmount = FVector2D(2.0f, 2.0f); // X和Y轴的拉伸倍数
};
