#include "DFHudWidget.h"
#include "DFArenaGameMode.h"
#include "DFCombatComponent.h"
#include "DFProfileSubsystem.h"
#include "DFFonts.h"
#include "DFVoiceSubsystem.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "DFFighterCharacter.h"

namespace
{
	const FLinearColor HudRed(0.9f, 0.25f, 0.27f);
	const FLinearColor HudGreen(0.3f, 0.85f, 0.45f);
	const FLinearColor HudGold(1.f, 0.78f, 0.2f);
	const FLinearColor HudPanel(0.04f, 0.05f, 0.08f, 0.8f);
	const FLinearColor HudDim(0.65f, 0.68f, 0.75f);
	const FSlateBrush* Round(float R = 14.f)
	{
		static FSlateRoundedBoxBrush B14(FLinearColor::White, 14.f, FLinearColor(1, 1, 1, .12f), 1.f);
		static FSlateRoundedBoxBrush B8(FLinearColor::White, 8.f);
		return R > 10.f ? &B14 : &B8;
	}
	const FButtonStyle& HudBtn()
	{
		static FButtonStyle S = []()
		{
			FButtonStyle B = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
			B.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1, 1, 1, .85f), 14.f));
			B.SetHovered(FSlateRoundedBoxBrush(FLinearColor::White, 14.f, FLinearColor::White, 2.f));
			B.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.7f, .7f, .7f, 1.f), 14.f));
			B.SetNormalPadding(FMargin(0)); B.SetPressedPadding(FMargin(0));
			return B;
		}();
		return S;
	}
}

FSlateFontInfo UDFHudWidget::Font(int32 Size) const
{
	if (ThaiFont) return FSlateFontInfo(ThaiFont, Size, TEXT("Default"));
	return FDFFonts::Get(Size, true);
}

TSharedRef<SWidget> UDFHudWidget::Label(TAttribute<FText> Text, int32 Size, FLinearColor Color) const
{
	return SNew(STextBlock).Text(Text).Font(Font(Size)).ColorAndOpacity(FSlateColor(Color))
		.ShadowOffset(FVector2D(2.f, 2.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .8f));
}

TSharedRef<SWidget> UDFHudWidget::Bar(TAttribute<TOptional<float>> Pct, FLinearColor Color, float W, float H) const
{
	return SNew(SBox).WidthOverride(W).HeightOverride(H)
		[ SNew(SProgressBar).Percent(Pct).FillColorAndOpacity(Color) ];
}

UDFCombatComponent* UDFHudWidget::PlayerCombat() const
{
	const ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0);
	return P ? P->FindComponentByClass<UDFCombatComponent>() : nullptr;
}

UDFCombatComponent* UDFHudWidget::Boss() const
{
	for (UDFCombatComponent* C : UDFCombatComponent::Registry())
		if (C && C->bIsBoss && !C->IsDead()) return C;
	return nullptr;
}

void UDFHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ADFArenaGameMode* GM = Cast<ADFArenaGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GM->OnWaveStarted.AddUniqueDynamic(this, &UDFHudWidget::HandleWave);
		GM->OnFightEnded.AddUniqueDynamic(this, &UDFHudWidget::HandleEnd);
	}
	if (UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this))
		V->OnSubtitle.AddUniqueDynamic(this, &UDFHudWidget::HandleSubtitle);
}

