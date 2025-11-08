#include "PortalActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h" // [!! 关键 !!] 需要包含这个头文件才能使用 OpenLevel
#include "ColorMageCharacter.h"     // [!! 关键 !!] 需要包含你的角色头文件来进行 Cast

APortalActor::APortalActor()
{
	// 传送门通常不需要每帧 Tick
	PrimaryActorTick.bCanEverTick = false;

	// 1. 创建触发体积
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume); // 将触发体积设为根组件

	// 2. 设置触发体积的碰撞
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 只用于查询 (重叠)
	TriggerVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic); // 设为静态物体
	TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); // 默认忽略所有
	TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap); // [!!] 只与 Pawn (玩家) 重叠
	TriggerVolume->SetGenerateOverlapEvents(true); // 确保它能触发事件
	
	
	// 3. 创建可选的视觉网格体
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent); // 附加到触发体积
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 视觉网格体不应阻挡
}

void APortalActor::BeginPlay()
{
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &APortalActor::OnOverlapBegin);
}

/** 当有 Actor 进入触发区域时调用 */
void APortalActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 1. 检查进入的是否是我们的玩家角色
	AColorMageCharacter* PlayerCharacter = Cast<AColorMageCharacter>(OtherActor);

	if (PlayerCharacter)
	{
		// 2. 检查关卡名称是否有效
		if (NextLevelName.IsNone())
		{
			UE_LOG(LogTemp, Error, TEXT("PortalActor %s: 未设置 NextLevelName!"), *GetName());
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("玩家 %s 已进入传送门，正在加载关卡: %s"), *PlayerCharacter->GetName(), *NextLevelName.ToString());

		// 3. [!! 关键 !!] 加载新关卡
		// "this" 是 WorldContextObject (世界上下文对象)
		UGameplayStatics::OpenLevel(this, NextLevelName);
	}
}