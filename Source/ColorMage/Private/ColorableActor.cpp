// ColorableActor.cpp
#include "ColorableActor.h"

#include "BurnableWoodActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "ColorComponent.h"
#include "ColorMageCharacter.h"
#include "HiddenPathActor.h"
#include "Engine/World.h"

AColorableActor::AColorableActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // 创建基础组件
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootSceneComponent);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootSceneComponent);

    ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));

    // 创建光照组件
    LightVolume = CreateDefaultSubobject<USphereComponent>(TEXT("LightVolume"));
    LightVolume->SetupAttachment(RootComponent);
    LightVolume->SetSphereRadius(LightRadius);
    LightVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LightVolume->SetHiddenInGame(false); // 调试时可见

    PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
    PointLight->SetupAttachment(RootComponent);
    PointLight->SetIntensity(LightIntensity);
    PointLight->SetAttenuationRadius(LightRadius);
    PointLight->SetLightColor(FLinearColor::Yellow);
    PointLight->SetVisibility(false); // 默认关闭
}

void AColorableActor::BeginPlay()
{
    Super::BeginPlay();

    // 存储起始位置
    HomeLocation = GetActorLocation();
    TargetLocation = HomeLocation;
    
    HomeScale = GetActorScale3D();
    TargetScale = HomeScale;
    // 绑定颜色改变事件
    if (ColorComponent)
    {
        ColorComponent->OnColorChanged.AddDynamic(this, &AColorableActor::HandleColorChange);
        UE_LOG(LogTemp, Log, TEXT("ColorableActor %s: 颜色委托绑定成功"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ColorableActor %s: 找不到 ColorComponent!"), *GetName());
    }

    // 同步光照参数
    if (LightVolume)
    {
        LightVolume->SetSphereRadius(LightRadius);
    }
    
    if (PointLight)
    {
        PointLight->SetIntensity(LightIntensity);
        PointLight->SetAttenuationRadius(LightRadius);
    }
}

void AColorableActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 平滑移动逻辑
    if (bIsMovingAutomatically)
    {
        FVector CurrentLocation = GetActorLocation();
        
        if (CurrentLocation.Equals(TargetLocation, 1.0f))
        {
            // 到达目标
            bIsMovingAutomatically = false;
            SetActorLocation(TargetLocation, false);
            UE_LOG(LogTemp, Log, TEXT("%s 到达目标位置 %s"), *GetName(), *TargetLocation.ToString());
        }
        else
        {
            // 插值移动
            FVector NewLocation = FMath::VInterpTo(
                CurrentLocation,
                TargetLocation,
                DeltaTime,
                MoveSpeed
            );
            SetActorLocation(NewLocation, true);
        }
    }

    // [!! 新增 !!] 平滑缩放逻辑
    if (bIsScalingAutomatically)
    {
        FVector CurrentScale = GetActorScale3D();
        
        if (CurrentScale.Equals(TargetScale, 0.01f))
        {
            // 到达目标缩放
            bIsScalingAutomatically = false;
            SetActorScale3D(TargetScale);
            UE_LOG(LogTemp, Log, TEXT("%s 到达目标缩放 %s"), *GetName(), *TargetScale.ToString());
        }
        else
        {
            // 插值缩放
            FVector NewScale = FMath::VInterpTo(
                CurrentScale,
                TargetScale,
                DeltaTime,
                ScaleSpeed
            );
            SetActorScale3D(NewScale);
        }
    }
}

void AColorableActor::HandleColorChange(EColor NewColor, EColor OldColor)
{
    UE_LOG(LogTemp, Warning, TEXT("=== %s 颜色变化: %d -> %d ==="), *GetName(), (int32)OldColor, (int32)NewColor);
    
    // [!! 修正 !!] 如果旧颜色是红色，清理火焰伤害计时器
    if (OldColor == EColor::EC_Red)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 离开红色 - 停止火焰伤害"), *GetName());
        GetWorld()->GetTimerManager().ClearTimer(FireDamageTimer);
        BurningWoods.Empty();
    }
    
    // [!! 修正 !!] 如果旧颜色是黄色，清理光照
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

    // [!! 新增 !!] 如果旧颜色是绿色，恢复原始缩放
    if (OldColor == EColor::EC_Green)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 离开绿色 - 恢复原始缩放"), *GetName());
        TargetScale = HomeScale;
        bIsScalingAutomatically = true;
    }

    // 处理移动逻辑
    if (NewColor == EColor::EC_White)
    {
        TargetLocation = HomeLocation + FVector(0.f, 0.f, MoveDistance);
        bIsMovingAutomatically = true;
        UE_LOG(LogTemp, Log, TEXT("%s: 白色 - 开始上升"), *GetName());
    }
    else if (NewColor == EColor::EC_Black)
    {
        TargetLocation = HomeLocation - FVector(0.f, 0.f, MoveDistance);
        bIsMovingAutomatically = true;
        UE_LOG(LogTemp, Log, TEXT("%s: 黑色 - 开始下降"), *GetName());
    }
    else
    {
        TargetLocation = HomeLocation;
        bIsMovingAutomatically = true;
    }

    // 处理光照逻辑（黄色）
    if (NewColor == EColor::EC_Yellow)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 进入黄色 - 激活光照"), *GetName());
        
        if (PointLight)
        {
            PointLight->SetVisibility(true);
        }
        UWorld* World = GetWorld();
        if (World)
        {
            TArray<FOverlapResult> OverlapResults;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(this);
            bool bHasOverlaps = World->OverlapMultiByObjectType(
                OverlapResults,
                GetActorLocation(),
                FQuat::Identity,
                FCollisionObjectQueryParams(ECC_WorldStatic),
                FCollisionShape::MakeSphere(LightRadius),
                QueryParams
            );
            for (const FOverlapResult& Result : OverlapResults)
            {
                if (AHiddenPathActor* Path = Cast<AHiddenPathActor>(Result.GetActor()))
                {
                    Path->Reveal();
                    RevealedPaths.AddUnique(Path);
                }
            }
        }
    }

    // [!! 新增 !!] 处理拉伸逻辑（绿色）
    if (NewColor == EColor::EC_Green)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 进入绿色 - 开始X/Y轴拉伸"), *GetName());
        
        // 计算拉伸后的缩放值（只拉伸X和Y轴，保持Z轴不变）
        TargetScale = FVector(
            HomeScale.X * StretchAmount.X,  // X轴拉伸
            HomeScale.Y * StretchAmount.Y,  // Y轴拉伸
            HomeScale.Z                     // Z轴保持不变
        );
        
        bIsScalingAutomatically = true;
        
        UE_LOG(LogTemp, Log, TEXT("%s: 绿色拉伸 - 从 %s 到 %s"), 
            *GetName(), *HomeScale.ToString(), *TargetScale.ToString());
    }

    // 处理燃烧逻辑（红色）
    if (NewColor == EColor::EC_Red)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: 进入红色 - 激活燃烧和火焰伤害"), *GetName());
        
        // 点燃附近的木头
        UWorld* World = GetWorld();
        if (World)
        {
            TArray<FOverlapResult> OverlapResults;
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(this);
            bool bHasOverlaps = World->OverlapMultiByObjectType(
                OverlapResults,
                GetActorLocation(),
                FQuat::Identity,
                FCollisionObjectQueryParams(ECC_WorldStatic),
                FCollisionShape::MakeSphere(LightRadius),
                QueryParams
            );
            for (const FOverlapResult& Result : OverlapResults)
            {
                if (ABurnableWoodActor* Wood = Cast<ABurnableWoodActor>(Result.GetActor()))
                {
                    if (!Wood->IsBurning())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("点燃木头: %s"), *Wood->GetName());
                        Wood->StartBurning();
                        BurningWoods.AddUnique(Wood);
                    }
                }
            }
        }
        
        // 开始对玩家造成火焰伤害
        GetWorld()->GetTimerManager().SetTimer(
            FireDamageTimer,
            this,
            &AColorableActor::DealFireDamageToPlayer,
            FireDamageInterval,
            true
        );
    }
}

