// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "StolenOathHUD.generated.h"

/** Minimal native HUD for the first playable slice; intended to be replaced by the team's UMG art. */
UCLASS()
class AStolenOathHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	void SetCombatCue(const FString& Message, const FLinearColor& Color, float Duration = 1.35f);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="0.0"))
	float HUDLeft = 42.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="0.0"))
	float HUDTop = 38.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float ResourceBarWidth = 280.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float ResourceBarHeight = 17.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float RelicPromptWidth = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float RelicPromptHeight = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RelicPromptVerticalAnchor = 0.78f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float BossBarWidth = 460.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="0.0"))
	float BossBarTop = 32.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float BossBarHeight = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float CombatCueMaxWidth = 560.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="1.0"))
	float CombatCueHeight = 44.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Layout", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CombatCueVerticalAnchor = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Colors")
	FLinearColor ResourceBarBackgroundColor = FLinearColor(0.05f, 0.07f, 0.1f, 0.9f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Colors")
	FLinearColor HealthBarColor = FLinearColor(0.75f, 0.12f, 0.16f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Colors")
	FLinearColor ResonanceBarColor = FLinearColor(0.12f, 0.72f, 0.92f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Colors")
	FLinearColor BossHealthBarColor = FLinearColor(0.9f, 0.16f, 0.08f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Stolen Oath|HUD|Colors")
	FLinearColor CombatCuePanelColor = FLinearColor(0.015f, 0.02f, 0.03f, 0.82f);

	FString CombatCueMessage;
	FLinearColor CombatCueColor = FLinearColor::White;
	float CombatCueExpireTime = 0.0f;
};
