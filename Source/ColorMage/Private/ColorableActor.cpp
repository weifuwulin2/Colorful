// ColorableActor.cpp
#include "ColorableActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h"
#include "ColorComponent.h"
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
}

void AColorableActor::HandleColorChange(EColor NewColor, EColor OldColor)
{
    UE_LOG(LogTemp, Warning, TEXT("=== %s 颜色变化: %d -> %d ==="), *GetName(), (int32)OldColor, (int32)NewColor);

    // 处理移动逻辑
    if (NewColor == EColor::EC_White)
    {
        // 白色 → 上升
        TargetLocation = HomeLocation + FVector(0.f, 0.f, MoveDistance);
        bIsMovingAutomatically = true;
        UE_LOG(LogTemp, Log, TEXT("%s: 白色 - 开始上升"), *GetName());
    }
    else if (NewColor == EColor::EC_Black)
    {
        // 黑色 → 下降
        TargetLocation = HomeLocation - FVector(0.f, 0.f, MoveDistance);
        bIsMovingAutomatically = true;
        UE_LOG(LogTemp, Log, TEXT("%s: 黑色 - 开始下降"), *GetName());
    }
    else
    {
        // 其他颜色 → 回到原位
        TargetLocation = HomeLocation;
        bIsMovingAutomatically = true;
    }

    // 处理光照逻辑
    if (NewColor == EColor::EC_Yellow)
    {
        // 黄色 → 发光并照亮隐藏物体
        UE_LOG(LogTemp, Warning, TEXT("%s: 黄色 - 激活光照"), *GetName());
        
        if (PointLight)
        {
            PointLight->SetVisibility(true);
        }

        // 查找并显示隐藏路径
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

            UE_LOG(LogTemp, Warning, TEXT("%s: 光照范围检测到 %d 个对象"), *GetName(), OverlapResults.Num());

            for (const FOverlapResult& Result : OverlapResults)
            {
                if (AHiddenPathActor* Path = Cast<AHiddenPathActor>(Result.GetActor()))
                {
                    UE_LOG(LogTemp, Warning, TEXT("照亮隐藏路径: %s"), *Path->GetName());
                    Path->Reveal();
                    RevealedPaths.AddUnique(Path);
                }
            }
        }
    }
    else if (OldColor == EColor::EC_Yellow)
    {
        // 从黄色变为其他颜色 → 关闭光照
        UE_LOG(LogTemp, Warning, TEXT("%s: 关闭光照"), *GetName());
        
        if (PointLight)
        {
            PointLight->SetVisibility(false);
        }

        // 隐藏所有已显示的路径
        for (AHiddenPathActor* Path : RevealedPaths)
        {
            if (IsValid(Path))
            {
                Path->Hide();
            }
        }
        RevealedPaths.Empty();
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
