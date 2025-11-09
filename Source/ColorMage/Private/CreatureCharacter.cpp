#include "CreatureCharacter.h"
#include "AIController.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h" // ACharacter 默认的根组件
#include "ColorComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "ColorMageController.h"
#include "ColorMageGameMode.h"
#include "HiddenPathActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Navigation/PathFollowingComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "RevealableInterface.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"



ACreatureCharacter::ACreatureCharacter()
{
	// 怪物需要 Tick 来执行 AI
	PrimaryActorTick.bCanEverTick = true;
	
	// [!! 关键 !!] 这是“主”颜色组件
	ColorComponent = CreateDefaultSubobject<UColorComponent>(TEXT("MainColorComponent"));

	CharacterExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterExitPoint"));
	CharacterExitPoint->SetupAttachment(GetCapsuleComponent());
	CharacterExitPoint->SetRelativeLocation(FVector(0.f, 0.f, 100.f));

	bCanBePossessed = false; // [!! 关键 !!] 默认不可附身，直到颜色统一
	ControlType = EPawnControlType::Unknown; 

	// --- [!! 复制自 AColorableActor !!] ---
	LightVolume = CreateDefaultSubobject<USphereComponent>(TEXT("LightVolume"));
	LightVolume->SetupAttachment(RootComponent);
	LightVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 默认关闭

	// [!! 已移除 !!]
	// PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	// PointLight->SetupAttachment(RootComponent);
	// PointLight->SetVisibility(false);

}

void ACreatureCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACreatureCharacter, CurrentState); // 复制状态
}

void ACreatureCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitMaterials();
	CheckForColorUnity(); // 检查初始颜色
	
	// 同步光照半径
	LightVolume->SetSphereRadius(LightRadius);
}
void ACreatureCharacter::InitMaterials()
{
    USkeletalMeshComponent* SkeletalMesh = GetMesh();
    if (!SkeletalMesh) return;
    DynamicMaterials.Empty();
    
    int32 MaterialCount = SkeletalMesh->GetNumMaterials();
    for (int32 i = 0; i < MaterialCount; i++)
    {
        UMaterialInstanceDynamic* DynMat = SkeletalMesh->CreateDynamicMaterialInstance(i);
        if (DynMat)
        {
            // 设置初始灰色
            DynMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor::Gray);
            DynamicMaterials.Add(DynMat);
        }
        else
        {
            DynamicMaterials.Add(nullptr);
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Creature %s: 初始化了 %d 个动态材质"), *GetName(), DynamicMaterials.Num());
}
void ACreatureCharacter::HitByColorProjectile(EColor IncomingColor, FVector HitLocation)
{
    UE_LOG(LogTemp, Warning, TEXT("Creature %s: 被颜色 %d 击中，位置: %s"), 
        *GetName(), (int32)IncomingColor, *HitLocation.ToString());
    // 根据击中位置确定身体部位
    int32 PartIndex = GetBodyPartIndexFromHitLocation(HitLocation);
    
    if (PartIndex >= 0)
    {
        UpdatePartColor(PartIndex, IncomingColor);
    }
}
int32 ACreatureCharacter::GetBodyPartIndexFromHitLocation(FVector HitLocation)
{
    USkeletalMeshComponent* SkeletalMesh = GetMesh();
    if (!SkeletalMesh) return -1;
    float ClosestDistance = FLT_MAX;
    int32 ClosestPartIndex = -1;
    // 找到最近的骨骼对应的部位
    for (int32 i = 0; i < BodyParts.Num(); i++)
    {
        FVector BoneLocation = SkeletalMesh->GetBoneLocation(BodyParts[i].BoneName);
        float Distance = FVector::Dist(HitLocation, BoneLocation);
        
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestPartIndex = i;
        }
    }
    if (ClosestPartIndex >= 0)
    {
        UE_LOG(LogTemp, Log, TEXT("击中部位: %s (距离: %f)"), 
            *BodyParts[ClosestPartIndex].PartName, ClosestDistance);
    }
    return ClosestPartIndex;
}

FLinearColor ACreatureCharacter::GetLinearColorFromEnum(EColor InColor) const
{
	// 在颜色映射表中查找
	for (const FColorMapping& Mapping : ColorMappings)
	{
		if (Mapping.ColorEnum == InColor)
		{
			return Mapping.LinearColor;
		}
	}
    
	// 如果没找到，返回灰色
	UE_LOG(LogTemp, Warning, TEXT("找不到颜色 %d 的映射，使用灰色"), (int32)InColor);
	return FLinearColor::Gray;
}

