#include "ColorManagerSubsystem.h"
#include "ColorMagePlayerState.h"
#include "ColorSourceActor.h"
#include "PossessablePawn.h"
#include "GameFramework/PlayerController.h"
#include "ColorMageCharacter.h"
#include "ColorMageController.h"

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
void UColorManagerSubsystem::AttemptPossession(APlayerController* Player, APossessablePawn* TargetPawn)
{
	AColorMagePlayerState* PlayerState = Player->GetPlayerState<AColorMagePlayerState>();
	AColorMageController* MageController = Cast<AColorMageController>(Player);
	AColorMageCharacter* CurrentCharacter = Cast<AColorMageCharacter>(Player->GetPawn()); 
	if (!PlayerState || !TargetPawn || !MageController || !CurrentCharacter) { return; }

	EColor PlayerColor = PlayerState->GetCurrentColor();
	EColor TargetColor = TargetPawn->GetColor();

	// GDD 规则：颜色必须完全一致
	if (PlayerColor != EColor::EC_None && PlayerColor == TargetColor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ColorManager: 匹配成功，正在执行附身..."));
		MageController->HiddenCharacter = CurrentCharacter;
		CurrentCharacter->SetActorHiddenInGame(true);
		CurrentCharacter->SetActorEnableCollision(false);
		CurrentCharacter->SetActorTickEnabled(false);
		Player->UnPossess();
		Player->Possess(TargetPawn);
	}
	else 
	{ 
		UE_LOG(LogTemp, Warning, TEXT("ColorManager: 附身失败，颜色不匹配。"));
	}
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