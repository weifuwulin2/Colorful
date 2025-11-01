// ColorSourceActor.cpp
#include "ColorSourceActor.h"
#include "Components/SceneComponent.h" // [!! ADDED !!]
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "ColorComponent.h"
#include "NiagaraComponent.h"

AColorSourceActor::AColorSourceActor()
{
    PrimaryActorTick.bCanEverTick = false;
    // Create Root Scene Component first
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootSceneComponent);
    // Create MeshComponent and attach to root
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootSceneComponent);
    
    // Create Box Collision Component and attach to root
    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    CollisionBox->SetupAttachment(RootSceneComponent);
    CollisionBox->SetBoxExtent(CollisionBoxExtent);
    
    // [!! FIXED !!] 设置碰撞配置
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 只查询
    CollisionBox->SetCollisionObjectType(ECC_WorldStatic);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap); // [!! CHANGED !!] 阻挡所有对象
    
    // Create NiagaraComponent and attach to mesh
    NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    NiagaraComponent->SetupAttachment(MeshComponent);
    // Apply the scale multiplier
    if (NiagaraComponent)
    {
        NiagaraComponent->SetRelativeScale3D(FVector(EffectScaleMultiplier));
    }
}

void AColorSourceActor::BeginPlay()
{
    Super::BeginPlay();

    // 同步碰撞盒大小
    if (CollisionBox)
    {
        CollisionBox->SetBoxExtent(CollisionBoxExtent);
    }

    UE_LOG(LogTemp, Log, TEXT("ColorSourceActor %s 已创建，提供颜色: %d"), 
           *GetName(), (int32)ColorToProvide);
}