TSharedRef<SWidget> UDFHudWidget::RebuildWidget()
{
	TWeakObjectPtr<UDFHudWidget> Self(this);
	auto HPPct = [Self]() -> TOptional<float>
	{
		const UDFCombatComponent* C = Self.IsValid() ? Self->PlayerCombat() : nullptr;
		return C ? C->HPFraction() : 0.f;
	};
	auto HPText = [Self]()
	{
		const UDFCombatComponent* C = Self.IsValid() ? Self->PlayerCombat() : nullptr;
		return C ? FText::FromString(FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(C->HP), FMath::RoundToInt(C->MaxHP))) : FText();
	};
	auto HeroText = [Self]()
	{
		UDFProfileSubsystem* P = Self.IsValid() ? UDFProfileSubsystem::Get(Self.Get()) : nullptr;
		FDFCharacterRow R;
		return (P && P->GetCharacter(P->GetSelected(), R)) ? FText::FromString(R.NameTH) : FText();
	};
	auto WaveText = [Self]()
	{
		const ADFArenaGameMode* GM = Self.IsValid() ? Cast<ADFArenaGameMode>(UGameplayStatics::GetGameMode(Self.Get())) : nullptr;
		if (!GM) return FText();
		if (GM->IsEndless())
		{
			const int32 S = (int32)GM->GetElapsed();
			return FText::FromString(FString::Printf(TEXT("ฝ่าคลื่นไม่รู้จบ   ·   %d:%02d   ·   ล้ม %d   ·   พลังระดับ %d"), S / 60, S % 60, GM->GetKills(), GM->GetPowerLevel() + 1));
		}
		return FText::FromString(FString::Printf(TEXT("บทที่ %d   ·   คลื่น %d/%d   ·   ศัตรูเหลือ %d"), GM->GetChapter(), FMath::Max(1, GM->GetWave()), GM->GetTotalWaves(), GM->EnemiesLeft()));
	};
	auto BossVis = [Self]() { return (Self.IsValid() && Self->Boss()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; };
	auto BossPct = [Self]() -> TOptional<float> { const UDFCombatComponent* B = Self.IsValid() ? Self->Boss() : nullptr; return B ? B->HPFraction() : 0.f; };
	auto BannerText = [Self]() { return Self.IsValid() ? FText::FromString(Self->Banner) : FText(); };
	auto BannerVis = [Self]() { return (Self.IsValid() && Self->BannerLeft > 0.f) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; };
	auto BannerCol = [Self]() { return FSlateColor(Self.IsValid() ? Self->BannerColor : FLinearColor::White); };
	auto SubText = [Self]() { return Self.IsValid() ? FText::FromString(Self->Subtitle) : FText(); };
	auto SubVis = [Self]() { return (Self.IsValid() && Self->SubtitleLeft > 0.f) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; };
	auto DashText = [Self]()
	{
		const UDFCombatComponent* C = Self.IsValid() ? Self->PlayerCombat() : nullptr;
		return FText::FromString((C && C->CanDash()) ? TEXT("Shift  หลบพร้อม") : TEXT("Shift  ..."));
	};

	// skill slots (names only; cooldown display comes with the skill runner)
	TSharedRef<SHorizontalBox> Skills = SNew(SHorizontalBox);
	if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
	{
		const TArray<FName> L = P->GetLoadout(P->GetSelected());
		const FLinearColor PC = P->GetPowerColor();
		auto CdText = [Self](int32 SlotIdx)
		{
			return TAttribute<FText>::CreateLambda([Self, SlotIdx]() {
				const ADFPlayerCharacter* Pl = Self.IsValid() ? Cast<ADFPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(Self.Get(), 0)) : nullptr;
				const float Cd = Pl ? Pl->SlotCooldownLeft(SlotIdx) : 0.f;
				return FText::FromString(Cd > .05f ? FString::Printf(TEXT("%.1f"), Cd) : FString(TEXT("พร้อม"))); });
		};
		auto CdCol = [Self](int32 SlotIdx, FLinearColor Ready)
		{
			return TAttribute<FSlateColor>::CreateLambda([Self, SlotIdx, Ready]() {
				const ADFPlayerCharacter* Pl = Self.IsValid() ? Cast<ADFPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(Self.Get(), 0)) : nullptr;
				const bool bReady = !Pl || Pl->SlotCooldownLeft(SlotIdx) <= .05f;
				return FSlateColor(bReady ? Ready : FLinearColor(.04f, .04f, .06f, .85f)); });
		};
		for (int32 I = 0; I < L.Num(); ++I)
			Skills->AddSlot().AutoWidth().Padding(6.f, 0.f)
			[
				SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(CdCol(I, PC * FLinearColor(.45f, .45f, .45f, .92f)))
				.Padding(FMargin(18.f, 10.f))
				[
					SNew(SBox).MinDesiredWidth(110.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(FText::AsNumber(I + 1), 16, HudGold) ]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(FText::FromString(P->SkillNameTH(L[I])), 18, FLinearColor::White) ]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(CdText(I), 14, HudDim) ]
					]
				]
			];
		const FName Ult = P->GetUltimate();
		if (!Ult.IsNone())
			Skills->AddSlot().AutoWidth().Padding(14.f, 0.f, 6.f, 0.f)
			[
				SNew(SBorder).BorderImage(Round()).BorderBackgroundColor(CdCol(3, FLinearColor(.55f, .08f, .14f, .95f)))
				.Padding(FMargin(22.f, 10.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(FText::FromString(TEXT("R")), 16, HudGold) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(FText::FromString(P->SkillNameTH(Ult)), 18, FLinearColor::White) ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(CdText(3), 14, HudDim) ]
				]
			];
	}

	SAssignNew(EndBox, SBox).Visibility(EVisibility::Collapsed);

	return SNew(SOverlay).Visibility(EVisibility::SelfHitTestInvisible)
		// top-left: hero + HP
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(30.f)
		[
			SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible)
			+ SVerticalBox::Slot().AutoHeight()[ Label(TAttribute<FText>::CreateLambda(HeroText), 24, FLinearColor::White) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[ Bar(TAttribute<TOptional<float>>::CreateLambda(HPPct), HudGreen, 360.f, 22.f) ]
			+ SVerticalBox::Slot().AutoHeight()[ Label(TAttribute<FText>::CreateLambda(HPText), 16, HudDim) ]
		]
		// top-center: wave + boss bar
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0.f, 26.f)
		[
			SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(TAttribute<FText>::CreateLambda(WaveText), 22, FLinearColor::White) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)
			[
				SNew(SVerticalBox).Visibility(TAttribute<EVisibility>::CreateLambda(BossVis))
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Label(TAttribute<FText>::CreateLambda([Self]() {
					const ADFArenaGameMode* GM = Self.IsValid() ? Cast<ADFArenaGameMode>(UGameplayStatics::GetGameMode(Self.Get())) : nullptr;
					return FText::FromString(GM && !GM->BossName.IsEmpty() ? GM->BossName : FString(TEXT("บอส"))); }), 22, HudRed) ]
				+ SVerticalBox::Slot().AutoHeight()[ Bar(TAttribute<TOptional<float>>::CreateLambda(BossPct), HudRed, 640.f, 18.f) ]
			]
		]
		// center: wave banner
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0.f, 0.f, 0.f, 220.f)
		[
			SNew(STextBlock).Visibility(TAttribute<EVisibility>::CreateLambda(BannerVis))
			.Text(TAttribute<FText>::CreateLambda(BannerText)).Font(Font(64))
			.ColorAndOpacity(TAttribute<FSlateColor>::CreateLambda(BannerCol))
			.ShadowOffset(FVector2D(3.f, 3.f)).ShadowColorAndOpacity(FLinearColor(0, 0, 0, .85f))
		]
		// bottom: skills + dash
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0.f, 0.f, 0.f, 30.f)
		[
			SNew(SVerticalBox).Visibility(EVisibility::HitTestInvisible)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 14)
			[
				SNew(SBorder).Visibility(TAttribute<EVisibility>::CreateLambda(SubVis))
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(FLinearColor(0, 0, 0, .55f))
				.Padding(FMargin(16.f, 6.f))
				[ Label(TAttribute<FText>::CreateLambda(SubText), 22, FLinearColor::White) ]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ Skills ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8)[ Label(TAttribute<FText>::CreateLambda(DashText), 16, HudDim) ]
		]
		// win / lose screen
		+ SOverlay::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)[ EndBox.ToSharedRef() ];
}

void UDFHudWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	EndBox.Reset();
}

void UDFHudWidget::HandleSubtitle(const FString& Text, float Seconds)
{
	Subtitle = Text;
	SubtitleLeft = Seconds;
}

void UDFHudWidget::HandleWave(int32 Wave, bool bBossWave)
{
	const ADFArenaGameMode* GM = Cast<ADFArenaGameMode>(UGameplayStatics::GetGameMode(this));
	Banner = bBossWave ? FString::Printf(TEXT("บอส: %s"), GM && !GM->BossName.IsEmpty() ? *GM->BossName : TEXT("มาแล้ว!"))
		: GM ? FString::Printf(TEXT("คลื่นที่ %d / %d"), Wave, GM->GetTotalWaves()) : FString::Printf(TEXT("คลื่นที่ %d"), Wave);
	BannerColor = bBossWave ? HudRed : FLinearColor::White;
	BannerLeft = 2.f;
}

void UDFHudWidget::HandleEnd(bool bVictory)
{
	if (bEnded) return;
	bEnded = true;
	ADFArenaGameMode* GM = Cast<ADFArenaGameMode>(UGameplayStatics::GetGameMode(this));
	const int32 Reward = GM ? GM->LastReward : 0;
	const FName Menu = GM ? GM->MenuLevel : FName(TEXT("MainMenu"));

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	if (!EndBox.IsValid()) return;

	TWeakObjectPtr<UDFHudWidget> Self(this);
	auto MakeBtn = [this, Self](const FString& Text, FLinearColor Color, TFunction<void()> Fn)
	{
		return SNew(SButton).ButtonStyle(&HudBtn()).ButtonColorAndOpacity(Color).HAlign(HAlign_Center).ContentPadding(FMargin(30.f, 14.f))
			.OnClicked_Lambda([Self, Fn]() { if (Self.IsValid()) Fn(); return FReply::Handled(); })
			[ Label(FText::FromString(Text), 24, FLinearColor::White) ];
	};
	const FString Again = UGameplayStatics::GetCurrentLevelName(this);

	// rewards: free skill card + newly unlocked secret classes
	TSharedRef<SVerticalBox> Rewards = SNew(SVerticalBox);
	if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
	{
		FDFGachaResult Card;
		if (P->GetLastCardDrop(Card))
			Rewards->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4)
			[ Label(FText::FromString(Card.NewLevel > 0
				? FString::Printf(TEXT("ได้การ์ดสกิล: %s  Lv%d"), *P->SkillNameTH(Card.Skill), Card.NewLevel)
				: FString::Printf(TEXT("การ์ด %s เต็มแล้ว  คืน %d ทอง"), *P->SkillNameTH(Card.Skill), Card.Refund)), 26, FLinearColor::White) ];
		for (const FName& Id : P->PopNewClasses())
		{
			FDFSecretClassRow K;
			if (P->GetSecretClass(Id, K))
				Rewards->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10)
				[ Label(FText::FromString(FString::Printf(TEXT("ปลดล็อกอาชีพลับ: %s !"), *K.NameTH)), 40, HudGold) ];
		}
	}

	EndBox->SetVisibility(EVisibility::Visible);
	EndBox->SetContent(
		SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0, 0, 0, .7f)).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[ Label(FText::FromString(bVictory ? TEXT("ชนะ!") : TEXT("แพ้...")), 90, bVictory ? HudGold : HudRed) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 6, 0, 30)
			[ Label(FText::FromString(FString::Printf(TEXT("+%d ทอง"), Reward)), 32, HudGold) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
			[ Rewards ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f)
				[ MakeBtn(TEXT("สู้อีกรอบ"), HudRed, [Self, Again]() { UGameplayStatics::OpenLevel(Self.Get(), FName(*Again)); }) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(10.f, 0.f)
				[ MakeBtn(TEXT("กลับเมนู"), HudPanel, [Self, Menu]() { UGameplayStatics::OpenLevel(Self.Get(), Menu); }) ]
			]
		]);
}

