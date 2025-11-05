#include "UpgradeItem_ColorMixing.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ColorMageCharacter.h"     // 包含角色
#include "ColorMageController.h"   // 包含控制器
#include "ColorMagePlayerState.h"  // 包含玩家状态
#include "Kismet/GameplayStatics.h" // 用于播放声音/特效 (可选)

AUpgradeItem_ColorMixing::AUpgradeItem_ColorMixing()
{
	PrimaryActorTick.bCanEverTick = false;

	// 使用 BoxComponent 作为根，以便在 MeshComponent 被销毁后仍能安全处理
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // 确保能重叠
	TriggerVolume->SetGenerateOverlapEvents(true);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 网格体不阻挡

	// 绑定重叠事件
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AUpgradeItem_ColorMixing::OnOverlapBegin);
}

void AUpgradeItem_ColorMixing::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 1. 检查是否是玩家角色
	AColorMageCharacter* PlayerCharacter = Cast<AColorMageCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		// 2. 获取玩家控制器
		APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController());
		if (PC)
		{
			// 3. 获取玩家状态
			AColorMagePlayerState* PS = PC->GetPlayerState<AColorMagePlayerState>();
			if (PS)
			{
				UE_LOG(LogTemp, Warning, TEXT("玩家 %s 拾取了混色道具!"), *PS->GetPlayerName());

				// 4. [!! 关键 !!] 调用服务器函数来解锁能力
				PS->Server_EnableColorMixing();

				// 5. (可选) 播放拾取音效和VFX
				// UGameplayStatics::PlaySoundAtLocation(...);
				// UGameplayStatics::SpawnEmitterAtLocation(...);

				// 6. 销毁道具
				Destroy();
			}
		}
	}
}