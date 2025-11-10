#include "HiddenPathActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h" // [!! 新增 !!] 包含 BoxComponent 的实现
#include "RevealableInterface.h"

AHiddenPathActor::AHiddenPathActor()
{
	DetectionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionVolume"));
	DetectionVolume->SetupAttachment(RootComponent);
	// [!! 关键修复 !!] 重新配置碰撞
	DetectionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionVolume->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic); // 改为 WorldStatic
	DetectionVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore); // 先全部忽略
	DetectionVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap); // 只对 WorldDynamic 重叠
	DetectionVolume->SetGenerateOverlapEvents(true);
	DetectionVolume->SetHiddenInGame(true); // [!! 调试时先设为可见 !!]
    
	// 设置一个合理的大小
	DetectionVolume->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
	// --- MeshComponent 设置 ---
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetVisibility(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
	// 添加调试日志
	UE_LOG(LogTemp, Warning, TEXT("创建 HiddenPathActor: %s"), *GetName());
}

/** 游戏开始时调用 */
void AHiddenPathActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 调用接口函数来在游戏开始时隐藏自己
	// (这会运行下面的 Hide_Implementation)
	Hide();
}

// --- [!! 3. 接口实现（只控制 MeshComponent） !!] ---

void AHiddenPathActor::Reveal()
{
	UE_LOG(LogTemp, Error, TEXT("=== 路径 %s Reveal 被调用 ==="), *GetName());
    
	if (!MeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("MeshComponent 为空！"));
		return;
	}
	MeshComponent->SetVisibility(true);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
    
	UE_LOG(LogTemp, Error, TEXT("路径 %s 显示完成"), *GetName());
}
void AHiddenPathActor::Hide()
{
	if (!MeshComponent) return;
    
	MeshComponent->SetVisibility(false);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
}

