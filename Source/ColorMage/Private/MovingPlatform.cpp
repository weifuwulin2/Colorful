// MovingPlatform.cpp
#include "MovingPlatform.h"
#include "ColorTypes.h"
#include "HiddenPathActor.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

AMovingPlatform::AMovingPlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    // 设置平台默认外观
    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    }

    // 默认设置
    PlatformMoveDistance = 500.0f;
    PlatformMoveSpeed = 100.0f;
    MoveDirection = EPlatformMoveDirection::EMD_Z_Axis;
    bStartMovingToPositive = true;
}

void AMovingPlatform::BeginPlay()
{
    Super::BeginPlay();

    // 存储平台的起始位置
    PlatformStartLocation = GetActorLocation();
    
    // 计算结束位置
    PlatformEndLocation = CalculateEndLocation();
    
    // 根据起始方向设置初始移动方向
    bIsMovingToEnd = bStartMovingToPositive;
    
    FString DirectionName;
    switch (MoveDirection)
    {
        case EPlatformMoveDirection::EMD_X_Axis:
            DirectionName = TEXT("X轴");
            break;
        case EPlatformMoveDirection::EMD_Y_Axis:
            DirectionName = TEXT("Y轴");
            break;
        case EPlatformMoveDirection::EMD_Z_Axis:
            DirectionName = TEXT("Z轴");
            break;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("MovingPlatform %s: 开始%s移动 %s -> %s"), 
        *GetName(), *DirectionName, *PlatformStartLocation.ToString(), *PlatformEndLocation.ToString());
}

FVector AMovingPlatform::CalculateEndLocation() const
{
    FVector EndLocation = PlatformStartLocation;
    
    switch (MoveDirection)
    {
        case EPlatformMoveDirection::EMD_X_Axis:
            EndLocation.X += PlatformMoveDistance;
            break;
        case EPlatformMoveDirection::EMD_Y_Axis:
            EndLocation.Y += PlatformMoveDistance;
            break;
        case EPlatformMoveDirection::EMD_Z_Axis:
            EndLocation.Z += PlatformMoveDistance;
            break;
    }
    
    return EndLocation;
}

void AMovingPlatform::Tick(float DeltaTime)
{
    if (!CurrentColorOffset.Equals(TargetColorOffset, 1.0f))
    {
        CurrentColorOffset = FMath::VInterpTo(
            CurrentColorOffset,
            TargetColorOffset,
            DeltaTime,
            ColorOffsetSpeed
        );
    }
    // 1. 处理缩放（保持不变）
    if (bIsScalingAutomatically)
    {
        FVector CurrentScale = GetActorScale3D();
        
        if (CurrentScale.Equals(TargetScale, 0.01f))
        {
            bIsScalingAutomatically = false;
            SetActorScale3D(TargetScale);
        }
        else
        {
            FVector NewScale = FMath::VInterpTo(
                CurrentScale,
                TargetScale,
                DeltaTime,
                ScaleSpeed
            );
            SetActorScale3D(NewScale);
        }
    }
    
    // 2. 处理平台移动（如果没被冻结）
    if (!bIsFrozen)
    {
        // 计算移动进度
        float MoveSpeedThisFrame = PlatformMoveSpeed * DeltaTime;
        float DistanceToMove = FVector::Dist(GetCurrentStartLocation(), GetCurrentEndLocation());
        
        if (DistanceToMove > 0.0f)
        {
            float ProgressDelta = MoveSpeedThisFrame / DistanceToMove;
            MovementProgress += ProgressDelta;
            
            // 检查是否到达终点
            if (MovementProgress >= 1.0f)
            {
                MovementProgress = 0.0f;
                bIsMovingToEnd = !bIsMovingToEnd; // 切换方向
            }
            
            // [!! 关键 !!] 计算带有颜色偏移的当前位置
            FVector BaseLocation;
            if (bIsMovingToEnd)
            {
                BaseLocation = FMath::Lerp(GetCurrentStartLocation(), GetCurrentEndLocation(), MovementProgress);
            }
            else
            {
                BaseLocation = FMath::Lerp(GetCurrentEndLocation(), GetCurrentStartLocation(), MovementProgress);
            }
            
            // [!! 关键 !!] 添加颜色高度偏移
            FVector FinalLocation = BaseLocation + GetColorHeightOffset();
            
            SetActorLocation(FinalLocation, true);
        }
    }
}

