// ColorProjectile.cpp
#include "ColorProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PossessablePawn.h" // Include the pawn to paint it

AColorProjectile::AColorProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Create Collision Sphere
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AColorProjectile::OnHit);

	// 2. Create Projectile Movement
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->InitialSpeed = 3000.0f;
	ProjectileMovementComponent->MaxSpeed = 3000.0f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;

	// Die after 3 seconds
	InitialLifeSpan = 3.0f;
}

void AColorProjectile::SetProjectileColor(EColor NewColor)
{
	ProjectileColor = NewColor;
	// Call the Blueprint event so it can update its visuals (e.g., particle color)
	OnColorSet(NewColor);
}

void AColorProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Check if we hit a PossessablePawn
	if (APossessablePawn* PawnToPaint = Cast<APossessablePawn>(OtherActor))
	{
		// If so, set its color
		PawnToPaint->SetColor(ProjectileColor);
	}

	// (You could also add a Cast for "PaintableEnemy" or "PaintableWall" here)

	// Destroy the projectile on impact
	Destroy();
}