void UDFHudWidget::NativeTick(const FGeometry& Geo, float Dt)
{
	Super::NativeTick(Geo, Dt);
	BannerLeft = FMath::Max(0.f, BannerLeft - Dt);
	SubtitleLeft = FMath::Max(0.f, SubtitleLeft - Dt);

	// floating numbers: watch every fighter's HP and pop the difference
	for (UDFCombatComponent* C : UDFCombatComponent::Registry())
	{
		if (!C || !C->GetOwner()) continue;
		float& Last = LastHP.FindOrAdd(C, C->HP);
		const float D = C->HP - Last;
		if (FMath::Abs(D) >= 0.5f && bShowNumbers)
		{
			FFloat F;
			F.World = C->GetOwner()->GetActorLocation() + FVector(FMath::FRandRange(-30.f, 30.f), 0.f, 110.f * C->GetOwner()->GetActorScale3D().Z);
			F.Text = (D > 0 ? FString(TEXT("+")) : FString()) + FString::FromInt(FMath::RoundToInt(FMath::Abs(D)));
			F.Color = D > 0 ? HudGreen : (C->Team == 0 ? HudRed : FLinearColor::White);
			F.Size = (C->bIsBoss || FMath::Abs(D) >= 20.f) ? 34.f : 24.f;
			Floats.Add(F);
		}
		Last = C->HP;
	}
	for (int32 I = Floats.Num() - 1; I >= 0; --I)
	{
		Floats[I].Age += Dt;
		Floats[I].World.Z += 90.f * Dt;
		if (Floats[I].Age > 0.9f) Floats.RemoveAtSwap(I);
	}
	for (auto It = LastHP.CreateIterator(); It; ++It)
		if (!It.Key().IsValid()) It.RemoveCurrent();
}

