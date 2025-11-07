#include "ColorMageGameMode.h"
#include "GameFramework/PlayerStart.h"     // 需要包含 PlayerStart
#include "Kismet/GameplayStatics.h"       // 需要包含 GameplayStatics
#include "ColorMageController.h"         // 需要包含玩家控制器
#include "ColorMageCharacter.h"         // 需要包含玩家角色
#include "ColorMagePlayerState.h"
#include "PossessablePawn.h"            // 需要包含可附身 Pawn
#include "GameFramework/CharacterMovementComponent.h"

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

    AColorMageController* MageController = Cast<AColorMageController>(PlayerController);
    if (!MageController)
    {
       UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: 无法将 Controller 转换为 AColorMageController!"));
       return;
    }

    // 获取玩家掉落时正在控制的 Pawn
    APawn* FallingPawn = MageController->GetPawn();
    // 获取存储在 Controller 中的“原始角色”（如果存在的话）
    AColorMageCharacter* OriginalCharacter = MageController->HiddenCharacter.Get();

    AColorMageCharacter* CharacterToRespawn = nullptr; // 这是我们最终要传送的角色

    // --- [!! 关键逻辑分支 !!] ---

    // 情况 1: 玩家在附身时掉落
    // (OriginalCharacter 有效，并且掉落的 Pawn 不是 OriginalCharacter)
    if (OriginalCharacter && FallingPawn && FallingPawn != OriginalCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("RespawnPlayer: 玩家在附身 %s 时掉落。正在返回角色..."), *FallingPawn->GetName());
        
        // 我们要重生的是“原始角色”
        CharacterToRespawn = OriginalCharacter;

        // 强制控制器重新附身到原始角色
        // Possess 会自动调用 UnPossess 来脱离 FallingPawn
        MageController->Possess(CharacterToRespawn); 
        MageController->HiddenCharacter = nullptr; // 清除引用

        // (可选，但推荐) 销毁掉下去的那个 Pawn
        FallingPawn->Destroy();
    }
    // 情况 2: 玩家角色自己掉落
    // (OriginalCharacter 是空的，并且 FallingPawn 是我们的角色)
    else if (!OriginalCharacter && Cast<AColorMageCharacter>(FallingPawn))
    {
        UE_LOG(LogTemp, Log, TEXT("RespawnPlayer: 玩家角色 %s 掉落。正在传送..."), *FallingPawn->GetName());
        
        // 我们要重生的就是这个掉下去的角色
        CharacterToRespawn = Cast<AColorMageCharacter>(FallingPawn);
    }
    // 情况 3: 发生了意外情况
    // (例如：OriginalCharacter 是空的，掉下去的也不是角色，或者一切都是空的)
    else
    {
        UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: 状态未知! 尝试使用 RestartPlayer 重启..."));
        // (你之前的逻辑)
        RestartPlayer(MageController); 
        CharacterToRespawn = Cast<AColorMageCharacter>(MageController->GetPawn()); // 获取新生成的角色
    }
    
    // --- [!! 执行传送 !!] ---
    if (CharacterToRespawn)
    {
       // [!! 传送 !!] 将角色传送到最后检查点
       CharacterToRespawn->TeleportTo(LastCheckpointTransform.GetLocation(), LastCheckpointTransform.GetRotation().Rotator(), false, true);
       
       // [!! 关键 !!] 重置角色的移动状态
       // (防止在空中冲刺时死亡，重生后继续以高速飞行)
       UCharacterMovementComponent* MoveComp = CharacterToRespawn->GetCharacterMovement();
       if (MoveComp)
       {
           MoveComp->StopMovementImmediately(); // 停止所有速度
           MoveComp->SetMovementMode(MOVE_Walking); // 强制设为行走 (如果重生点在空中，它会自动变为 Falling)
           MoveComp->GravityScale = CharacterToRespawn->DefaultGravityScale; // 恢复重力 (以防在 Dash 中死亡)
       }

       // (可选：重置颜色)
       AColorMagePlayerState* PlayerState = MageController->GetPlayerState<AColorMagePlayerState>();
       if (PlayerState)
       {
           // PlayerState->Server_SetCurrentColor(EColor::EC_None);
       }
       
       UE_LOG(LogTemp, Log, TEXT("RespawnPlayer: 角色 %s 已重生到 Checkpoint。"), *CharacterToRespawn->GetName());
    }
    else
    {
       UE_LOG(LogTemp, Error, TEXT("RespawnPlayer: 重生失败，最终未能获取有效的 Pawn!"));
    }
}