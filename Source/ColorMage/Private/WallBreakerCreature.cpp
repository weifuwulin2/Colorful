// WallBreakerCreature.cpp
#include "WallBreakerCreature.h"

#include "ColorTypes.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
// #include "BreakableWall.h" // 如果你有破墙Actor的话

AWallBreakerCreature::AWallBreakerCreature()
{
	// [!! 设置这个怪物的控制类型 !!]
	ControlType = EPawnControlType::WallBreaker;
    
	// [!! 设置默认数值 !!]
	BreakWallRange = 500.0f;
	BreakWallDamage = 1000.0f;
}

void AWallBreakerCreature::OnLMBPressed()
{
	EColor MyColor = GetColor();
	UE_LOG(LogTemp, Warning, TEXT("WallBreakerCreature %s: 破墙怪兽LMB被触发，颜色为 %d"), *GetName(), (int32)MyColor);

	// [!! 调用蓝图事件 !!]
	BP_OnSpecialAbilityTriggered();

	// [!! 破墙怪兽的特殊逻辑 !!]
	switch (MyColor)
	{
	case EColor::EC_Red:
		UE_LOG(LogTemp, Warning, TEXT("WallBreakerCreature: 执行【红色破墙攻击】！"));
		PerformWallBreak();
		break;
	case EColor::EC_Yellow:
		UE_LOG(LogTemp, Log, TEXT("WallBreakerCreature: 执行【黄色震击】！"));
		// 可以添加震击效果
		break;
	default:
		UE_LOG(LogTemp, Log, TEXT("WallBreakerCreature: 此颜色没有特殊LMB能力"));
		// 调用父类的默认行为
		Super::OnLMBPressed();
		break;
	}
}

void AWallBreakerCreature::PerformWallBreak()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector StartLocation = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();
    
	// [!! 执行球形重叠检测，查找可破坏的墙 !!]
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
    
	bool bHasOverlaps = World->OverlapMultiByObjectType(
		OverlapResults,
		StartLocation + ForwardVector * (BreakWallRange * 0.5f), // 前方检测
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeSphere(BreakWallRange),
		QueryParams
   );
	if (bHasOverlaps)
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* HitActor = Result.GetActor();
			if (HitActor)
			{
				// [!! 检查是否是可破坏的墙 !!]
				// 方法1：通过Tag检查
				if (HitActor->ActorHasTag(TEXT("BreakableWall")))
				{
					UE_LOG(LogTemp, Warning, TEXT("WallBreakerCreature: 破坏墙壁 %s!"), *HitActor->GetName());
					HitActor->Destroy();
				}
                
				// 方法2：通过类型检查（如果你有BreakableWall类）
				/*
				if (ABreakableWall* Wall = Cast<ABreakableWall>(HitActor))
				{
					Wall->TakeDamage(BreakWallDamage, FDamageEvent(), GetController(), this);
				}
				*/
                
				// 方法3：通过接口检查（如果你有IBreakable接口）
				/*
				if (IBreakable* Breakable = Cast<IBreakable>(HitActor))
				{
					Breakable->Break();
				}
				*/
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("WallBreakerCreature: 范围内没有找到可破坏的墙"));
	}
}