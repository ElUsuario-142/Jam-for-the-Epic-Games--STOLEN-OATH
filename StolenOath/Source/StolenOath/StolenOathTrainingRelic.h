// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StolenOathTrainingRelic.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** Primitive-only relic pickup for the first playable Stolen Oath slice. */
UCLASS()
class AStolenOathTrainingRelic : public AActor
{
	GENERATED_BODY()

public:
	AStolenOathTrainingRelic();
	virtual void BeginPlay() override;
	void Collect();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> RelicBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> RelicCore;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance", meta=(ClampMin="0.01"))
	FVector RelicBodyScale = FVector(0.52f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance", meta=(ClampMin="0.01"))
	FVector RelicCoreScale = FVector(0.2f, 0.2f, 0.65f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	FRotator RelicCoreRotation = FRotator(0.0f, 45.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|Prototype|Appearance")
	FLinearColor RelicColor = FLinearColor(0.95f, 0.58f, 0.12f, 1.0f);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RelicMaterial;
};
