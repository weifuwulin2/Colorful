#include "ColorManagerSubsystem.h"
#include "ColorMagePlayerState.h"
#include "ColorSourceActor.h"
#include "PossessablePawn.h"
#include "GameFramework/PlayerController.h"
#include "ColorMageCharacter.h" // 需要包含角色头文件
#include "ColorMageController.h" // 需要包含控制器头文件

/** 处理交互的总入口 */
void UColorManagerSubsystem::HandlePlayerInteraction(APlayerController* Player, AActor* HitActor)
{
	if (!Player || !HitActor) return;

	// 情况 1: 击中颜色源
	if (AColorSourceActor* ColorSource = Cast<AColorSourceActor>(HitActor))
	{
		ExtractColor(Player, ColorSource);
		return;
	}

	// 情况 2: 击中可附身 Pawn
	if (APossessablePawn* PossessablePawn = Cast<APossessablePawn>(HitActor))
	{
		AttemptPossession(Player, PossessablePawn);
		return;
	}
}

/** 汲取颜色 */
void UColorManagerSubsystem::ExtractColor(APlayerController* Player, AColorSourceActor* ColorSource)
{
	AColorMagePlayerState* PlayerState = Player->GetPlayerState<AColorMagePlayerState>();
	if (PlayerState && ColorSource)
	{
		// 通过服务器设置 PlayerState 的颜色
		PlayerState->Server_SetCurrentColor(ColorSource->GetColor());
	}
}

/** 尝试附身 (包含隐藏逻辑) */
void UColorManagerSubsystem::AttemptPossession(APlayerController* Player, APossessablePawn* TargetPawn)
{
	// 获取必要的状态和对象
	AColorMagePlayerState* PlayerState = Player->GetPlayerState<AColorMagePlayerState>();
	AColorMageController* MageController = Cast<AColorMageController>(Player);
	AColorMageCharacter* CurrentCharacter = Cast<AColorMageCharacter>(Player->GetPawn()); // 获取当前控制的角色

	// 确保所有对象都有效
	if (!PlayerState || !TargetPawn || !MageController || !CurrentCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("AttemptPossession 失败：缺少必要对象。"));
		return;
	}

	EColor PlayerColor = PlayerState->GetCurrentColor();
	EColor TargetColor = TargetPawn->GetColor();

	// 检查颜色是否匹配 (且玩家有颜色)
	if (PlayerColor != EColor::EC_None && PlayerColor == TargetColor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ColorManager: 颜色匹配，正在执行附身..."));

		// --- [!! 隐藏逻辑 !!] ---
		// 1. 告诉 Controller 记住这个即将被隐藏的角色
		//    (通过友元类访问私有成员)
		MageController->HiddenCharacter = CurrentCharacter;

		// 2. 隐藏角色并禁用其功能
		CurrentCharacter->SetActorHiddenInGame(true);      // 在游戏中隐藏
		CurrentCharacter->SetActorEnableCollision(false); // 禁用碰撞
		CurrentCharacter->SetActorTickEnabled(false);     // 禁用 Tick
		// --- [!! 隐藏逻辑结束 !!] ---

		// 解除对当前角色的附身
		Player->UnPossess();
		
		// 附身到目标 Pawn
		Player->Possess(TargetPawn); 
		
		UE_LOG(LogTemp, Log, TEXT("成功附身到 %s"), *TargetPawn->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ColorManager: 附身失败，颜色不匹配或玩家无颜色。玩家颜色: %d, 目标颜色: %d"), (int32)PlayerColor, (int32)TargetColor);
	}
}