void ACreatureCharacter::UpdatePartColor(int32 PartIndex, EColor NewColor)
{
	if (PartIndex < 0 || PartIndex >= BodyParts.Num()) return;
    
	FSimpleBodyPart& Part = BodyParts[PartIndex];
	Part.CurrentColor = NewColor;
    
	// 更新材质颜色
	int32 MatIndex = Part.MaterialIndex;
	if (MatIndex >= 0 && MatIndex < DynamicMaterials.Num() && DynamicMaterials[MatIndex])
	{
		// [!! 使用编辑器配置的颜色映射 !!]
		FLinearColor NewLinearColor = GetLinearColorFromEnum(NewColor);
        
		DynamicMaterials[MatIndex]->SetVectorParameterValue("BaseColor", NewLinearColor);
        
		UE_LOG(LogTemp, Warning, TEXT("部位 %s 染色为 %d (线性颜色: %s)"), 
			*Part.PartName, (int32)NewColor, *NewLinearColor.ToString());
	}
    
	CheckColorUnity();
}
void ACreatureCharacter::CheckColorUnity()
{
	if (BodyParts.Num() == 0) return;
	EColor FirstColor = BodyParts[0].CurrentColor;
	if (FirstColor == EColor::EC_None) 
	{
		SetColor(EColor::EC_None);
		CurrentState = ECreatureState::Hostile;
		OnRep_CreatureState();
		return;
	}
	// 检查是否所有部位颜色相同
	bool bUnified = true;
	for (const FSimpleBodyPart& Part : BodyParts)
	{
		if (Part.CurrentColor != FirstColor)
		{
			bUnified = false;
			break;
		}
	}
	if (bUnified)
	{
		UE_LOG(LogTemp, Warning, TEXT("Creature %s: 颜色统一为 %d！现在可以被附身！"), *GetName(), (int32)FirstColor);
		SetColor(FirstColor);
		CurrentState = ECreatureState::Unified;
        
		// [!! 新增：设置为可附身状态 !!]
		bCanBePossessed = true;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Creature %s: 颜色未统一，保持敌对状态"), *GetName());
		SetColor(EColor::EC_None);
		CurrentState = ECreatureState::Hostile;
        
		// [!! 新增：设置为不可附身状态 !!]
		bCanBePossessed = false;
	}
	OnRep_CreatureState();
}
/** 怪物 AI Tick */
void ACreatureCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// [!! GDD 怪物 AI Tick !!]
	if (!IsPlayerControlled() && CurrentState == ECreatureState::Hostile)
	{
		// (AI 移动逻辑由 AIController 处理)
	}
}

/** 绑定玩家附身后的输入 */
void ACreatureCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// [!! 关键 !!] 先调用父类 (ACharacter) 的绑定
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// [!! 复制自 APossessablePawn !!]
		// 绑定“解除附身” (F键)
		if (PossessAction)
		{
			EnhancedInputComp->BindAction(PossessAction, ETriggerEvent::Started, this, &ACreatureCharacter::OnUnpossess);
		}
		
		// [!! GDD 逻辑：绑定特殊能力 !!]
		if (MoveAction)
		{
			// 注意：ACharacter 已经处理了 MoveAction (通过 AddMovementInput)
			// 你可能需要重写基类的移动绑定，或者在这里不绑定
			// 为了简单起见，我们假设 ACharacter 的默认 AddMovementInput 就可以了
		}
		if (FireProjectileAction)
		{
			EnhancedInputComp->BindAction(FireProjectileAction, ETriggerEvent::Started, this, &ACreatureCharacter::OnLMBPressed);
		}
		if (JumpAction)
		{
			EnhancedInputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ACreatureCharacter::OnJumpPressed);
		}
	}
}

// --- [!! GDD 逻辑：部位和状态 !!] ---

/** 当任何一个“部位”颜色改变时，此函数会被调用 */
void ACreatureCharacter::OnPartColorChanged(EColor NewColor, EColor OldColor)
{
	UE_LOG(LogTemp, Warning, TEXT("Creature %s: 一个部位颜色改变了。正在检查是否统一..."), *GetName());
	CheckForColorUnity();
}

