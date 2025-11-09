#include "ColorMageGameMode.h"
#include "GameFramework/PlayerStart.h"     // 需要包含 PlayerStart
#include "Kismet/GameplayStatics.h"       // 需要包含 GameplayStatics
#include "ColorMageController.h"         // 需要包含玩家控制器
#include "ColorMageCharacter.h"         // 需要包含玩家角色
#include "ColorMagePlayerState.h"
#include "CreatureCharacter.h"
#include "NiagaraFunctionLibrary.h"
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
	if (!PlayerController) { return; }
	if (IsPlayerRespawning(PlayerController)) { return; }

	UE_LOG(LogTemp, Warning, TEXT("GameMode: 正在为 %s 启动重生序列..."), *PlayerController->GetName());

	// 1. 标记为“正在重生”
	RespawningPlayers.Add(PlayerController);

	// 2. 禁用玩家输入
	AColorMageController* MageController = Cast<AColorMageController>(PlayerController);
	if (MageController)
	{
		UE_LOG(LogTemp, Warning, TEXT("RespawnPlayer: 禁用玩家 %s 的输入。"), *PlayerController->GetName());
		MageController->DisableInput(MageController);
	}
	
	// (玩家的移动和可见性已在 AColorableActor 中被禁用)

	// 3. 设置 1 秒延迟
	FTimerHandle RespawnTimer;
	float RespawnDelay = 1.0f; // 1秒延迟
	
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUFunction(this, FName("DelayedRespawnLogic"), PlayerController);

	GetWorld()->GetTimerManager().SetTimer(
		RespawnTimer,
		TimerDelegate,
		RespawnDelay,
		false 
	);
}
void AColorMageGameMode::DelayedRespawnLogic(AController* PlayerController)
{
	if (!PlayerController) { return; }

	UE_LOG(LogTemp, Warning, TEXT("DelayedRespawnLogic: 正在为 %s 执行传送..."), *PlayerController->GetName());

	AColorMageController* MageController = Cast<AColorMageController>(PlayerController);
    if (!MageController)
    {
	   RespawningPlayers.Remove(PlayerController); 
       return;
    }

    // --- [!! 1. 识别掉下去的 Pawn !!] ---
    APawn* FallingPawn = MageController->GetPawn(); // 这是掉下去的 Pawn
    AColorMageCharacter* OriginalCharacter = MageController->HiddenCharacter.Get(); // 这是隐藏的法师

    APawn* PawnToTeleport = nullptr; // 我们最终要传送的 Pawn
    AColorMageCharacter* CharacterToRestoreHealth = nullptr; // 我们需要回血的法师 (如果有)

    // --- [!! 2. 决定重生哪个 Pawn !!] ---

    // 情况 A: 玩家角色自己掉落
    if (AColorMageCharacter* MageCharacter = Cast<AColorMageCharacter>(FallingPawn))
    {
        UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: 玩家角色掉落。重生法师..."));
        PawnToTeleport = MageCharacter;
        CharacterToRestoreHealth = MageCharacter;
        // (控制器已附身法师，无需操作)
    }
    // 情况 B: 玩家附身的【怪物】掉落
    else if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(FallingPawn))
    {
        UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: 附身的【怪物】掉落。重生怪物..."));
        PawnToTeleport = Creature; // [!! 关键 !!] 我们要传送的是怪物
        // (控制器保持附身怪物，无需操作)
        // (怪物不需要回血，法师是安全的)
    }
    // 情况 C: 玩家附身的【平台】(或任何其他 Pawn) 掉落
    else if (OriginalCharacter && FallingPawn)
    {
        UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: 附身的【平台】 %s 掉落。重生法师..."), *FallingPawn->GetName());
        
        PawnToTeleport = OriginalCharacter;       // 我们要传送的是法师
        CharacterToRestoreHealth = OriginalCharacter; // 我们要给法师回血
        
        MageController->Possess(OriginalCharacter); // 强制返回法师
        MageController->HiddenCharacter = nullptr;
        if (IsValid(FallingPawn)) { FallingPawn->Destroy(); } // 销毁掉下去的平台
    }
    // 情况 D: 发生意外
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DelayedRespawnLogic: 状态未知! 尝试重启玩家..."));
        RestartPlayer(MageController); 
        PawnToTeleport = MageController->GetPawn();
        CharacterToRestoreHealth = Cast<AColorMageCharacter>(PawnToTeleport);
    }
    
    // --- [!! 3. 执行传送和恢复 !!] ---
    if (PawnToTeleport)
    {
       // a. 传送到检查点
       PawnToTeleport->TeleportTo(LastCheckpointTransform.GetLocation(), LastCheckpointTransform.GetRotation().Rotator(), false, true);
       
	   // b. 播放重生 VFX (在检查点位置)
	   if (RespawnVFX)
	   {
		   UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), RespawnVFX, LastCheckpointTransform.GetLocation(), LastCheckpointTransform.GetRotation().Rotator());
	   }
	   
	   // c. [!! 关键 !!] 确保被传送的 Pawn 是可见的并已启用
       // (这会处理法师的“取消隐藏”)
	   PawnToTeleport->SetActorHiddenInGame(false);
	   PawnToTeleport->SetActorEnableCollision(true);
	   
	   // d. 重置移动组件
       if (ACharacter* CharToReset = Cast<ACharacter>(PawnToTeleport))
       {
           if (UCharacterMovementComponent* MoveComp = CharToReset->GetCharacterMovement())
           {
               MoveComp->StopMovementImmediately(); 
               MoveComp->SetMovementMode(MOVE_Falling); // 设为下落 (如果落地会自动变行走)
               
               // 恢复正确的重力
               if (AColorMageCharacter* Mage = Cast<AColorMageCharacter>(CharToReset))
               {
                   MoveComp->GravityScale = Mage->DefaultGravityScale;
               }
               else
               {
                   MoveComp->GravityScale = 1.0f; // 怪物使用默认重力
               }
           }
       }
       
       // e. [!! 关键 !!] 只在需要时恢复法师的血量
       if (CharacterToRestoreHealth)
       {
           CharacterToRestoreHealth->RestoreFullHealth();
       }

       UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: Pawn %s 已重生。"), *PawnToTeleport->GetName());
    }
    else
    {
       UE_LOG(LogTemp, Error, TEXT("DelayedRespawnLogic: 重生失败，最终未能获取有效的 Pawn!"));
    }
	
	// 4. 恢复玩家输入
	UE_LOG(LogTemp, Warning, TEXT("DelayedRespawnLogic: 恢复玩家 %s 的输入。"), *PlayerController->GetName());
	MageController->EnableInput(MageController);

	// 5. 将玩家从“正在重生”集合中移除
	RespawningPlayers.Remove(PlayerController);
}
bool AColorMageGameMode::IsPlayerRespawning(AController* PlayerController) const
{
	if (!PlayerController) return false;
	return RespawningPlayers.Contains(PlayerController);
}