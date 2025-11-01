// ColorableActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ColorableActor.generated.h"

class UColorComponent;
class UStaticMeshComponent;
class USceneComponent;
class USphereComponent;
class UPointLightComponent;
class AHiddenPathActor;

UCLASS()
class COLORMAGE_API AColorableActor : public AActor
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

private:
    // 颜色改变处理
    UFUNCTION()
    void HandleColorChange(EColor NewColor, EColor OldColor);

    // === 移动相关 ===
    FVector HomeLocation;
    FVector TargetLocation;
    bool bIsMovingAutomatically = false;

    // === 光照相关 ===
    UPROPERTY()
    TArray<TObjectPtr<AHiddenPathActor>> RevealedPaths;
};
