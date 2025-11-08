#include "ColorManagerSubsystem.h"
#include "ColorMagePlayerState.h"
#include "ColorSourceActor.h"
#include "PossessablePawn.h"
#include "GameFramework/PlayerController.h"
#include "ColorMageCharacter.h"
#include "ColorMageController.h"
#include "CreatureCharacter.h"

/** (RMB) 处理汲取和混合逻辑 */
void UColorManagerSubsystem::HandleAcquireColor(APlayerController* Player, AColorSourceActor* ColorSource)
{
	if (!Player || !ColorSource) return;
	AColorMagePlayerState* PlayerState = Player->GetPlayerState<AColorMagePlayerState>();
	if (!PlayerState) return;

	EColor PlayerColor = PlayerState->GetCurrentColor();
	EColor SourceColor = ColorSource->GetColorToProvide(); 

	EColor NewColor;

	// --- [!! 关键检查 !!] ---
	// 检查玩家是否已解锁混色能力
	if (PlayerState->CanMixColors())
	{
		// 1. (已解锁) 玩家在第二关 -> 执行复杂的混合逻辑
		NewColor = GetMixedColor(PlayerColor, SourceColor);
	}
	else
	{
		// 2. (未解锁) 玩家在第一关 -> 逻辑降级为“直接覆盖”
		NewColor = SourceColor;
	}
	// --- [!! 检查结束 !!] ---

	// 3. 设置新颜色
	PlayerState->Server_SetCurrentColor(NewColor);
	
	UE_LOG(LogTemp, Log, TEXT("颜色汲取: 玩家 %s + 颜色源 %s = 玩家变为 %s (混色能力: %s)"), 
		*UEnum::GetValueAsString(PlayerColor), 
		*UEnum::GetValueAsString(SourceColor), 
		*UEnum::GetValueAsString(NewColor),
		PlayerState->CanMixColors() ? TEXT("已启用") : TEXT("已禁用")
	);
}

/** (F) 尝试附身 (保持不变) */
void UColorManagerSubsystem::AttemptPossession(APlayerController* Player,  APawn* TargetPawn)
{
	if (!Player || !TargetPawn)
    {
        UE_LOG(LogTemp, Error, TEXT("AttemptPossession: Player 或 TargetPawn 为空"));
        return;
    }
    AColorMagePlayerState* PS = Player->GetPlayerState<AColorMagePlayerState>();
    if (!PS)
    {
        UE_LOG(LogTemp, Error, TEXT("AttemptPossession: 找不到 ColorMagePlayerState"));
        return;
    }
    EColor PlayerColor = PS->GetCurrentColor();
    if (PlayerColor == EColor::EC_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptPossession: 玩家当前没有颜色，无法附身"));
        return;
    }
    // [!! 修改：支持两种类型的Pawn !!]
    EColor TargetColor = EColor::EC_None;
    bool bCanBePossessed = false;
    // 检查是否为APossessablePawn
    if (APossessablePawn* PossessablePawn = Cast<APossessablePawn>(TargetPawn))
    {
        TargetColor = PossessablePawn->GetColor();
        bCanBePossessed = PossessablePawn->bCanBePossessed;
        UE_LOG(LogTemp, Log, TEXT("检测到APossessablePawn: %s"), *PossessablePawn->GetName());
    }
    // 检查是否为ACreatureCharacter
    else if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(TargetPawn))
    {
        TargetColor = Creature->GetColor();
        bCanBePossessed = Creature->CanBePossessed();
        UE_LOG(LogTemp, Log, TEXT("检测到ACreatureCharacter: %s"), *Creature->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AttemptPossession: 目标不是可附身的类型"));
        return;
    }
    // 检查颜色匹配
    if (PlayerColor != TargetColor)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptPossession: 颜色不匹配。玩家: %d, 目标: %d"), 
            (int32)PlayerColor, (int32)TargetColor);
        return;
    }
    // 检查是否可以被附身
    if (!bCanBePossessed)
    {
        UE_LOG(LogTemp, Warning, TEXT("AttemptPossession: 目标当前不能被附身"));
        return;
    }
    // 执行附身
    UE_LOG(LogTemp, Warning, TEXT("AttemptPossession: 开始附身 %s"), *TargetPawn->GetName());
    AColorMageController* ColorMageController = Cast<AColorMageController>(Player);
    if (!ColorMageController)
    {
        UE_LOG(LogTemp, Error, TEXT("AttemptPossession: Player 不是 ColorMageController"));
        return;
    }
    // 保存当前角色
    APawn* CurrentPawn = Player->GetPawn();
    if (AColorMageCharacter* ColorMageChar = Cast<AColorMageCharacter>(CurrentPawn))
    {
        ColorMageController->HiddenCharacter = ColorMageChar;
        ColorMageChar->SetActorHiddenInGame(true);
        ColorMageChar->SetActorEnableCollision(false);
    }
    // 播放特效
    if (APossessablePawn* PossessablePawn = Cast<APossessablePawn>(TargetPawn))
    {
        PossessablePawn->PlayPossessEffect();
    }
    else if (ACreatureCharacter* Creature = Cast<ACreatureCharacter>(TargetPawn))
    {
        Creature->PlayPossessEffect();
    }
    // 执行附身
    Player->Possess(TargetPawn);
    UE_LOG(LogTemp, Warning, TEXT("AttemptPossession: 附身成功！"));
}


