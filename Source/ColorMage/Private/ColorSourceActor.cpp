#include "ColorSourceActor.h"
#include "Components/StaticMeshComponent.h"
#include "ColorComponent.h"
#include "NiagaraComponent.h" // <--- [!! ADDED !!]

AColorSourceActor::AColorSourceActor()
{
	PrimaryActorTick.bCanEverTick = false; // Good for performance

	// Create the MeshComponent first so it can be the Root
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	// Create the ColorComponent
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("ColorComponent"));

	// --- [!! ADDED !!] ---
	// Create the NiagaraComponent
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
    
	// Attach the effect to the mesh.
	// This makes it automatically move and scale WITH the mesh!
	NiagaraComponent->SetupAttachment(MeshComponent); 
	// --- [!! ADDED END !!] ---

	// Apply the scale multiplier
	if (NiagaraComponent)
	{
		// Set the effect's scale *relative* to its parent (the mesh).
		// This combines with the mesh's scale.
		// FVector(EffectScaleMultiplier) applies the float uniformly to X, Y, and Z.
		NiagaraComponent->SetRelativeScale3D(FVector(EffectScaleMultiplier));
	}
}

// --- [!! ADDED !!] ---
/** Called when the game starts */
void AColorSourceActor::BeginPlay()
{
	Super::BeginPlay();

	
}
// --- [!! ADDED END !!] ---

/** Gets the color from the ColorComponent */
EColor AColorSourceActor::GetColor() const
{
	if (ColorComponent)
	{
		return ColorComponent->GetColor();
	}
	return EColor::EC_None;
}