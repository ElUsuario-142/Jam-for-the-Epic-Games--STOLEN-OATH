// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimEnums.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "StolenOathCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UAnimSequence;
class UAnimInstance;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AStolenOathTrainingSentinel;
class AStolenOathTrainingRelic;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AStolenOathCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
protected:
	/** Current prototype combat health. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="1.0"))
	float MaxHealth = 5.0f;

	/** Current health for the first playable combat slice. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stolen Oath|Combat")
	float CurrentHealth = 5.0f;

	/** Resonance earned by successful dodges. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Resonance = 0.0f;

	/** Quinn-compatible prototype attack animation sequence. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat")
	TObjectPtr<UAnimSequence> BasicAttackAnimation = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat|Animation", meta=(ClampMin="0.1", ClampMax="5.0"))
	float BasicAttackAnimationPlayRate = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat|Animation", meta=(ClampMin="0.1", ClampMax="5.0"))
	float DirectorCutAnimationPlayRate = 0.82f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat|Animation", meta=(ClampMin="0.1", ClampMax="5.0"))
	float DodgeAnimationPlayRate = 2.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", Units="cm"))
	float BasicAttackTraceHeight = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="1.0", Units="cm"))
	float BasicAttackRange = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="1.0", Units="cm"))
	float BasicAttackRadius = 95.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", Units="cm/s"))
	float BasicAttackImpulse = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="1.0", Units="cm"))
	float RelicInteractionRange = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.0", Units="cm"))
	float TrainingRelicSpawnDistance = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.0", Units="cm"))
	float TrainingSentinelSpawnDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.0", Units="cm"))
	float TrainingSentinelGroundOffset = 180.0f;

	/** Runtime copy of the dash animation with its root bone locked in place. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> DodgeInPlaceAnimation = nullptr;

	/** Whether the prototype sparring sentinel should spawn beside the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype")
	bool bSpawnTrainingSentinel = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Classes")
	TSubclassOf<AStolenOathTrainingRelic> TrainingRelicClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Classes")
	TSubclassOf<AStolenOathTrainingSentinel> TrainingSentinelClass;

	/** How long a dodge protects Kael from an incoming strike. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.1", ClampMax="1.5", Units="s"))
	float DodgeDuration = 0.45f;

	/** Fixed horizontal travel distance for a dodge, independent of friction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(DisplayName="Dodge Distance", ClampMin="100.0", ClampMax="1000.0", Units="cm"))
	float DodgeSpeed = 350.0f;

	/** Resonance earned for avoiding one complete sentinel strike. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="100.0"))
	float NormalDodgeResonance = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="100.0"))
	float PreciseDodgeResonance = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="100.0"))
	float PerfectDodgeResonance = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="1.0", Units="s"))
	float PerfectDodgeWindow = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="1.0", Units="s"))
	float PreciseDodgeWindow = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0"))
	float BasicAttackDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="100.0"))
	float AttackResonanceGain = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MaxResonanceDamageBonus = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.0"))
	float DirectorCutDamage = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="1.0", Units="cm"))
	float DirectorCutRange = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Combat", meta=(ClampMin="0.1", Units="s"))
	float DirectorCutStunDuration = 1.4f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stolen Oath|Prototype")
	TObjectPtr<UStaticMeshComponent> PrototypeSwordBlade;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stolen Oath|Prototype")
	TObjectPtr<UStaticMeshComponent> PrototypeSwordHilt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Stolen Oath|Prototype")
	TObjectPtr<UStaticMeshComponent> PrototypeSwordGuard;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PrototypeSwordBladeMaterial;

	bool bIsDodging = false;
	bool bAttackRootMotionSuppressed = false;
	TEnumAsByte<ERootMotionMode::Type> PreAttackRootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
	TWeakObjectPtr<UAnimInstance> AttackAnimInstance;
	uint32 NextPlayerAttackId = 0;
	uint32 LastHandledSentinelAttackId = 0;
	FVector PrototypeSpawnLocation = FVector::ZeroVector;
	FTimerHandle DodgeTimerHandle;
	FTimerHandle AttackRootMotionRestoreTimerHandle;
	TWeakObjectPtr<AStolenOathTrainingSentinel> TrainingSentinel;
	TWeakObjectPtr<AStolenOathTrainingRelic> TrainingRelic;
	float DodgeStartTime = 0.0f;
	bool bHasRelic = false;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void EndDodge();
	void RestoreAttackRootMotionMode();
	void PlayBasicAttackAnimation(float PlayRate);
	void TryCollectRelic();
	void PerformDirectorCut();
	void SpawnTrainingSentinel();
	void UpdateSwordAppearance();
	void PerformBasicAttack();
	void StartDodge();

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

public:

	/** Constructor */
	AStolenOathCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Handles a telegraphed prototype sentinel attack. */
	void ReceiveSentinelAttack(uint32 AttackId, float Damage);
	void ShowCombatMessage(const FString& Message, const FColor& Color = FColor::White, float Duration = 1.35f) const;

	/** Read-only values used by the prototype HUD. */
	UFUNCTION(BlueprintPure, Category="Stolen Oath|Combat")
	float GetHealthFraction() const;

	UFUNCTION(BlueprintPure, Category="Stolen Oath|Combat")
	float GetResonanceFraction() const;
	float GetResonance() const { return Resonance; }
	bool HasRelic() const { return bHasRelic; }
	bool IsRelicInteractionAvailable() const;

	UFUNCTION(BlueprintPure, Category="Stolen Oath|Combat")
	bool IsDodging() const { return bIsDodging; }

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

