// ColorMagePlayerState.cpp
#include "ColorMagePlayerState.h"
#include "Net/UnrealNetwork.h" // Needed for DOREPLIFETIME

AColorMagePlayerState::AColorMagePlayerState()
{
	CurrentColor = EColor::EC_None;
}

void AColorMagePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate the CurrentColor variable
	DOREPLIFETIME(AColorMagePlayerState, CurrentColor);
}

// This function executes on the server
void AColorMagePlayerState::Server_SetCurrentColor_Implementation(EColor NewColor)
{
	if (CurrentColor != NewColor)
	{
		CurrentColor = NewColor;
		// Manually call the OnRep function on the server
		OnRep_CurrentColor();
	}
}

void AColorMagePlayerState::Server_EnableColorMixing_Implementation()
{
	if (bCanMixColors == false) // 仅在尚未解锁时执行
		{
		bCanMixColors = true;
		UE_LOG(LogTemp, Warning, TEXT("玩家 %s 已解锁混色能力!"), *GetPlayerName());
		
		// (可选) 你可以在这里也添加一个 RepNotify
		// OnRep_CanMixColors(); 
		// 并广播一个“能力已解锁”的委托，让 UI 播放特效
		}
}

// This function executes on clients when the server updates CurrentColor
void AColorMagePlayerState::OnRep_CurrentColor()
{
	// You can broadcast an event here for UI widgets to update
	OnPlayerColorChanged.Broadcast(CurrentColor);
	UE_LOG(LogTemp, Warning, TEXT("PlayerState Color updated to: %d"), (int32)CurrentColor);
}