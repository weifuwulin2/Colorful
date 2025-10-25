#include "ColorMageCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"

AColorMageCharacter::AColorMageCharacter()
{
	// 构造函数...
	DefaultGravityScale = 1.0f; // 先设个默认值
}

void AColorMageCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 在游戏开始时，获取并存储角色移动组件的默认重力
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		DefaultGravityScale = MoveComp->GravityScale;
	}
}

void AColorMageCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 确保我们使用的是 Enhanced Input Component
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// --- 绑定跳跃 (Space) ---
		if (JumpAction)
		{
			// ACharacter 自带了 Jump() 和 StopJumping() 函数
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		// --- 绑定冲刺 (Shift) ---
		if (DashAction)
		{
			// 当按键“按下” (Started) 时，触发 OnDash 函数
			EnhancedInputComp->BindAction(DashAction, ETriggerEvent::Started, this, &AColorMageCharacter::OnDash);
		}
	}
}

/**
 * 执行冲刺的核心逻辑
 */
void AColorMageCharacter::OnDash()
{
	// --- 1. 安全检查 ---
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// 检查：是否已经在冲刺中？
	if (MoveComp->GravityScale != DefaultGravityScale)
	{
		return;
	}
	
	// --- 2. 播放动画 ---
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && DashMontage)
	{
		AnimInstance->Montage_Play(DashMontage);
	}
	
	// --- 3. 计算冲刺速度 ---
	const float DashSpeed = DashDistance / DashDuration;
	const FVector ForwardDir = GetActorForwardVector();
	const FVector DashVelocity = ForwardDir * DashSpeed;

	// --- 4. 执行冲刺 (核心) ---
	
	// a. 【修正】暂时关闭重力，直接设置属性
	MoveComp->GravityScale = 0.0f;

	// b. 使用 LaunchCharacter 给予一个瞬间的爆发速度
	LaunchCharacter(DashVelocity, true, true);

	// --- 5. 设置计时器，在 DashDuration 结束后恢复一切 ---
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_DashFinished);
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_DashFinished,
		this,
		&AColorMageCharacter::OnDashFinished,
		DashDuration,
		false
	);
}

/**
 * 冲刺结束时调用的恢复函数
 */
void AColorMageCharacter::OnDashFinished()
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// 1. 【修正】恢复重力，直接设置属性
	MoveComp->GravityScale = DefaultGravityScale;

	// 2. (可选) 立即停止当前的冲刺速度
	MoveComp->StopMovementImmediately();
}