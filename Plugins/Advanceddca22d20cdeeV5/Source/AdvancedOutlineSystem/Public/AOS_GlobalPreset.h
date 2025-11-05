// Copyright 2023 Dev Levy. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include <Engine/DataTable.h>

#include "Kismet/KismetSystemLibrary.h"
#include "UObject/Object.h"
#include "AOS_GlobalPreset.generated.h"


UENUM(BlueprintType)
enum class EAOSCheckOption : uint8
{
	AllComps = 0 UMETA(DisplayName="Components Has No Tag"),
	AllCompsWithTag = 1 UMETA(DisplayName="Components Has Tag")
};


USTRUCT(BlueprintType)
struct FAOS_GlobalPreset : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:
	
	FAOS_GlobalPreset()
	:TargetTag(NAME_None),
	HighlightColor(FLinearColor::White)
	{
	}

	FAOS_GlobalPreset(const FName InTargetTag, const FLinearColor InHighlightColor)
	:TargetTag(InTargetTag), HighlightColor(InHighlightColor)
	{
	}


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AOS")
	FName TargetTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="AOS")
	FLinearColor HighlightColor;

};


USTRUCT(BlueprintType)
struct FAOSTraceParam
{
	GENERATED_USTRUCT_BODY()
	
public:
	
	FAOSTraceParam()
	 :
	TargetTraceType(ECC_Visibility),
	TargetCollisionChannel(ECC_Visibility),
	bIsComplex(false),
	bIsDebug(false),
	DebugTraceType(EDrawDebugTrace::None)
	{
	}

	FAOSTraceParam(const ETraceTypeQuery InTraceType, const ECollisionChannel InCollisionChannel, const bool InIsTraceComplex,
		TArray<AActor*> InActorsToIgnore, const bool InIsDebug, const EDrawDebugTrace::Type InDrawDebug)
	 :
	TargetTraceType(InTraceType),
	TargetCollisionChannel(InCollisionChannel),
	bIsComplex(InIsTraceComplex),
	ActorsToIgnore(InActorsToIgnore),
	bIsDebug(InIsDebug),
	DebugTraceType(InDrawDebug)
	{
	}

	UPROPERTY(BlueprintReadWrite, Category="AOS|TraceParam")
	TEnumAsByte<ETraceTypeQuery> TargetTraceType;

	UPROPERTY(BlueprintReadWrite, Category="AOS|TraceParam")
	TEnumAsByte<ECollisionChannel> TargetCollisionChannel;

	UPROPERTY(BlueprintReadWrite, Category="AOS|TraceParam")
	bool bIsComplex;

	UPROPERTY(BlueprintReadWrite, Category="AOS|TraceParam")
	TArray<AActor*> ActorsToIgnore;

	UPROPERTY(BlueprintReadWrite, Category="AOS|TraceParam")
	bool bIsDebug;

	UPROPERTY(BlueprintReadWrite, Category="AOS|TraceParam")
	TEnumAsByte<EDrawDebugTrace::Type> DebugTraceType;
	
};

USTRUCT(BlueprintType)
struct FAOSWorldOutlineInfo
{
	GENERATED_USTRUCT_BODY()

public:

	FAOSWorldOutlineInfo()
	:
	CheckOption(EAOSCheckOption::AllComps),
	TargetTag(NAME_None),
	bEnableOutline(false),
	bIsToggle(false),
	OutlineColor(FLinearColor::Black)
	{}

	FAOSWorldOutlineInfo(const EAOSCheckOption InCheckOption, const FName InTag, const bool InEnableOutline, const bool InIsToggle, const FLinearColor InColor)
	:
	CheckOption(InCheckOption),
	TargetTag(InTag),
	bEnableOutline(InEnableOutline),
	bIsToggle(InIsToggle),
	OutlineColor(InColor)
	{}

	UPROPERTY(BlueprintReadWrite, Category="AOS")
	EAOSCheckOption CheckOption;

	UPROPERTY(BlueprintReadWrite, Category="AOS")
	FName TargetTag;

	UPROPERTY(BlueprintReadWrite, Category="AOS")
	bool bEnableOutline;

	UPROPERTY(BlueprintReadWrite, Category="AOS")
	bool bIsToggle;

	UPROPERTY(BlueprintReadWrite, Category="AOS")
	FLinearColor OutlineColor;
	
	/** Comparison operators */
	FORCEINLINE bool operator==(const FAOSWorldOutlineInfo& AOSInfo) const
	{
		return this->TargetTag == AOSInfo.TargetTag && this->OutlineColor == AOSInfo.OutlineColor && this->bEnableOutline == AOSInfo.bEnableOutline;
	}

	FORCEINLINE bool operator!=(const FAOSWorldOutlineInfo& AOSInfo) const
	{
		return this->TargetTag != AOSInfo.TargetTag || this->OutlineColor != AOSInfo.OutlineColor || this->bEnableOutline != AOSInfo.bEnableOutline;
	}


};