/** 检查所有“部位”的颜色是否统一 */
void ACreatureCharacter::CheckForColorUnity()
{
	if (ColorableParts.Num() == 0) return; // 没有部位，无法检查

	EColor TargetColor = ColorableParts[0]->GetColor();
	bool bIsUnified = true;

	if (TargetColor == EColor::EC_None)
	{
		bIsUnified = false; // 统一后的颜色不能是“灰色”
	}
	else
	{
		for (UColorComponent* Part : ColorableParts)
		{
			if (!Part || Part->GetColor() != TargetColor)
			{
				bIsUnified = false; // 颜色不匹配
				break;
			}
		}
	}

	if (bIsUnified)
	{
		UE_LOG(LogTemp, Warning, TEXT("Creature %s: 颜色已统一为 %d!"), *GetName(), (int32)TargetColor);
		CurrentState = ECreatureState::Unified;
		SetColor(TargetColor); // [!!] 设置“主”颜色，使其可被附身
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Creature %s: 颜色未统一。"), *GetName());
		CurrentState = ECreatureState::Hostile;
		SetColor(EColor::EC_None); // [!!] 设为灰色，使其不可附身
	}
	
	// 手动调用 RepNotify (因为服务器自己也需要知道状态变化)
	OnRep_CreatureState();
}

/** 当 CurrentState 改变时自动调用 */
void ACreatureCharacter::OnRep_CreatureState()
{
	AAIController* AI = Cast<AAIController>(GetController());
	
	if (CurrentState == ECreatureState::Hostile)
	{
		UE_LOG(LogTemp, Log, TEXT("Creature %s: AI 状态 -> Hostile"), *GetName());
		// (恢复 AI)
	}
	else 
	{
		UE_LOG(LogTemp, Log, TEXT("Creature %s: AI 状态 -> Unified (Tamed)"), *GetName());
		
		// 停止 AI 移动
		if (AI) 
		{ 
			AI->StopMovement(); 
		}

		// [!! 修复 !!] 
		// 确保生物在被“驯服”时恢复到正常的物理状态 (行走/下落)
		// 这样它就不会卡在“飞行”模式
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->SetDefaultMovementMode(); // 自动设为 Walking 或 Falling
			MoveComp->GravityScale = 1.0f; // 确保重力恢复
		}
	}
}

// --- [!! GDD 逻辑：特殊能力 !!] ---

void ACreatureCharacter::OnLMBPressed()
{
	EColor MyColor = GetColor(); 
	UE_LOG(LogTemp, Warning, TEXT("CreatureCharacter %s: 基类LMB被触发，颜色为 %d"), *GetName(), (int32)MyColor);
	// [!! 新增 !!] 调用蓝图事件，让子类可以在蓝图中实现
	BP_OnSpecialAbilityTriggered();
	// [!! 基类默认行为 - 子类可以重写这个函数来替换 !!]
	switch (MyColor)
	{
	case EColor::EC_Red:
		UE_LOG(LogTemp, Log, TEXT("基类：执行【红色】默认能力！"));
		// 基类的默认红色能力
		break;
	default:
		UE_LOG(LogTemp, Log, TEXT("基类：此颜色没有默认LMB能力。"));
		break;
	}
}

void ACreatureCharacter::OnJumpPressed()
{
	EColor MyColor = GetColor();
	UE_LOG(LogTemp, Warning, TEXT("CreatureCharacter %s: 基类Jump被触发，颜色为 %d"), *GetName(), (int32)MyColor);
	// [!! 新增 !!] 调用蓝图事件，让子类可以在蓝图中实现
	BP_OnJumpAbilityTriggered();
	// [!! 基类默认行为 - 子类可以重写这个函数来替换 !!]
	switch (MyColor)
	{
	case EColor::EC_Green:
		UE_LOG(LogTemp, Log, TEXT("基类：执行【绿色】默认高跳！"));
		LaunchCharacter(FVector(0,0,1500.f), false, true); // 基类的默认高跳
		break;
	default:
		UE_LOG(LogTemp, Log, TEXT("基类：执行普通跳跃"));
		Super::Jump(); // ACharacter的默认跳跃
		break;
	}
}

// --- [!! 以下是 100% 复制自 APossessablePawn 的功能 !!] ---

EColor ACreatureCharacter::GetColor() const 
{ 
	return ColorComponent ? ColorComponent->GetColor() : EColor::EC_None; 
}

void ACreatureCharacter::SetColor(EColor NewColor) 
{ 
	if (ColorComponent) { ColorComponent->SetColor(NewColor); } 
}