// [!! 新增 !!] 火焰伤害函数
void AColorableActor::DealFireDamageToPlayer()
{
    UWorld* World = GetWorld();
    if (!World) return;
    // 检测范围内的玩家
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    bool bHasOverlaps = World->OverlapMultiByObjectType(
        OverlapResults,
        GetActorLocation(),
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn), // 检测玩家
        FCollisionShape::MakeSphere(LightRadius), // 使用灯光范围作为火焰伤害范围
        QueryParams
    );
    for (const FOverlapResult& Result : OverlapResults)
    {
        if (AColorMageCharacter* Player = Cast<AColorMageCharacter>(Result.GetActor()))
        {
            UE_LOG(LogTemp, Error, TEXT("玩家 %s 被红色ColorableActor烧伤！"), *Player->GetName());
            
            // 杀死玩家或造成伤害
            // 临时方案：将玩家传送到上方（模拟死亡重生）
            FVector RespawnLocation = Player->GetActorLocation() + FVector(0, 0, 1000);
            Player->SetActorLocation(RespawnLocation);
            
            UE_LOG(LogTemp, Warning, TEXT("玩家因红色火焰死亡，重生到: %s"), *RespawnLocation.ToString());
            
            // 你也可以实现更复杂的死亡/重生逻辑
            // 比如调用游戏模式的重生函数，播放死亡动画等
        }
    }
}
EColor AColorableActor::GetColor() const
{
    return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None;
}

void AColorableActor::SetColor(EColor NewColor)
{
    if (ColorComponent)
    {
        ColorComponent->SetColor(NewColor);
    }
}
