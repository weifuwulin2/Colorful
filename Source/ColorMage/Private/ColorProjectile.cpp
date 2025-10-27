// ColorProjectile.cpp
#include "ColorProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PossessablePawn.h" // (或者你的Pawn的基类)
#include "ColorComponent.h" // 包含新组件的头文件

AColorProjectile::AColorProjectile()
{
 	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	// --- [!! 调试日志 !!] ---
	if (!CollisionComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[Projectile Constructor]: CollisionComponent 创建失败!"));
		return; // 如果核心组件失败，就没必要继续了
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Projectile Constructor]: CollisionComponent 创建成功。"));
	}
	// --- [!! 调试结束 !!] ---
	
	SetRootComponent(CollisionComponent);
	
	// 设置碰撞 (我们先在代码里强制设置一次，以防万一)
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic")); 
	CollisionComponent->SetGenerateOverlapEvents(true);

	

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
		ProjectileMovementComponent->InitialSpeed = 3000.0f;
		ProjectileMovementComponent->MaxSpeed = 3000.0f;
		ProjectileMovementComponent->bRotationFollowsVelocity = true;
		ProjectileMovementComponent->bShouldBounce = false;
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f; 
		ProjectileMovementComponent->bIsSliding = false;
		UE_LOG(LogTemp, Log, TEXT("[Projectile Constructor]: ProjectileMovementComponent 创建并配置成功。"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Projectile Constructor]: ProjectileMovementComponent 创建失败!"));
	}

	InitialLifeSpan = 3.0f;

	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));
	if (!ColorComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[Projectile Constructor]: ColorComponent 创建失败!"));
	}
}

// --- [!! 新增：BeginPlay 中的日志 !!] ---
void AColorProjectile::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[Projectile BeginPlay]: %s 已生成!"), *GetName());

	// 绑定事件
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AColorProjectile::OnOverlapBegin);
	if (CollisionComponent)
	{
		// 打印出实际生效的碰撞设置
		FName ProfileName = CollisionComponent->GetCollisionProfileName();
		bool bGeneratesOverlap = CollisionComponent->GetGenerateOverlapEvents();
		UE_LOG(LogTemp, Log, TEXT("[Projectile BeginPlay]: CollisionComponent 配置: Profile='%s', GeneratesOverlap=%s"), 
			*ProfileName.ToString(), 
			bGeneratesOverlap ? TEXT("True") : TEXT("False"));

		// 检查绑定是否还在 (虽然不太可能消失)
		if (!CollisionComponent->OnComponentBeginOverlap.IsBound())
		{
			UE_LOG(LogTemp, Error, TEXT("[Projectile BeginPlay]: 严重错误! OnComponentBeginOverlap 没有绑定!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Projectile BeginPlay]: CollisionComponent 是 NULL!"));
	}

	
}

// --- [!! 已修改 !!] ---
void AColorProjectile::SetProjectileColor(EColor NewColor)
{
	ColorComponent->SetColor(NewColor);
}

// --- [!! 新增 !!] ---
EColor AColorProjectile::GetProjectileColor() const
{
	return ColorComponent->GetColor();
}

void AColorProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// --- [!! 调试 !!] ---
	if (OtherActor) { UE_LOG(LogTemp, Warning, TEXT("[Projectile]: OnOverlapBegin 重叠了: %s"), *OtherActor->GetName()); }
	else { UE_LOG(LogTemp, Error, TEXT("[Projectile]: OnOverlapBegin 重叠了一个无效 Actor!")); Destroy(); return; }
	// --- [!! 调试结束 !!] ---

	// 1. 检查我们是否重叠了有效的Actor (并且不是我们自己)
	if (OtherActor && OtherActor != this)
	{
		// 2. 检查重叠的 Actor 是否是一个 "APossessablePawn"
		APossessablePawn* PossessablePawn = Cast<APossessablePawn>(OtherActor);

		if (PossessablePawn)
		{
			UE_LOG(LogTemp, Log, TEXT("[Projectile]: 成功! 重叠的是 APossessablePawn。正在尝试获取 ColorComponent..."));
            
			UColorComponent* TargetColorComp = PossessablePawn->FindComponentByClass<UColorComponent>();
			if (TargetColorComp)
			{
				UE_LOG(LogTemp, Log, TEXT("[Projectile]: 成功! 找到了 ColorComponent。正在调用 SetColor(%d)..."), (int32)GetProjectileColor());
				TargetColorComp->SetColor(GetProjectileColor());

				// [!! 重要 !!] 既然我们上色成功了，就应该销毁投射物
				// 否则它会继续飞行并给后面的东西也上色
				Destroy(); 
				return; // 提前退出函数
			}
			else { UE_LOG(LogTemp, Error, TEXT("[Projectile]: 失败! 重叠的 Pawn 没有 UColorComponent!")); }
		}
		else { UE_LOG(LogTemp, Warning, TEXT("[Projectile]: OnOverlapBegin 重叠的 %s 不是 APossessablePawn。忽略上色。"), *OtherActor->GetName()); }
        
		// (你也可以在这里添加对其他类型Actor的检查，比如墙壁，如果希望子弹碰到墙就消失)
		// if (Cast<AStaticMeshActor>(OtherActor)) { Destroy(); return; }
	}
    
	// 如果上面的逻辑没有销毁它 (比如它穿过了Pawn但没找到组件)，
	// 我们可能不希望它在这里销毁，让它继续飞行直到 lifespan 结束
	// Destroy(); // <--- 移除这行，或者根据你的设计决定
}