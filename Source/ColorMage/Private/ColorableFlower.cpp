#include "ColorableFlower.h"
#include "ColorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PointLightComponent.h" // [!! 改为 PointLight !!]
#include "HiddenPathActor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

AColorableFlower::AColorableFlower()
{
    PrimaryActorTick.bCanEverTick = false;
    
    // 创建光照体积（用于可视化和 Sphere Trace 的半径参考）
    LightVolume = CreateDefaultSubobject<USphereComponent>(TEXT("LightVolume"));
    LightVolume->SetupAttachment(RootComponent);
    LightVolume->SetSphereRadius(LightRadius);
    LightVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 不需要碰撞检测
    LightVolume->SetHiddenInGame(false); // 调试时可见，发布时可改为 true

    // [!! 创建点光源 !!]
    PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
    PointLight->SetupAttachment(RootComponent);
    PointLight->SetIntensity(LightIntensity); // 使用可配置的强度
    PointLight->SetAttenuationRadius(LightRadius); // 光照衰减半径与检测半径匹配
    PointLight->SetLightColor(FLinearColor::Yellow); // 设置为黄色光
    PointLight->SetVisibility(false); // 默认关闭

    // 初始化数组
    RevealedPaths.Empty();
    
    UE_LOG(LogTemp, Warning, TEXT("创建 ColorableFlower: %s"), *GetName());
}

void AColorableFlower::BeginPlay()
{
    Super::BeginPlay();

    // 绑定 ColorComponent 的颜色改变事件
    if (ColorComponent)
    {
        ColorComponent->OnColorChanged.AddDynamic(this, &AColorableFlower::HandleColorChange);
        UE_LOG(LogTemp, Warning, TEXT("ColorableFlower %s 成功绑定颜色改变事件"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ColorableFlower %s 找不到继承的 ColorComponent!"), *GetName());
    }

    // 同步光照体积和点光源的参数（如果在蓝图中修改了）
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

void AColorableFlower::HandleColorChange(EColor NewColor, EColor OldColor)
{
    UE_LOG(LogTemp, Error, TEXT("=== 花朵 %s 颜色变化调试 ==="), *GetName());
    UE_LOG(LogTemp, Error, TEXT("颜色变化: %d -> %d"), (int32)OldColor, (int32)NewColor);
    UE_LOG(LogTemp, Error, TEXT("花朵位置: %s"), *GetActorLocation().ToString());
    
    if (NewColor == EColor::EC_Yellow)
    {
        UE_LOG(LogTemp, Error, TEXT("开始激活黄色光照逻辑"));
        
        // 激活点光源
        if (PointLight)
        {
            PointLight->SetVisibility(true);
            UE_LOG(LogTemp, Error, TEXT("点光源已激活"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("错误：点光源为空！"));
        }

        // Sphere Trace 调试
        FVector TraceCenter = GetActorLocation();
        float TraceRadius = LightRadius;
        
        UE_LOG(LogTemp, Error, TEXT("准备 Sphere Trace - 中心: %s, 半径: %.1f"), 
               *TraceCenter.ToString(), TraceRadius);

        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("致命错误：World 为空！"));
            return;
        }

        // [!! 先尝试查询所有类型的对象 !!]
        TArray<FOverlapResult> AllResults;
        FCollisionQueryParams QueryParams;
        QueryParams.bTraceComplex = false;
        QueryParams.AddIgnoredActor(this);

        bool bHasAnyOverlaps = World->OverlapMultiByChannel(
            AllResults,
            TraceCenter,
            FQuat::Identity,
            ECC_WorldStatic, // 或者试试 ECC_Pawn
            FCollisionShape::MakeSphere(TraceRadius),
            QueryParams
        );

        UE_LOG(LogTemp, Error, TEXT("Sphere Trace (所有对象) 找到 %d 个对象"), AllResults.Num());

        // 打印所有找到的对象
        for (int32 i = 0; i < AllResults.Num(); i++)
        {
            if (AllResults[i].GetActor())
            {
                UE_LOG(LogTemp, Error, TEXT("对象 %d: %s (类型: %s)"), 
                       i, 
                       *AllResults[i].GetActor()->GetName(), 
                       *AllResults[i].GetActor()->GetClass()->GetName());
            }
        }

        // 现在专门查找 HiddenPathActor
        TArray<FOverlapResult> OverlapResults;
        bool bHasOverlaps = World->OverlapMultiByObjectType(
            OverlapResults,
            TraceCenter,
            FQuat::Identity,
            FCollisionObjectQueryParams(ECC_WorldStatic),
            FCollisionShape::MakeSphere(TraceRadius),
            QueryParams
        );

        UE_LOG(LogTemp, Error, TEXT("Sphere Trace (WorldStatic) 找到 %d 个对象"), OverlapResults.Num());

        // 处理路径
        int32 PathCount = 0;
        for (const FOverlapResult& Result : OverlapResults)
        {
            AActor* Actor = Result.GetActor();
            if (Actor)
            {
                UE_LOG(LogTemp, Error, TEXT("检查对象: %s"), *Actor->GetName());
                
                if (AHiddenPathActor* Path = Cast<AHiddenPathActor>(Actor))
                {
                    UE_LOG(LogTemp, Error, TEXT("找到 HiddenPathActor: %s，尝试显示"), *Path->GetName());
                    Path->Reveal();
                    RevealedPaths.AddUnique(Path);
                    PathCount++;
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("对象 %s 不是 HiddenPathActor"), *Actor->GetName());
                }
            }
        }
        
        UE_LOG(LogTemp, Error, TEXT("=== 总共处理了 %d 条路径 ==="), PathCount);
    }
    else if (OldColor == EColor::EC_Yellow)
    {
        UE_LOG(LogTemp, Error, TEXT("停用黄色光照"));
        
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
}
