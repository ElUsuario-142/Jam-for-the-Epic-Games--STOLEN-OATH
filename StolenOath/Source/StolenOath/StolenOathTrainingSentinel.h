// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Variant_Combat/Interfaces/CombatDamageable.h"
#include "StolenOathTrainingSentinel.generated.h"

class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AStolenOathCharacter;

/** Primitive-only sparring target used by the first Stolen Oath gameplay slice. */
UCLASS()
class AStolenOathTrainingSentinel : public AActor, public ICombatDamageable
{
	GENERATED_BODY()

public:
	AStolenOathTrainingSentinel();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool ReceivePlayerAttack(uint32 AttackId, float Damage, const FVector& HitLocation);
	float GetHealth() const { return CurrentHealth; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthFraction() const { return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f; }
	int32 GetCombatPhase() const { return bPhaseTwoActive ? 2 : 1; }
	void InterruptForDirectorCut(float StunDuration);

	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;
	virtual void HandleDeath() override;
	virtual void ApplyHealing(float Healing, AActor* Healer) override;
	virtual void NotifyDanger(const FVector& DangerLocation, AActor* DangerSource) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> SentinelMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	TObjectPtr<UMaterialInterface> SentinelBaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SentinelDynamicMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> OverheadWarningMesh;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> SweepWarningMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OverheadWarningFillMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OverheadWarningEdgeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SweepWarningFillMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SweepWarningEdgeMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="1.0"))
	float MaxHealth = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="100.0", Units="cm"))
	float AttackRange = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Movement", meta=(ClampMin="0.0", Units="cm/s"))
	float FollowSpeed = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Movement", meta=(ClampMin="0.0", Units="cm"))
	float FollowStopDistance = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Movement", meta=(ClampMin="0.0"))
	float TurnSpeed = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.1", Units="s"))
	float AttackInterval = 3.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.1", Units="s"))
	float TelegraphDuration = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.0"))
	float AttackDamage = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.1", Units="s"))
	float PhaseTwoAttackInterval = 2.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.1", Units="s"))
	float PhaseTwoTelegraphDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="1.0", Units="cm"))
	float OverheadAttackRadius = 190.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="1.0", Units="cm"))
	float SweepAttackRange = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.0", Units="cm"))
	float SweepAttackInnerRadius = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SweepAttackDotThreshold = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype", meta=(ClampMin="1.0"))
	float PhaseTwoDamageMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Timing", meta=(ClampMin="0.0", Units="s"))
	float InitialAttackDelay = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Timing", meta=(ClampMin="0.0", Units="s"))
	float DefeatRespawnDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Timing", meta=(ClampMin="0.0", Units="s"))
	float HitFlashDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Timing", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PhaseTwoHealthFraction = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Timing", meta=(ClampMin="0.0", Units="s"))
	float PhaseTwoEntryDelay = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance", meta=(ClampMin="0.01"))
	FVector BaseMeshScale = FVector(2.0f, 2.0f, 3.6f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	FLinearColor PhaseOneColor = FLinearColor(0.85f, 0.035f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	FLinearColor PhaseTwoColor = FLinearColor(0.95f, 0.2f, 0.015f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	FLinearColor HitFlashColor = FLinearColor(1.0f, 0.72f, 0.12f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Warnings")
	FLinearColor OverheadWarningColor = FLinearColor(1.0f, 0.015f, 0.0f, 0.24f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Warnings")
	FLinearColor SweepWarningColor = FLinearColor(1.0f, 0.015f, 0.0f, 0.24f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Warnings", meta=(ClampMin="24", ClampMax="128"))
	int32 WarningArcSegments = 64;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Warnings", meta=(ClampMin="2", ClampMax="32"))
	int32 WarningRadialSubdivisions = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Warnings", meta=(ClampMin="1.0", Units="cm"))
	float WarningEdgeWidth = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Warnings", meta=(ClampMin="0.0", Units="cm"))
	float WarningGroundOffset = 3.0f;

	float CurrentHealth = 5.0f;
	bool bPhaseTwoActive = false;
	bool bUseSweepNext = false;
	bool bCurrentAttackIsSweep = false;
	bool bInterruptedForSpecial = false;
	FVector AttackTargetLocation = FVector::ZeroVector;
	FVector AttackForward = FVector::ForwardVector;
	uint32 NextAttackId = 0;
	uint32 LastPlayerAttackId = 0;
	FTimerHandle AttackTimerHandle;
	FTimerHandle StrikeTimerHandle;
	FTimerHandle ResetTimerHandle;
	FTimerHandle HitFlashTimerHandle;
	FTimerHandle StunTimerHandle;

	void TelegraphStrike();
	void ResolveStrike(uint32 AttackId);
	void ResetAfterDefeat();
	void UpdateDamageAppearance();
	void ClearHitFlash();
	void UpdateCombatPhase();
	void EndSpecialInterrupt();
	void CreateWarningMeshes();
	void SetWarningMeshesVisible(bool bVisible);
	void RebuildOverheadWarning();
	void RebuildSweepWarning();
	void BuildPolarGrid(UProceduralMeshComponent* Mesh, float InnerRadius, float OuterRadius,
		float StartAngle, float EndAngle, bool bWrapAngle, UMaterialInterface* Material);
	FVector ProjectWarningPointToGround(UProceduralMeshComponent* Mesh, float X, float Y, float SurfaceOffset) const;
	float GetCurrentAttackInterval() const;
};
