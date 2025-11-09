// CreatureCharacter.h
#pragma once

#include "CoreMinimal.h"
#include "ColorTypes.h"
#include "HighlightableInterface.h"
#include "PawnControlType.h"
#include "GameFramework/Character.h"
#include "CreatureCharacter.generated.h"

class UInputAction;
class UColorComponent;
class UStaticMeshComponent;
class USceneComponent;
class USphereComponent;
class AHiddenPathActor;     // 明确包含
class ABurnableWoodActor;   // 明确包含
class AColorMageCharacter;  // 明确包含
class UNiagaraSystem;


USTRUCT(BlueprintType)
struct FColorMapping
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EColor ColorEnum = EColor::EC_Red;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor LinearColor = FLinearColor::Red;
};

USTRUCT(BlueprintType)
struct FSimpleBodyPart
{
    GENERATED_BODY()
    /** 骨骼名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName BoneName;
    /** 材质索引 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaterialIndex = 0;
    /** 部位名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartName = TEXT("Part");
    /** 当前颜色 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EColor CurrentColor = EColor::EC_None;
};


UCLASS(Abstract) // [!! 新增 !!] 标记为抽象基类
class COLORMAGE_API ACreatureCharacter : public ACharacter, public IHighlightableInterface
{
    GENERATED_BODY()

public:
    ACreatureCharacter();

    virtual void Tick(float DeltaTime) override;

    // --- [!! 从 APossessablePawn 复制的功能 !!] ---
    UFUNCTION(BlueprintCallable, Category = "Color Magic")
    EColor GetColor() const;

    UFUNCTION(BlueprintCallable, Category = "Color Magic")
    void SetColor(EColor NewColor);

    UFUNCTION(BlueprintCallable, Category = "Possession")
    FTransform GetCharacterExitTransform() const;

    UFUNCTION(BlueprintPure, Category = "UI")
    EPawnControlType GetControlType() const { return ControlType; }

    UFUNCTION(BlueprintCallable, Category = "Effects")
    virtual void PlayPossessEffect();

    UFUNCTION(BlueprintCallable, Category = "Effects")
    virtual void PlayUnpossessEffect();
    void OnHighlight_Implementation();
    void OnUnhighlight_Implementation();

    // [!! 新增 !!] 虚函数供子类重写
    /** 子类重写这个函数来实现自己的LMB能力 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Abilities")
    void BP_OnSpecialAbilityTriggered();

    /** 子类重写这个函数来实现自己的Jump能力 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Abilities") 
    void BP_OnJumpAbilityTriggered();

    UFUNCTION(BlueprintPure)
    bool CanBePossessed() const {
        bool bResult = bCanBePossessed && (CurrentState == ECreatureState::Unified);
        UE_LOG(LogTemp, Warning, TEXT("Creature %s: CanBePossessed检查 - bCanBePossessed:%s, CurrentState:%d, 结果:%s"), 
            *GetName(), 
            bCanBePossessed ? TEXT("true") : TEXT("false"),
            (int32)CurrentState,
            bResult ? TEXT("true") : TEXT("false"));
        return bResult;
    }

    UFUNCTION(BlueprintPure, Category = "Creature")
    ECreatureState GetCurrentState() const { return CurrentState; }
    
    /** 播放跳跃攻击“落地”动画 */
    void PlayJumpAttackLand();
    virtual void PossessedBy(AController* NewController) override;
    void HandleMainColorChange(EColor NewColor, EColor OldColor);
    
protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

    // --- [!! 现有组件保持不变 !!] ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UColorComponent> ColorComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Possession")
    TObjectPtr<USceneComponent> CharacterExitPoint;

   
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    EPawnControlType ControlType = EPawnControlType::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
    TObjectPtr<UNiagaraSystem> PossessVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
    TObjectPtr<UNiagaraSystem> UnpossessVFX;
    
    void OnUnpossess();

    // --- [!! 复制自 AColorableActor (已修改) !!] ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions")
    bool bReactsToMovementColors = false; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions")
    bool bReactsToLightColor = false; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions")
    bool bReactsToFireColor = false; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions")
    bool bReactsToLifeColor = false; 

    // (移动 黑/白)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Movement")
    float MoveDistance = 500.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Movement")
    float MoveSpeed = 5.0f;

    // (发光 黄)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> LightVolume;
    // [!! 已移除 !!] 
    // TObjectPtr<USpotLightComponent> PointLight; 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Light")
    float LightRadius = 500.0f;


    // (生长 绿)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Life")
    FVector StretchAmount = FVector(2.0, 2.0, 1.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Life")
    float GrowthTime = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Color Reactions|Life")
    FVector GrowthDirection;
    // --- [!! 现有怪物逻辑保持不变 !!] ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CreatureState, Category = "Creature")
    ECreatureState CurrentState = ECreatureState::Hostile;
    // === 光照相关 ===
    UPROPERTY()
    TArray<TObjectPtr<AHiddenPathActor>> RevealedPaths;
    
    UFUNCTION()
    void OnRep_CreatureState();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature")
    TArray<TObjectPtr<UColorComponent>> ColorableParts;

    UFUNCTION()
    void OnPartColorChanged(EColor NewColor, EColor OldColor);

    void CheckForColorUnity();

    // [!! 修改 !!] 将这些改为虚函数，供子类重写
    /** 基类的LMB处理，子类可以重写 */
    virtual void OnLMBPressed();
    
    /** 基类的Jump处理，子类可以重写 */
    virtual void OnJumpPressed();

    // --- [!! 输入资产保持不变 !!] ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> FireProjectileAction;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> PossessAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Creature|AI")
    float PatrolSpeed = 150.0f;

    bool bCanBePossessed;

protected:
    /** 身体部位列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body Parts")
    TArray<FSimpleBodyPart> BodyParts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body Parts")
    TArray<FColorMapping> ColorMappings;
    /** 动态材质数组 */
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;
public:
    /** 被颜色子弹击中时调用 */
    UFUNCTION(BlueprintCallable)
    void HitByColorProjectile(EColor IncomingColor, FVector HitLocation);
private:
    /** 根据击中位置确定是哪个身体部位 */
    int32 GetBodyPartIndexFromHitLocation(FVector HitLocation);
    FLinearColor GetLinearColorFromEnum(EColor InColor) const;

    /** 更新部位颜色 */
    void UpdatePartColor(int32 PartIndex, EColor NewColor);
    
    /** 检查颜色统一 */
    void CheckColorUnity();
    
    /** 初始化材质 */
    void InitMaterials();

protected:
    // [!! 新增：摄像机距离设置 !!]
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    float CameraDistance = 400.0f;  // 默认距离
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    float CameraHeight = 100.0f;    // 摄像机高度偏移
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    float CameraLagSpeed = 3.0f;    // 摄像机跟随速度
    public:
    // [!! 新增：获取摄像机设置的函数 !!]
    UFUNCTION(BlueprintPure, Category = "Camera")
    float GetCameraDistance() const { return CameraDistance; }
    UFUNCTION(BlueprintPure, Category = "Camera")
    float GetCameraHeight() const { return CameraHeight; }
    UFUNCTION(BlueprintPure, Category = "Camera")
    float GetCameraLagSpeed() const { return CameraLagSpeed; }

    // --- [!! 动画 !!] ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Animation")
    TObjectPtr<UAnimMontage> JumpAttackLandMontage;
    
};
