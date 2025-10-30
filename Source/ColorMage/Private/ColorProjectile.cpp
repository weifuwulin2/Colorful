#include "ColorProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ColorComponent.h"

AColorProjectile::AColorProjectile()
{
 	PrimaryActorTick.bCanEverTick = false;
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	if (!CollisionComponent) { return; }
	SetRootComponent(CollisionComponent);
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
	}
	InitialLifeSpan = 3.0f;
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));
}

void AColorProjectile::BeginPlay()
{
    Super::BeginPlay();
    if (CollisionComponent)
    {
        CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AColorProjectile::OnOverlapBegin);
    }
}

void AColorProjectile::SetProjectileColor(EColor NewColor)
{
	if (ColorComponent) { ColorComponent->SetColor(NewColor); }
}

EColor AColorProjectile::GetProjectileColor() const
{
	return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None;
}

// --- [!! GDD 修正：覆盖规则 !!] ---
void AColorProjectile::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 忽略自己和发射者
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
    {
        return;
    }

	// 尝试获取目标的颜色组件
    UColorComponent* TargetColorComp = OtherActor->FindComponentByClass<UColorComponent>();

    // 检查目标是否是“可上色的”(即，它有关心颜色的组件)
    if (TargetColorComp)
    {
		UE_LOG(LogTemp, Log, TEXT("[Projectile]: 击中 ColorableActor %s。正在覆盖颜色..."), *OtherActor->GetName());
        
		// [!! 绝对覆盖 !!]
        // 无视目标当前颜色，强制将其设置为投射物的颜色
        TargetColorComp->SetColor(GetProjectileColor());

        Destroy(); // 成功上色，销毁
        return;
    }
	
	// 如果击中了没有 ColorComponent 的东西 (比如墙壁)，也销毁
	Destroy();
}