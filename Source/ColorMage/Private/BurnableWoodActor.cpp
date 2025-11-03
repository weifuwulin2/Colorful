// BurnableWoodActor.cpp - 简化版本
#include "BurnableWoodActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"

ABurnableWoodActor::ABurnableWoodActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootSceneComponent);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootSceneComponent);

    // 创建燃烧VFX组件
    BurnVFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BurnVFX"));
    BurnVFXComponent->SetupAttachment(RootSceneComponent);
    BurnVFXComponent->SetAutoActivate(false); // 默认不激活

    // [!! 添加碰撞设置 !!]
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void ABurnableWoodActor::BeginPlay()
{
    Super::BeginPlay();
}

void ABurnableWoodActor::StartBurning()
{
    if (bIsBurning) return;

    bIsBurning = true;
    UE_LOG(LogTemp, Warning, TEXT("木头 %s 开始燃烧！"), *GetName());

    // 播放燃烧VFX
    if (BurnVFXComponent)
    {
        BurnVFXComponent->Activate(true);
    }

    // 设置定时器，燃烧一段时间后消失
    GetWorld()->GetTimerManager().SetTimer(
        BurnTimerHandle,
        this,
        &ABurnableWoodActor::OnBurnFinished,
        BurnDuration,
        false
    );
}

void ABurnableWoodActor::OnBurnFinished()
{
    UE_LOG(LogTemp, Warning, TEXT("木头 %s 燃烧完毕，消失！"), *GetName());
    
    // 燃烧完毕，直接销毁
    Destroy();
}
