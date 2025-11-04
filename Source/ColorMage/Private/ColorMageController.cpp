#include "ColorMageController.h"
#include "ColorManagerSubsystem.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ColorMageCharacter.h"
#include "PossessablePawn.h"
#include "ColorSourceActor.h" 

AColorMageController::AColorMageController() { bShowMouseCursor = false; }

void AColorMageController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		if (DefaultInputMappingContext) { Subsystem->AddMappingContext(DefaultInputMappingContext, 0); }
	}
    if (PlayerCameraManager) { PlayerCameraManager->ViewPitchMin = -70.0f; PlayerCameraManager->ViewPitchMax = 80.0f; }
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true); 
	SetInputMode(InputMode);
	
	EPawnControlType PossessedType = EPawnControlType::Unknown;
	if (InPawn)
	{
		if (AColorMageCharacter* ColorMageCharacter = Cast<AColorMageCharacter>(InPawn)) { PossessedType = ColorMageCharacter->GetControlType(); }
		else if (APossessablePawn* PossPawn = Cast<APossessablePawn>(InPawn)) { PossessedType = PossPawn->GetControlType(); }
	}
	OnPawnControlChanged.Broadcast(PossessedType);
}

void AColorMageController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInputComp = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction) { EnhancedInputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleMove); }
		if (LookAction) { EnhancedInputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &AColorMageController::HandleLook); }

		// --- [!! GDD 修正：绑定 !!] ---
		if (AcquireAction) // RMB
		{
			EnhancedInputComp->BindAction(AcquireAction, ETriggerEvent::Started, this, &AColorMageController::OnAcquire);
		}
		if (PossessAction) // F
		{
			EnhancedInputComp->BindAction(PossessAction, ETriggerEvent::Started, this, &AColorMageController::OnPossessInteract);
		}
		// --- [!! GDD 修正结束 !!] ---
	}
}

void AColorMageController::HandleMove(const FInputActionValue& Value)
{
    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;
    const FVector2D MoveVector = Value.Get<FVector2D>();
    const FRotator Rotation = GetControlRotation(); 
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    if (MoveVector.Y != 0.0f) { MyPawn->AddMovementInput(ForwardDirection, MoveVector.Y); }
    if (MoveVector.X != 0.0f) { MyPawn->AddMovementInput(RightDirection, MoveVector.X); }
}
void AColorMageController::HandleLook(const FInputActionValue& Value)
{
    const FVector2D LookVector = Value.Get<FVector2D>();
    if (LookVector.X != 0.0f) { AddYawInput(LookVector.X); }
    if (LookVector.Y != 0.0f) { AddPitchInput(-LookVector.Y); } // 反转 Pitch
}

// --- [!! GDD 修正：新函数 !!] ---
/** (RMB) 处理汲取/混合请求 */
void AColorMageController::OnAcquire()
{
	AColorMageCharacter* MyCharacter = Cast<AColorMageCharacter>(GetPawn());

	// 2. 检查它是否是我们的法师角色 (平台/生物不能汲取颜色)
	if (MyCharacter)
	{
		// 3. 告诉角色去执行“汲取”动作（播放动画并设置计时器）
		MyCharacter->RequestAcquireColor();
	}
	else
	{
		// (如果正附身在平台上按 RMB，什么也不做)
		UE_LOG(LogTemp, Log, TEXT("OnAcquire: 只有法师角色才能汲取颜色。"));
	}
}

// --- [!! GDD 修正：新函数 !!] ---
/** (F) 处理附身请求 */
void AColorMageController::OnPossessInteract()
{
	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	FVector TraceStart = CamLoc;
	FVector TraceEnd = TraceStart + (CamRot.Vector() * InteractionDistance);
	TArray<AActor*> ActorsToIgnore;
	APawn* MyPawn = GetPawn(); if (MyPawn) { ActorsToIgnore.Add(MyPawn); }
	FHitResult HitResult;
	bool bHit = UKismetSystemLibrary::LineTraceSingle(
		this, TraceStart, TraceEnd, ETraceTypeQuery::TraceTypeQuery1,
		false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true
	);

	if (bHit && HitResult.GetActor())
	{
		APossessablePawn* TargetPawn = Cast<APossessablePawn>(HitResult.GetActor());
		if (TargetPawn)
		{
			UColorManagerSubsystem* ColorManager = GetWorld()->GetSubsystem<UColorManagerSubsystem>();
			if (ColorManager)
			{
				ColorManager->AttemptPossession(this, TargetPawn);
			}
		}
	}
}
// --- [!! GDD 修正结束 !!] ---

void AColorMageController::RequestRepossessOriginalCharacter()
{
	if (HiddenCharacter.IsValid())
	{
		AColorMageCharacter* CharacterToRepossess = HiddenCharacter.Get();
		APawn* CurrentPossessedPawn = GetPawn();
		FTransform ExitTransform = CharacterToRepossess->GetActorTransform(); 
		if (CurrentPossessedPawn)
		{
			APossessablePawn* Possessable = Cast<APossessablePawn>(CurrentPossessedPawn);
			if (Possessable) { ExitTransform = Possessable->GetCharacterExitTransform(); }
			else { ExitTransform = CurrentPossessedPawn->GetActorTransform(); ExitTransform.AddToTranslation(FVector(0,0,100)); }
		}
		
		Super::Possess(CharacterToRepossess);

		CharacterToRepossess->TeleportTo(ExitTransform.GetLocation(), ExitTransform.GetRotation().Rotator(), false, true);
		HiddenCharacter = nullptr;
	}
	else { /* ... (Log Warning) ... */ }
}