// Copyright Epic Games, Inc. All Rights Reserved.

#include "StolenOathHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "StolenOathCharacter.h"
#include "StolenOathTrainingSentinel.h"

void AStolenOathHUD::SetCombatCue(const FString& Message, const FLinearColor& Color, float Duration)
{
	CombatCueMessage = Message;
	CombatCueColor = Color;
	CombatCueExpireTime = GetWorld() ? GetWorld()->GetTimeSeconds() + Duration : 0.0f;
}

void AStolenOathHUD::DrawHUD()
{
	Super::DrawHUD();

	const AStolenOathCharacter* Player = Cast<AStolenOathCharacter>(GetOwningPawn());
	if (!Player || !Canvas || !GEngine)
	{
		return;
	}

	const float Left = HUDLeft;
	const float Top = HUDTop;
	const float BarWidth = ResourceBarWidth;
	const float BarHeight = ResourceBarHeight;
	const UFont* Font = GEngine->GetSmallFont();

	DrawText(TEXT("KAEL  |  HEALTH"), FLinearColor(0.92f, 0.94f, 1.0f), Left, Top, const_cast<UFont*>(Font), 1.15f, false);
	DrawRect(ResourceBarBackgroundColor, Left, Top + 23.0f, BarWidth, BarHeight);
	DrawRect(HealthBarColor, Left + 2.0f, Top + 25.0f, (BarWidth - 4.0f) * Player->GetHealthFraction(), BarHeight - 4.0f);

	DrawText(TEXT("RESONANCE"), FLinearColor(0.65f, 0.93f, 1.0f), Left, Top + 53.0f, const_cast<UFont*>(Font), 1.15f, false);
	DrawRect(ResourceBarBackgroundColor, Left, Top + 76.0f, BarWidth, BarHeight);
	DrawRect(ResonanceBarColor, Left + 2.0f, Top + 78.0f, (BarWidth - 4.0f) * Player->GetResonanceFraction(), BarHeight - 4.0f);

	DrawText(TEXT("LMB ATTACK   SHIFT DODGE"), FLinearColor(0.88f, 0.9f, 0.95f), Left, Top + 108.0f, const_cast<UFont*>(Font), 0.95f, false);
	if (Player->IsRelicInteractionAvailable())
	{
		const float PromptWidth = RelicPromptWidth;
		const float PromptHeight = RelicPromptHeight;
		const float PromptLeft = (Canvas->ClipX - PromptWidth) * 0.5f;
		const float PromptTop = Canvas->ClipY * RelicPromptVerticalAnchor;
		DrawRect(FLinearColor(0.025f, 0.02f, 0.015f, 0.88f), PromptLeft, PromptTop, PromptWidth, PromptHeight);
		DrawText(TEXT("E  CLAIM THE RELIC"), FLinearColor(1.0f, 0.78f, 0.3f), PromptLeft + 22.0f, PromptTop + 10.0f, const_cast<UFont*>(Font), 1.2f, false);
	}
	const bool bDirectorCutReady = Player->HasRelic() && Player->GetResonanceFraction() >= 1.0f;
	const FString DirectorCutHint = bDirectorCutReady
		? TEXT("Q  DIRECTOR'S CUT READY")
		: (Player->HasRelic() ? TEXT("Q  DIRECTOR'S CUT  |  NEEDS FULL RESONANCE") : TEXT("Q  DIRECTOR'S CUT  |  CLAIM RELIC FIRST"));
	DrawText(DirectorCutHint, bDirectorCutReady ? FLinearColor(0.35f, 0.95f, 1.0f) : FLinearColor(0.62f, 0.68f, 0.75f), Left, Top + 130.0f, const_cast<UFont*>(Font), 0.95f, false);

	const AStolenOathTrainingSentinel* Sentinel = nullptr;
	for (TActorIterator<AStolenOathTrainingSentinel> It(GetWorld()); It; ++It)
	{
		Sentinel = *It;
		break;
	}

	if (Sentinel)
	{
		const float CurrentBossBarWidth = FMath::Min(BossBarWidth, Canvas->ClipX - 48.0f);
		const float BossBarLeft = (Canvas->ClipX - CurrentBossBarWidth) * 0.5f;
		const FString BossLabel = Sentinel->GetCombatPhase() == 2
			? TEXT("STONE SENTINEL  |  PHASE II")
			: TEXT("STONE SENTINEL  |  PHASE I");
		DrawText(BossLabel, FLinearColor(1.0f, 0.78f, 0.67f), BossBarLeft, BossBarTop, const_cast<UFont*>(Font), 1.2f, false);
		DrawRect(FLinearColor(0.055f, 0.035f, 0.045f, 0.95f), BossBarLeft, BossBarTop + 23.0f, CurrentBossBarWidth, BossBarHeight);
		DrawRect(BossHealthBarColor, BossBarLeft + 3.0f, BossBarTop + 26.0f, (CurrentBossBarWidth - 6.0f) * Sentinel->GetHealthFraction(), FMath::Max(1.0f, BossBarHeight - 6.0f));
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (!CombatCueMessage.IsEmpty() && CurrentTime <= CombatCueExpireTime)
	{
		const float CueWidth = FMath::Min(CombatCueMaxWidth, Canvas->ClipX - 32.0f);
		const float CueLeft = (Canvas->ClipX - CueWidth) * 0.5f;
		const float CueTop = Canvas->ClipY * CombatCueVerticalAnchor;
		DrawRect(CombatCuePanelColor, CueLeft, CueTop, CueWidth, CombatCueHeight);
		DrawText(CombatCueMessage, CombatCueColor, CueLeft + 14.0f, CueTop + 12.0f, const_cast<UFont*>(Font), 1.2f, false);
	}
}
