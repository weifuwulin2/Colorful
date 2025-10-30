// Fill out your copyright notice in the Description page of Project Settings.


#include "ColorReactorActor.h"

AColorReactorActor::AColorReactorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootSceneComponent);
}
