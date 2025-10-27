#include "ColorMageGameMode.h"
#include "GameFramework/PlayerStart.h"     // 需要包含 PlayerStart
#include "Kismet/GameplayStatics.h"       // 需要包含 GameplayStatics
#include "ColorMageController.h"         // 需要包含玩家控制器
#include "ColorMageCharacter.h"         // 需要包含玩家角色
#include "ColorMagePlayerState.h"
#include "PossessablePawn.h"            // 需要包含可附身 Pawn

AColorMageGameMode::AColorMageGameMode()
{
	// 设置默认的 Pawn, Controller, PlayerState (如果你的蓝图继承这个 C++ 类)
	// DefaultPawnClass = AColorMageCharacter::StaticClass();
	// PlayerControllerClass = AColorMageController::StaticClass();
	// PlayerStateClass = AColorMagePlayerState::StaticClass();
}

void AColorMageGameMode::BeginPlay()
{
	Super::BeginPlay();
	// 使用 GameModeBase 的 FindPlayerStart 函数
	AActor* PlayerStartActor = FindPlayerStart(nullptr);
	if (PlayerStartActor)
	{
		LastCheckpointTransform = PlayerStartActor->GetActorTransform();
		UE_LOG(LogTemp, Log, TEXT("GameMode: 初始重生点设置为 PlayerStart。"));
	}
	else
	{
		// 如果找不到 PlayerStart，给一个默认值并警告
		LastCheckpointTransform = FTransform::Identity; // 世界原点
		UE_LOG(LogTemp, Warning, TEXT("GameMode: 未找到 PlayerStart! 初始重生点设为世界原点。"));
	}
}


void AColorMageGameMode::UpdateCheckpoint(const FTransform& NewCheckpointTransform)
{
	LastCheckpointTransform = NewCheckpointTransform;
	UE_LOG(LogTemp, Log, TEXT("GameMode: Checkpoint 更新!"));
}


void AColorMageGameMode::RespawnPlayer(AController* PlayerController)
{
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: PlayerController 无效!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("GameMode: 正在为 %s 执行重生..."), *PlayerController->GetName());

	// 尝试将 Controller 转换为我们的特定类型
	AColorMageController* MageController = Cast<AColorMageController>(PlayerController);
	if (!MageController)
	{
		UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: 无法将 Controller 转换为 AColorMageController!"));
		// 备用方案：尝试直接重启玩家？
		// RestartPlayer(PlayerController);
		return;
	}

	// --- [!! 处理附身状态 !!] ---
	// 检查玩家当前是否附身在某个 Pawn 上 (而不是原始角色)
	APawn* CurrentPawn = MageController->GetPawn();
	AColorMageCharacter* OriginalCharacter = MageController->HiddenCharacter.Get(); // 获取隐藏的角色引用

	if (CurrentPawn && OriginalCharacter && CurrentPawn != OriginalCharacter)
	{
		UE_LOG(LogTemp, Log, TEXT("RespawnPlayer: 玩家当前附身在 %s。正在解除附身..."), *CurrentPawn->GetName());
		// 如果玩家正附身在某个 Pawn (比如平台) 上，先解除附身
		// 注意：我们直接调用 Possess，它会自动处理 UnPossess
		MageController->Possess(OriginalCharacter); 
		// Possess 会调用 OriginalCharacter->PossessedBy() 来取消隐藏
		MageController->HiddenCharacter = nullptr; // 清除引用
	}
	else if (!CurrentPawn && OriginalCharacter)
	{
		// 如果由于某种原因控制器没有附身任何东西，但我们有隐藏角色，尝试附身
		UE_LOG(LogTemp, Log, TEXT("RespawnPlayer: 控制器未附身，但有隐藏角色。正在尝试附身..."));
		MageController->Possess(OriginalCharacter);
		MageController->HiddenCharacter = nullptr;
	}
	else if (!OriginalCharacter)
	{
		// 如果连原始角色都没有了 (理论上不应该发生)，可能需要重新生成一个
		UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: 找不到原始角色引用! 可能需要重新生成角色。"));
		// 这里可以调用 RestartPlayer(MageController); 来尝试重新生成默认 Pawn
		RestartPlayer(MageController); // 尝试使用 GameMode 的标准重生逻辑
		CurrentPawn = MageController->GetPawn(); // 获取新生成的 Pawn
	}
	
	// 现在确保我们有一个有效的 Pawn 来传送
	CurrentPawn = MageController->GetPawn(); // 再次获取 (可能是原始角色或新生成的)
	if (CurrentPawn)
	{
		// [!! 传送 !!] 将玩家的 Pawn (现在应该是 ColorMageCharacter) 传送到最后检查点
		CurrentPawn->TeleportTo(LastCheckpointTransform.GetLocation(), LastCheckpointTransform.GetRotation().Rotator(), false, true);
		
		// 可选：重置玩家状态 (比如颜色、生命值等)
		AColorMagePlayerState* PlayerState = MageController->GetPlayerState<AColorMagePlayerState>();
		if (PlayerState)
		{
			// PlayerState->Server_SetCurrentColor(EColor::EC_None); // 例如，重生后清除颜色
		}
		
		UE_LOG(LogTemp, Log, TEXT("RespawnPlayer: 玩家已重生到 Checkpoint。"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: 重生失败，最终未能获取有效的 Pawn!"));
	}
}