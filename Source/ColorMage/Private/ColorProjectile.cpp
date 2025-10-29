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
	if (ColorComponent) { ColorComponent->SetColor(NewColor); }
}

// --- [!! 新增 !!] ---
EColor AColorProjectile::GetProjectileColor() const
{
	return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None;
}

void AColorProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Basic checks: Ignore invalid actors and self-overlaps
	if (!OtherActor || OtherActor == this)
    {
        return; // Do nothing if overlapping self or nothing
    }

    // --- [!! NEW CHECK: Ignore Owner/Instigator !!] ---
    // GetOwner() usually returns the Character who spawned the projectile.
    // GetInstigator() is another way to track who fired it. Checking both is safest.
    if (OtherActor == GetOwner() || OtherActor == GetInstigator())
    {
        UE_LOG(LogTemp, Log, TEXT("[Projectile]: Overlap with owner/instigator (%s). Ignoring."), *OtherActor->GetName());
        return; // Do nothing, DO NOT destroy
    }
    // --- [!! END NEW CHECK !!] ---

    // If we've reached here, we overlapped something valid *other* than the player.

    UE_LOG(LogTemp, Warning, TEXT("[Projectile]: Overlap with: %s"), *OtherActor->GetName());

    // Attempt to get the target's ColorComponent
    UColorComponent* TargetColorComp = OtherActor->FindComponentByClass<UColorComponent>();

    // Does the target have a ColorComponent?
    if (TargetColorComp)
    {
        EColor TargetCurrentColor = TargetColorComp->GetColor();
        EColor ProjColor = GetProjectileColor();

        // --- Painting Rule ---
        // Can we paint it? Only if the target is Grey (EC_None)
        // AND the projectile is NOT Grey
        if (TargetCurrentColor == EColor::EC_None && ProjColor != EColor::EC_None)
        {
            UE_LOG(LogTemp, Log, TEXT("[Projectile]: Target %s is Grey. Painting with %d."), *OtherActor->GetName(), (int32)ProjColor);
            TargetColorComp->SetColor(ProjColor);
            Destroy(); // Destroy projectile after successful paint
            return;
        }
        else // Target already has color, or projectile is grey
        {
             if (TargetCurrentColor != EColor::EC_None)
             {
                 UE_LOG(LogTemp, Log, TEXT("[Projectile]: Target %s already has color %d. Cannot paint over."), *OtherActor->GetName(), (int32)TargetCurrentColor);
             }
             else // ProjColor == EColor::EC_None
             {
                 UE_LOG(LogTemp, Log, TEXT("[Projectile]: Grey projectile hit Grey target %s. No effect."), *OtherActor->GetName());
             }
             Destroy(); // Destroy projectile on ineffective hit on a colorable target
             return;
        }
    }
    else
    {
        // Target does not have a ColorComponent (e.g., wall, floor)
        UE_LOG(LogTemp, Log, TEXT("[Projectile]: Target %s does not have a ColorComponent. Destroying projectile."), *OtherActor->GetName());
        Destroy(); // Destroy projectile upon hitting non-colorable surface
        return;
    }

    // Fallback destroy just in case (shouldn't be reached)
    // Destroy();
}