int32 UDFHudWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Cull,
	FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	Layer = Super::NativePaint(Args, Geo, Cull, Out, Layer, Style, bParentEnabled);
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return Layer;
	const float Scale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");

	auto ToLocal = [&](const FVector& W, FVector2f& OutPos)
	{
		FVector2D S;
		if (!UGameplayStatics::ProjectWorldToScreen(PC, W, S)) return false;
		OutPos = FVector2f(S / Scale);
		return true;
	};

	// enemy HP bars over heads
	if (bShowEnemyBars)
		for (UDFCombatComponent* C : UDFCombatComponent::Registry())
		{
			if (!C || C->Team == 0 || C->IsDead() || C->bIsBoss || !C->GetOwner()) continue;
			FVector2f P;
			const float H = 120.f * C->GetOwner()->GetActorScale3D().Z;
			if (!ToLocal(C->GetOwner()->GetActorLocation() + FVector(0, 0, H), P)) continue;
			const FVector2f Size(70.f, 7.f), Pos = P - FVector2f(35.f, 0.f);
			FSlateDrawElement::MakeBox(Out, Layer + 1, Geo.ToPaintGeometry(Size, FSlateLayoutTransform(Pos)), White,
				ESlateDrawEffect::None, FLinearColor(0, 0, 0, .7f));
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geo.ToPaintGeometry(FVector2f(70.f * C->HPFraction(), 7.f), FSlateLayoutTransform(Pos)), White,
				ESlateDrawEffect::None, C->IsWindingUp() ? FLinearColor(1.f, .55f, .1f) : HudRed);
		}

	// floating numbers
	for (const FFloat& F : Floats)
	{
		FVector2f P;
		if (!ToLocal(F.World, P)) continue;
		const float A = 1.f - FMath::Clamp((F.Age - .5f) / .4f, 0.f, 1.f);
		const float Pop = F.Age < .12f ? 1.f + (.12f - F.Age) * 3.f : 1.f;
		FSlateFontInfo Fi = Font(FMath::RoundToInt(F.Size * Pop));
		const FVector2f Off(-F.Size * .6f, -F.Size * .5f);
		FSlateDrawElement::MakeText(Out, Layer + 3, Geo.ToPaintGeometry(FVector2f(200.f, 60.f), FSlateLayoutTransform(P + Off + FVector2f(2.f, 2.f))),
			F.Text, Fi, ESlateDrawEffect::None, FLinearColor(0, 0, 0, .8f * A));
		FSlateDrawElement::MakeText(Out, Layer + 4, Geo.ToPaintGeometry(FVector2f(200.f, 60.f), FSlateLayoutTransform(P + Off)),
			F.Text, Fi, ESlateDrawEffect::None, F.Color.CopyWithNewOpacity(A));
	}
	return Layer + 4;
}
