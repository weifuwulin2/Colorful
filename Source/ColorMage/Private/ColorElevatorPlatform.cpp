#include "ColorElevatorPlatform.h"
#include "ColorComponent.h"
#include "Components/StaticMeshComponent.h" // 虽然不由它移动，但 ColorComponent 可能需要它

AColorElevatorPlatform::AColorElevatorPlatform()
{
	// [!! 关键 !!] 确保 Tick 被启用
	PrimaryActorTick.bCanEverTick = true;
	
	// MeshComponent 和 ColorComponent 已经在父类 AColorableActor 中创建好了
}

void AColorElevatorPlatform::BeginPlay()
{
	Super::BeginPlay();

	// 1. [!! 修复 !!] 存储 Actor 的起始位置
	HomeLocation = GetActorLocation();
	TargetLocation = HomeLocation; 
	bIsMovingAutomatically = false;

	// 2. 绑定 ColorComponent 的委托
	if (ColorComponent)
	{
		ColorComponent->OnColorChanged.AddDynamic(this, &AColorElevatorPlatform::HandleColorChange);
		UE_LOG(LogTemp, Log, TEXT("ColorElevatorPlatform %s: ColorComponent 委托绑定成功。"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ColorElevatorPlatform %s 找不到继承的 ColorComponent!"), *GetName());
	}
}

/** 当 ColorComponent 颜色改变时调用的函数 */
void AColorElevatorPlatform::HandleColorChange(EColor NewColor, EColor OldColor)
{
	// --- [!! 调试日志 !!] ---
	UE_LOG(LogTemp, Warning, TEXT("ColorElevatorPlatform %s: HandleColorChange 被调用! 新颜色: %d"), *GetName(), (int32)NewColor);
	// --- [!! 调试结束 !!] ---

	if (NewColor == EColor::EC_White)
	{
		TargetLocation = HomeLocation + FVector(0.f, 0.f, MoveDistance); // 上升
		bIsMovingAutomatically = true;
	}
	else if (NewColor == EColor::EC_Black)
	{
		TargetLocation = HomeLocation - FVector(0.f, 0.f, MoveDistance); // 下降
		bIsMovingAutomatically = true;
	}
	else
	{
		// 任何其他颜色 (包括灰色) 都让平台返回原点
		TargetLocation = HomeLocation;
		bIsMovingAutomatically = true;
	}
}

/** 每帧调用 - 用于平滑移动 */
void AColorElevatorPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 如果我们被告知要移动...
	if (bIsMovingAutomatically)
	{
		// [!! 修复 !!] 获取并移动 Actor 的位置
		FVector CurrentLocation = GetActorLocation();
		
		if (CurrentLocation.Equals(TargetLocation, 1.0f))
		{
			// 已经到达
			if (bIsMovingAutomatically) // 仅在第一次到达时打印
			{
				UE_LOG(LogTemp, Log, TEXT("Platform %s 已到达目标 %s"), *GetName(), *TargetLocation.ToString());
				bIsMovingAutomatically = false;
				SetActorLocation(TargetLocation, false); // 精确停在目标点
			}
		}
		else
		{
			// 使用 VInterpTo (插值) 来平滑移动
			FVector NewLocation = FMath::VInterpTo(
				CurrentLocation,
				TargetLocation,
				DeltaTime,
				MoveSpeed
			);
			
			// [!! 修复 !!] 移动整个 Actor (Sweep=true 用于碰撞)
			SetActorLocation(NewLocation, true);
		}
	}
}