#include "ColorMageGameMode.h"
#include "GameFramework/PlayerStart.h"     // 需要包含 PlayerStart
#include "Kismet/GameplayStatics.h"       // 需要包含 GameplayStatics
#include "ColorMageController.h"         // 需要包含玩家控制器
#include "ColorMageCharacter.h"         // 需要包含玩家角色
#include "ColorMagePlayerState.h"
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
	   RespawningPlayers.Remove(PlayerController); // 清理集合
       return;
    }

    // --- (这是你之前的 RespawnPlayer 逻辑) ---
    APawn* CurrentPawn = MageController->GetPawn();
    AColorMageCharacter* OriginalCharacter = MageController->HiddenCharacter.Get();
	AColorMageCharacter* CharacterToRespawn = nullptr; 

	// 情况 1: 玩家在附身时掉落 (或死亡)
    if (OriginalCharacter && CurrentPawn && CurrentPawn != OriginalCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: 玩家附身在 %s。正在返回角色..."), *CurrentPawn->GetName());
        CharacterToRespawn = OriginalCharacter;
        MageController->Possess(CharacterToRespawn); // 这会触发 Character->PossessedBy()
        MageController->HiddenCharacter = nullptr; 
        if(IsValid(CurrentPawn)) { CurrentPawn->Destroy(); }
    }
	// 情况 2: 玩家角色自己掉落 (或死亡)
    else if (!OriginalCharacter && Cast<AColorMageCharacter>(CurrentPawn))
    {
        UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: 玩家角色 %s 掉落。正在传送..."), *CurrentPawn->GetName());
        CharacterToRespawn = Cast<AColorMageCharacter>(CurrentPawn);
    }
	// 情况 3: 发生了意外情况
    else { /*... (你的 RestartPlayer 逻辑) ...*/ }
    
    // --- [!! 执行传送和视觉效果 !!] ---
    if (CharacterToRespawn)
    {
       // 1. 传送到检查点
       CharacterToRespawn->TeleportTo(LastCheckpointTransform.GetLocation(), LastCheckpointTransform.GetRotation().Rotator(), false, true);
       
	   // 2. [!! 新增 !!] 播放重生 VFX
	   if (RespawnVFX)
	   {
		   UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			   GetWorld(),
			   RespawnVFX,
			   LastCheckpointTransform.GetLocation(), // 在重生点播放
			   LastCheckpointTransform.GetRotation().Rotator()
		   );
	   }
	   
	   // 3. [!! 新增 !!] 让角色重新可见并启用碰撞
	   CharacterToRespawn->SetActorHiddenInGame(false);
	   CharacterToRespawn->SetActorEnableCollision(true);
	   
	   // 4. 重置移动组件
       UCharacterMovementComponent* MoveComp = CharacterToRespawn->GetCharacterMovement();
       if (MoveComp)
       {
           MoveComp->StopMovementImmediately(); 
           MoveComp->SetDefaultMovementMode(); // 恢复为 Walking 或 Falling
           MoveComp->GravityScale = CharacterToRespawn->DefaultGravityScale; 
       }

       // (可选：重置颜色)
       // ... (PlayerState->Server_SetCurrentColor(EColor::EC_None)) ...
       
       UE_LOG(LogTemp, Log, TEXT("DelayedRespawnLogic: 角色 %s 已重生。"), *CharacterToRespawn->GetName());
    }
    else { /*... (Log Error) ...*/ }
	
	// 5. 恢复玩家输入
	UE_LOG(LogTemp, Warning, TEXT("DelayedRespawnLogic: 恢复玩家 %s 的输入。"), *PlayerController->GetName());
	MageController->EnableInput(MageController);

	// 6. 将玩家从“正在重生”集合中移除
	RespawningPlayers.Remove(PlayerController);
}
bool AColorMageGameMode::IsPlayerRespawning(AController* PlayerController) const
{
	if (!PlayerController) return false;
	return RespawningPlayers.Contains(PlayerController);
}