void AMovingPlatform::HandleColorChange(EColor NewColor, EColor OldColor)
{
    UE_LOG(LogTemp, Warning, TEXT("=== MovingPlatform %s 颜色变化: %d -> %d ==="), *GetName(), (int32)OldColor, (int32)NewColor);
    
    // 清理旧颜色状态（保持不变）
    if (OldColor == EColor::EC_Red)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 离开红色 - 停止火焰伤害"), *GetName());
        GetWorld()->GetTimerManager().ClearTimer(FireDamageTimer);
        BurningWoods.Empty();
    }
    
    if (OldColor == EColor::EC_Yellow)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 离开黄色 - 关闭光照"), *GetName());
        if (PointLight)
        {
            PointLight->SetVisibility(false);
        }
        for (AHiddenPathActor* Path : RevealedPaths)
        {
            if (IsValid(Path))
            {
                Path->Hide();
            }
        }
        RevealedPaths.Empty();
    }

    if (OldColor == EColor::EC_Green)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 离开绿色 - 恢复原始缩放"), *GetName());
        TargetScale = HomeScale;
        bIsScalingAutomatically = true;
    }

    if (OldColor == EColor::EC_Blue)
    {
        UE_LOG(LogTemp, Warning, TEXT("MovingPlatform %s: 解除蓝色冻结！"), *GetName());
        bIsFrozen = false;
    }

    // [!! 关键修改 !!] 设置颜色高度偏移，而不是移动到特定位置
    if (NewColor == EColor::EC_White)
    {
        TargetColorOffset = FVector(0.f, 0.f, MoveDistance); // 上升偏移
        UE_LOG(LogTemp, Log, TEXT("%s: 白色 - 设置上升偏移 %f"), *GetName(), MoveDistance);
    }
    else if (NewColor == EColor::EC_Black)
    {
        TargetColorOffset = FVector(0.f, 0.f, -MoveDistance); // 下降偏移
        UE_LOG(LogTemp, Log, TEXT("%s: 黑色 - 设置下降偏移 %f"), *GetName(), -MoveDistance);
    }
    else if (NewColor != EColor::EC_Blue)
    {
        TargetColorOffset = FVector::ZeroVector; // 无偏移
        UE_LOG(LogTemp, Log, TEXT("%s: 其他颜色 - 无高度偏移"), *GetName());
    }

    // 其他颜色逻辑（光照、拉伸、燃烧）保持不变...
    
    if (NewColor == EColor::EC_Yellow)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 进入黄色 - 激活光照"), *GetName());
        if (PointLight)
        {
            PointLight->SetVisibility(true);
        }
    }

    if (NewColor == EColor::EC_Green)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 进入绿色 - 开始X/Y轴拉伸"), *GetName());
        TargetScale = FVector(
            HomeScale.X * StretchAmount.X,
            HomeScale.Y * StretchAmount.Y,
            HomeScale.Z
        );
        bIsScalingAutomatically = true;
    }

    if (NewColor == EColor::EC_Red)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 进入红色 - 激活燃烧和火焰伤害"), *GetName());
        // 燃烧逻辑...
        GetWorld()->GetTimerManager().SetTimer(
            FireDamageTimer,
            this,
            &AColorableActor::DealFireDamageToPlayer,
            FireDamageInterval,
            true
        );
    }

    if (NewColor == EColor::EC_Blue)
    {
        UE_LOG(LogTemp, Warning, TEXT("MovingPlatform %s: 蓝色冻结！"), *GetName());
        bIsFrozen = true;
    }
}

void AMovingPlatform::UpdatePlatformMovementLocations()
{
    // 以当前位置作为新的起点
    FVector CurrentLocation = GetActorLocation();
    PlatformStartLocation = CurrentLocation;
    
    // 重新计算终点
    switch (MoveDirection)
    {
    case EPlatformMoveDirection::EMD_X_Axis:
        PlatformEndLocation = PlatformStartLocation + FVector(PlatformMoveDistance, 0.0f, 0.0f);
        break;
    case EPlatformMoveDirection::EMD_Y_Axis:
        PlatformEndLocation = PlatformStartLocation + FVector(0.0f, PlatformMoveDistance, 0.0f);
        break;
    case EPlatformMoveDirection::EMD_Z_Axis:
        PlatformEndLocation = PlatformStartLocation + FVector(0.0f, 0.0f, PlatformMoveDistance);
        break;
    }
    
    // 重置移动进度
    MovementProgress = 0.0f;
    bIsMovingToEnd = bStartMovingToPositive;
    
    UE_LOG(LogTemp, Log, TEXT("MovingPlatform %s: 更新移动范围 %s -> %s"), 
        *GetName(), *PlatformStartLocation.ToString(), *PlatformEndLocation.ToString());
}
FVector AMovingPlatform::GetCurrentStartLocation() const
{
    return PlatformStartLocation;
}
FVector AMovingPlatform::GetCurrentEndLocation() const
{
    return PlatformEndLocation;
}
FVector AMovingPlatform::GetColorHeightOffset() const
{
    // [!! 平滑插值到目标偏移 !!]
    return CurrentColorOffset;
}