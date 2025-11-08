// HighJumperCreature.cpp
#include "HighJumperCreature.h"

#include "ColorTypes.h"

AHighJumperCreature::AHighJumperCreature()
{
	// [!! 设置这个怪物的控制类型 !!]
	ControlType = EPawnControlType::HighJumper;
    
	// [!! 设置默认数值 !!]
	SuperJumpForce = 3000.0f;
	NormalJumpForce = 1000.0f;
}

void AHighJumperCreature::OnJumpPressed()
{
	EColor MyColor = GetColor();
	UE_LOG(LogTemp, Warning, TEXT("HighJumperCreature %s: 高跳怪兽Jump被触发，颜色为 %d"), *GetName(), (int32)MyColor);

	// [!! 调用蓝图事件 !!]
	BP_OnJumpAbilityTriggered();

	// [!! 高跳怪兽的特殊逻辑 !!]
	switch (MyColor)
	{
	case EColor::EC_Green:
		UE_LOG(LogTemp, Warning, TEXT("HighJumperCreature: 执行【超级高跳】！力度: %f"), SuperJumpForce);
		LaunchCharacter(FVector(0, 0, SuperJumpForce), false, true);
		break;
	case EColor::EC_White:
		UE_LOG(LogTemp, Log, TEXT("HighJumperCreature: 执行【白色中等跳】！"));
		LaunchCharacter(FVector(0, 0, NormalJumpForce), false, true);
		break;
	default:
		UE_LOG(LogTemp, Log, TEXT("HighJumperCreature: 执行普通跳跃"));
		Super::Jump(); // 使用ACharacter的默认跳跃
		break;
	}
}