// --- [!! GDD 修正：混合规则实现 !!] ---

/** 检查颜色是否为基础元素色 (红, 黄, 蓝) */
bool UColorManagerSubsystem::IsPrimaryElement(EColor Color) const
{
	return Color == EColor::EC_Red || Color == EColor::EC_Yellow || Color == EColor::EC_Blue;
}

/** 检查颜色是否为调节色 (白, 黑) */
bool UColorManagerSubsystem::IsModifier(EColor Color) const
{
	return Color == EColor::EC_White || Color == EColor::EC_Black;
}

/** 检查颜色是否为 *任何* 类型的混合色 */
bool UColorManagerSubsystem::IsMixedColor(EColor Color) const
{
	// 如果它既不是基础色，也不是调节色，也不是 None，那它一定是混合色
	return Color != EColor::EC_None && !IsPrimaryElement(Color) && !IsModifier(Color);
}

/**
 * 根据 GDD 规则计算新颜色
 */
EColor UColorManagerSubsystem::GetMixedColor(EColor PlayerCurrentColor, EColor SourceColor) const
{
	// 规则 1: 重置规则 (手中是混合色 -> 覆盖)
	if (IsMixedColor(PlayerCurrentColor))
	{
		return SourceColor;
	}
	
	// 规则 2: GDD 规则 (元素色 + 调节色)
	if (IsPrimaryElement(PlayerCurrentColor) && IsModifier(SourceColor))
	{
		if (PlayerCurrentColor == EColor::EC_Red)
		{
			if (SourceColor == EColor::EC_White) return EColor::EC_LightRed;
			if (SourceColor == EColor::EC_Black) return EColor::EC_DarkRed;
		}
		if (PlayerCurrentColor == EColor::EC_Yellow)
		{
			if (SourceColor == EColor::EC_White) return EColor::EC_LightYellow;
			if (SourceColor == EColor::EC_Black) return EColor::EC_DarkYellow;
		}
		if (PlayerCurrentColor == EColor::EC_Blue)
		{
			if (SourceColor == EColor::EC_White) return EColor::EC_LightBlue;
			if (SourceColor == EColor::EC_Black) return EColor::EC_DarkBlue;
		}
	}
	
	// 规则 3: GDD 规则 (元素色 + 元素色)
	if (IsPrimaryElement(PlayerCurrentColor) && IsPrimaryElement(SourceColor))
	{
		if ((PlayerCurrentColor == EColor::EC_Red && SourceColor == EColor::EC_Yellow) || (PlayerCurrentColor == EColor::EC_Yellow && SourceColor == EColor::EC_Red))
			return EColor::EC_Orange;
		if ((PlayerCurrentColor == EColor::EC_Yellow && SourceColor == EColor::EC_Blue) || (PlayerCurrentColor == EColor::EC_Blue && SourceColor == EColor::EC_Yellow))
			return EColor::EC_Green;
		if ((PlayerCurrentColor == EColor::EC_Red && SourceColor == EColor::EC_Blue) || (PlayerCurrentColor == EColor::EC_Blue && SourceColor == EColor::EC_Red))
			return EColor::EC_Purple;
	}
	
	// 规则 4: GDD 规则 (调节色 + 调节色)
	if (IsModifier(PlayerCurrentColor) && IsModifier(SourceColor))
	{
		if ((PlayerCurrentColor == EColor::EC_White && SourceColor == EColor::EC_Black) || (PlayerCurrentColor == EColor::EC_Black && SourceColor == EColor::EC_White))
			return EColor::EC_Grey_Neutral;
	}
	
	// 规则 5: GDD 规则 (调节色 + 元素色 -> 覆盖)
	if (IsModifier(PlayerCurrentColor) && IsPrimaryElement(SourceColor))
	{
		return SourceColor; 
	}
	
	// 规则 6: 默认覆盖
	// (例如：手中是空 + 汲取任何颜色 -> 覆盖)
	// (例如：手中颜色与汲取颜色相同 -> 覆盖，即刷新)
	return SourceColor;
}