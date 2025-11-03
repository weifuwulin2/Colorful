// MovingPlatform.h
#pragma once

#include "CoreMinimal.h"
#include "ColorableActor.h"
#include "MovingPlatform.generated.h"

UENUM(BlueprintType)
enum class EPlatformMoveDirection : uint8
{
    EMD_X_Axis      UMETA(DisplayName = "X Axis (Forward/Backward)"),
    EMD_Y_Axis      UMETA(DisplayName = "Y Axis (Left/Right)"),
    EMD_Z_Axis      UMETA(DisplayName = "Z Axis (Up/Down)")
};

UCLASS()
class COLORMAGE_API AMovingPlatform : public AColorableActor
{
    GENERATED_BODY()
    
public:    
    AMovingPlatform();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // 重写颜色处理函数
    virtual void HandleColorChange(EColor NewColor, EColor OldColor) override;

    // 移动设置
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Movement")
    EPlatformMoveDirection MoveDirection = EPlatformMoveDirection::EMD_Z_Axis; // 移动轴向

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Movement")
    float PlatformMoveDistance = 500.0f; // 平台移动距离

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Movement")
    float PlatformMoveSpeed = 100.0f; // 移动速度（单位/秒）

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Movement")
    bool bStartMovingToPositive = true; // true = 先向正方向移动，false = 先向负方向移动

    private:
    // 平台移动相关
    FVector PlatformStartLocation;
    FVector PlatformEndLocation;
    bool bIsFrozen = false;
    bool bIsMovingToEnd = true;
    float MovementProgress = 0.0f;

    // 根据移动方向计算结束位置
    FVector CalculateEndLocation() const;

    void UpdatePlatformMovementLocations();

    // [!! 新增 !!] 当前颜色的高度偏移
    FVector CurrentColorOffset = FVector::ZeroVector;
    FVector TargetColorOffset = FVector::ZeroVector;
    float ColorOffsetSpeed = 5.0f;
    
    // 辅助函数
    FVector GetCurrentStartLocation() const;
    FVector GetCurrentEndLocation() const;
    FVector GetColorHeightOffset() const;

   
    
};
