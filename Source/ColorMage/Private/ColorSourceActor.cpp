// Fill out your copyright notice in the Description page of Project Settings.


#include "ColorSourceActor.h"

AColorSourceActor::AColorSourceActor()
{
	PrimaryActorTick.bCanEverTick = false;
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
}
