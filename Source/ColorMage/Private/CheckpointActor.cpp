#include "CheckpointActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ColorMageCharacter.h"     // 需要包含角色头文件
#include "ColorMageGameMode.h"
#include "Kismet/GameplayStatics.h" // 用于获取 GameMode

ACheckpointActor::ACheckpointActor()
{
	PrimaryActorTick.bCanEverTick = false; // Checkpoint 通常不需要 Tick

	// 创建触发区域
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume); // 将触发区域设为根组件
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic")); // 设置碰撞预设
	TriggerVolume->SetGenerateOverlapEvents(true); // 确保生成重叠事件
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointActor::OnOverlapBegin); // 绑定事件

	// 创建可选的视觉网格体
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent); // 附加到触发区域
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 网格体本身不需要碰撞
}

void ACheckpointActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 1. 检查此 Checkpoint 是否已激活且设置为禁用
	if (bActivated && bDisableAfterActivation)
	{
		return; // 如果已激活且禁用，则不执行任何操作
	}

	// 2. 检查进入触发区域的是否是玩家角色
	AColorMageCharacter* PlayerCharacter = Cast<AColorMageCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		// 3. 获取当前的 GameMode
		AGameModeBase* CurrentGameModeBase = UGameplayStatics::GetGameMode(this);
		// 尝试转换为你的特定 GameMode (你需要创建一个 C++ GameMode 类)
		AColorMageGameMode* MyGameMode = Cast<AColorMageGameMode>(CurrentGameModeBase); 

		if (MyGameMode)
		{
			UE_LOG(LogTemp, Log, TEXT("玩家触碰 Checkpoint: %s"), *GetName());
			
			// 4. 调用 GameMode 的函数来更新重生点
			// 我们将重生点设为此 Checkpoint Actor 的位置和旋转
			MyGameMode->UpdateCheckpoint(GetActorTransform());

			// 5. 标记为已激活
			bActivated = true;

			// (可选) 在这里播放激活音效或特效
			// UGameplayStatics::SpawnEmitterAtLocation(...)
			// UGameplayStatics::PlaySoundAtLocation(...)

			// (可选) 如果设置了禁用，可以隐藏 Mesh 或禁用 TriggerVolume
			if (bDisableAfterActivation)
			{
				// TriggerVolume->SetGenerateOverlapEvents(false); // 停止检测
				// MeshComponent->SetVisibility(false); // 隐藏
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CheckpointActor 无法获取 MyGameMode!"));
		}
	}
}