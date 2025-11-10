#include "TutorialPopupActor.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"       // [!! 关键 !!] 用于暂停游戏
#include "ColorMageCharacter.h"         // [!! 关键 !!] 用于检查玩家
#include "TutorialWidgetBase.h"         // [!! 关键 !!] 包含我们的 UI 基类
#include "GameFramework/PlayerController.h" // [!! 关键 !!] 用于切换输入

ATutorialPopupActor::ATutorialPopupActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 创建触发体积
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap); // 只与 Pawn 重叠
	TriggerVolume->SetGenerateOverlapEvents(true);

	// 2. [!! 关键 !!] 绑定重叠事件
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ATutorialPopupActor::OnOverlapBegin);
}

/** 当有 Actor 进入触发区域时调用 */
void ATutorialPopupActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// --- [!! 1. 检查是否是“第一次” !!] ---
	if (bHasBeenTriggered)
	{
		return; // 已经触发过了，什么也不做
	}

	// 2. 检查是否是玩家角色
	AColorMageCharacter* PlayerCharacter = Cast<AColorMageCharacter>(OtherActor);
	if (PlayerCharacter)
	{
		// 3. 检查是否在蓝图中指定了 UI 类
		if (!TutorialWidgetClass)
		{
			UE_LOG(LogTemp, Error, TEXT("TutorialPopupActor %s: 未指定 TutorialWidgetClass!"), *GetName());
			return;
		}

		APlayerController* PC = Cast<APlayerController>(PlayerCharacter->GetController());
		if (!PC)
		{
			UE_LOG(LogTemp, Error, TEXT("TutorialPopupActor: 找不到 PlayerController!"));
			return;
		}

		// --- [!! 核心逻辑开始 !!] ---
		
		UE_LOG(LogTemp, Warning, TEXT("玩家 %s 首次触发教学: %s"), *PlayerCharacter->GetName(), *GetName());

		// 4. 标记为“已触发”
		bHasBeenTriggered = true;

		// 5. 存储玩家，以便稍后恢复
		TriggeringPlayer = PC;

		// 6. [!! 暂停游戏 !!]
		UGameplayStatics::SetGamePaused(GetWorld(), true);

		// 7. 创建 UI 控件
		ActiveTutorialWidget = CreateWidget<UTutorialWidgetBase>(PC, TutorialWidgetClass);
		if (!ActiveTutorialWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("创建 TutorialWidget 失败!"));
			UGameplayStatics::SetGamePaused(GetWorld(), false); // 恢复游戏
			return;
		}

		// 8. [!! 关键：绑定委托 !!]
		// “订阅” UI 的关闭事件
		ActiveTutorialWidget->OnTutorialClosedDelegate.AddDynamic(this, &ATutorialPopupActor::OnTutorialClosed);

		// 9. 显示 UI
		ActiveTutorialWidget->AddToViewport();

		// 10. [!! 切换输入模式 !!]
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ActiveTutorialWidget->TakeWidget()); // 让 UI 获得焦点
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true); // 显示鼠标
	}
}

/**
 * [!! 关键 !!] 当 UI 广播“关闭”事件时，这个函数会被调用
 */
void ATutorialPopupActor::OnTutorialClosed()
{
	UE_LOG(LogTemp, Warning, TEXT("教学 %s 已关闭，恢复游戏..."), *GetName());

	// 1. [!! 恢复游戏 !!]
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 2. [!! 恢复输入模式 !!]
	if (TriggeringPlayer)
	{
		FInputModeGameOnly InputMode;
		TriggeringPlayer->SetInputMode(InputMode);
		TriggeringPlayer->SetShowMouseCursor(false); // 隐藏鼠标
	}

	// 3. [!! 移除 UI !!]
	if (ActiveTutorialWidget)
	{
		ActiveTutorialWidget->RemoveFromParent();
	}

	// 4. 清理引用
	ActiveTutorialWidget = nullptr;
	TriggeringPlayer = nullptr;

	// (可选) 销毁这个 Actor，因为它不再需要了
	// Destroy();
}