FTransform ACreatureCharacter::GetCharacterExitTransform() const 
{ 
	if (CharacterExitPoint) { return CharacterExitPoint->GetComponentTransform(); }
	FTransform ExitTransform = GetActorTransform();
	ExitTransform.AddToTranslation(FVector(0,0,100));
	return ExitTransform;
}

void ACreatureCharacter::OnUnpossess()
{
	AController* MyController = GetController();
	if (MyController)
	{
		AColorMageController* MageController = Cast<AColorMageController>(MyController);
		if (MageController)
		{
			MageController->RequestRepossessOriginalCharacter();
		}
	}
}

void ACreatureCharacter::FellOutOfWorld(const class UDamageType& dmgType)
{
	UE_LOG(LogTemp, Warning, TEXT("CreatureCharacter %s 掉出世界!"), *GetName());
	AGameModeBase* CurrentGameModeBase = UGameplayStatics::GetGameMode(this);
	AColorMageGameMode* MyGameMode = Cast<AColorMageGameMode>(CurrentGameModeBase);
	if (MyGameMode)
	{
		AController* MyController = GetController();
		if (MyController)
		{
			MyGameMode->RespawnPlayer(MyController);
		}
		else { Super::FellOutOfWorld(dmgType); }
	}
	else { Super::FellOutOfWorld(dmgType); }
}

void ACreatureCharacter::PlayPossessEffect()
{
	if (PossessVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PossessVFX, GetActorLocation(), GetActorRotation());
	}
}

void ACreatureCharacter::PlayUnpossessEffect()
{
	if (UnpossessVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), UnpossessVFX, GetActorLocation(), GetActorRotation());
	}
}

void ACreatureCharacter::OnHighlight_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("CreatureCharacter高亮: %s"), *GetName());
	// 添加高亮效果
}
void ACreatureCharacter::OnUnhighlight_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("CreatureCharacter取消高亮: %s"), *GetName());
	// 移除高亮效果
}

void ACreatureCharacter::PlayJumpAttackLand()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && JumpAttackLandMontage)
	{
		AnimInstance->Montage_Play(JumpAttackLandMontage);
	}
}

void ACreatureCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController); // 调用父类

	if (Cast<AColorMageController>(NewController))
	{
		UE_LOG(LogTemp, Warning, TEXT("CreatureCharacter %s: 被玩家附身。"), *GetName());
       
		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->SetDefaultMovementMode(); 
			MoveComp->GravityScale = 3.0f; // (你粘贴的值)
		}
		
		// 立即调用一次，以同步当前状态的效果
		HandleMainColorChange(GetColor(), GetColor()); 
	}
	// (如果是被 AI 附身, AIController::OnPossess 会处理)
}

// --- [!! 复制自 AColorableActor 的功能 (已修复) !!] ---
void ACreatureCharacter::HandleMainColorChange(EColor NewColor, EColor OldColor)
{
    UE_LOG(LogTemp, Warning, TEXT("=== %s 颜色变化 (HandleMainColorChange): %d -> %d ==="), *GetName(), (int32)OldColor, (int32)NewColor);

	// [!! 修正 !!] 如果旧颜色是黄色，清理光照
	if (OldColor == EColor::EC_Yellow)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 离开黄色 - 关闭光照"), *GetName());
		
		for (AHiddenPathActor* Path : RevealedPaths)
		{
			if (IsValid(Path))
			{
				Path->Hide();
			}
		}
		RevealedPaths.Empty();
	}

	// 处理光照逻辑（黄色）
	if (NewColor == EColor::EC_Yellow)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 进入黄色 - 激活光照"), *GetName());
        
		UWorld* World = GetWorld();
		if (World)
		{
			TArray<FOverlapResult> OverlapResults;
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			bool bHasOverlaps = World->OverlapMultiByObjectType(
				OverlapResults,
				GetActorLocation(),
				FQuat::Identity,
				FCollisionObjectQueryParams(ECC_WorldStatic),
				FCollisionShape::MakeSphere(LightRadius),
				QueryParams
			);

			
			for (const FOverlapResult& Result : OverlapResults)
			{
				if (AHiddenPathActor* Path = Cast<AHiddenPathActor>(Result.GetActor()))
				{
					Path->Reveal();
					RevealedPaths.AddUnique(Path);
				}
			}
		}
	}


}
