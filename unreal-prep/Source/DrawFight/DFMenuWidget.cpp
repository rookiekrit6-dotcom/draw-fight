#include "DFMenuWidget.h"
#include "DFProfileSubsystem.h"
#include "DFFonts.h"
#include "DFVoiceSubsystem.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"

namespace
{
	const FLinearColor BG(0.02f, 0.025f, 0.04f, 0.92f);
	const FLinearColor Panel(0.07f, 0.08f, 0.12f, 0.95f);
	const FLinearColor Gold(1.f, 0.78f, 0.2f);
	const FLinearColor Dim(0.6f, 0.63f, 0.7f);
	const FLinearColor Accent(0.95f, 0.3f, 0.32f);
	const FLinearColor Blue(0.25f, 0.5f, 0.95f);
	const FLinearColor Green(0.25f, 0.75f, 0.45f);

	FLinearColor Hex(const FString& H, FLinearColor Fallback = FLinearColor::White)
	{
		return H.StartsWith(TEXT("#")) ? FLinearColor(FColor::FromHex(H)) : Fallback;
	}

	FLinearColor RarityColor(EDFRarity R)
	{
		switch (R)
		{
		case EDFRarity::Legendary: return FLinearColor(1.f, 0.62f, 0.1f);
		case EDFRarity::Rare:      return FLinearColor(0.55f, 0.45f, 1.f);
		default:                   return FLinearColor(0.75f, 0.8f, 0.85f);
		}
	}

	FString Bar(float Mult) // 1.0 = 5 blocks
	{
		const int32 N = FMath::Clamp(FMath::RoundToInt(Mult * 5.f), 1, 9);
		return FString::ChrN(N, TEXT('|')) + FString::ChrN(9 - N, TEXT('.'));
	}
}

UDFProfileSubsystem* UDFMenuWidget::Profile() const { return UDFProfileSubsystem::Get(this); }

FSlateFontInfo UDFMenuWidget::Font(int32 Size, bool bBold) const
{
	if (ThaiFont) return FSlateFontInfo(ThaiFont, Size, bBold ? TEXT("Bold") : TEXT("Default"));
	return FDFFonts::Get(Size, bBold);
}

TSharedRef<SWidget> UDFMenuWidget::Label(const FString& Text, int32 Size, FLinearColor Color, bool bBold) const
{
	return SNew(STextBlock).Text(FText::FromString(Text)).Font(Font(Size, bBold))
		.ColorAndOpacity(FSlateColor(Color)).AutoWrapText(true);
}

TSharedRef<SWidget> UDFMenuWidget::Button(const FString& Text, TFunction<void()> OnClick, FLinearColor Color, int32 Size, bool bEnabled)
{
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	return SNew(SButton)
		.ButtonColorAndOpacity(Color)
		.IsEnabled(bEnabled)
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.ContentPadding(FMargin(18.f, 10.f))
		.OnClicked_Lambda([Self, OnClick]()
		{
			if (Self.IsValid()) OnClick();
			return FReply::Handled();
		})
		[
			Label(Text, Size, FLinearColor::White, true)
		];
}

TSharedRef<SWidget> UDFMenuWidget::RebuildWidget()
{
	SAssignNew(Root, SBox);
	Refresh();
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(BG)
		.Padding(40.f)
		[
			Root.ToSharedRef()
		];
}

void UDFMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Root.Reset();
}

void UDFMenuWidget::ShowMain()   { Page = EPage::Main;   Toast.Reset(); Refresh(); }
void UDFMenuWidget::ShowHeroes() { Page = EPage::Heroes; Toast.Reset(); Refresh(); }
void UDFMenuWidget::ShowAura()    { Page = EPage::Aura;    Toast.Reset(); LastRolls.Reset(); Refresh(); }
void UDFMenuWidget::ShowClasses() { Page = EPage::Classes; Toast.Reset(); Refresh(); }

void UDFMenuWidget::Refresh()
{
	if (!Root.IsValid()) return;
	UDFProfileSubsystem* P = Profile();
	if (!P || !P->HasData())
	{
		Root->SetContent(Label(TEXT("ไม่พบตารางข้อมูล ให้ import CSV ไปไว้ที่ /Game/DrawFight/Data ก่อน\n(DT_Characters, DT_Skills, DT_WeaponClasses, DT_Elements)"), 22, Accent));
		return;
	}
	switch (Page)
	{
	case EPage::Main:   Root->SetContent(BuildMain()); break;
	case EPage::Heroes: Root->SetContent(BuildHeroes()); break;
	case EPage::Aura:    Root->SetContent(BuildAura()); break;
	case EPage::Classes: Root->SetContent(BuildClasses()); break;
	}
}

TSharedRef<SWidget> UDFMenuWidget::TopBar(const FText& Title)
{
	UDFProfileSubsystem* P = Profile();
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[ Button(TEXT("< กลับ"), [this]() { ShowMain(); }, Panel, 18) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(20.f, 0.f).VAlign(VAlign_Center)
		[ Label(Title.ToString(), 34, FLinearColor::White, true) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[ Label(FString::Printf(TEXT("ทอง %d"), P->GetGold()), 26, Gold, true) ];
}

// ================= MAIN =================
TSharedRef<SWidget> UDFMenuWidget::BuildMain()
{
	UDFProfileSubsystem* P = Profile();
	FDFCharacterRow Hero;
	P->GetCharacter(P->GetSelected(), Hero);
	TWeakObjectPtr<UDFMenuWidget> Self(this);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 40, 0, 6)
		[ Label(TEXT("วาดแล้วสู้"), 72, FLinearColor::White, true) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 30)
		[ Label(TEXT("DRAW FIGHT"), 24, Dim, true) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 30)
		[
			Label(FString::Printf(TEXT("ตัวละคร: %s · %s      ทอง %d      ชนะ %d"),
				*Hero.NameTH, *Hero.TitleTH, P->GetGold(), P->GetWins())
				+ [P]() {
					FString X; FDFAuraRow A; FDFSecretClassRow K;
					if (P->GetAura(P->GetEquippedAura(), A)) X += FString::Printf(TEXT("\nออร่า: %s"), *A.NameTH);
					if (P->GetSecretClass(P->GetEquippedClass(), K)) X += FString::Printf(TEXT("      อาชีพ: %s"), *K.NameTH);
					return X; }(), 22, Hex(Hero.Color, Gold))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SBox).WidthOverride(380.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
				[ Button(TEXT("เริ่มสู้"), [Self]() {
					if (Self.IsValid()) UGameplayStatics::OpenLevel(Self.Get(), Self->ArenaLevel); }, Accent, 30) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
				[ Button(TEXT("เลือกตัวละคร"), [this]() { ShowHeroes(); }, Blue, 24) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
				[ Button(TEXT("หมุนออร่า"), [this]() { ShowAura(); }, FLinearColor(0.55f, 0.35f, 0.9f), 24) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
				[ Button(TEXT("อาชีพลับ"), [this]() { ShowClasses(); }, FLinearColor(0.15f, 0.12f, 0.2f), 24) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
				[ Button(TEXT("ออกจากเกม"), [Self]() {
					if (Self.IsValid()) UKismetSystemLibrary::QuitGame(Self.Get(), Self->GetOwningPlayer(), EQuitPreference::Quit, false); }, Panel, 20) ]
			]
		];
}

// ================= HEROES =================
TSharedRef<SWidget> UDFMenuWidget::BuildHeroes()
{
	UDFProfileSubsystem* P = Profile();
	const TArray<FName> Ids = P->GetCharacterIds();
	if (Viewing.IsNone() || !Ids.Contains(Viewing)) Viewing = P->GetSelected();

	// left: hero list
	TSharedRef<SScrollBox> List = SNew(SScrollBox);
	for (const FName& Id : Ids)
	{
		FDFCharacterRow R;
		if (!P->GetCharacter(Id, R)) continue;
		const bool bOwn = P->IsUnlocked(Id);
		FString Text = R.NameTH;
		if (Id == P->GetSelected()) Text += TEXT("  (ใช้อยู่)");
		else if (!bOwn) Text += FString::Printf(TEXT("  [ล็อก %d]"), R.UnlockCost);
		FLinearColor C = Hex(R.Color, Blue);
		if (Id != Viewing) C *= FLinearColor(.45f, .45f, .45f, 1.f);
		List->AddSlot().Padding(0, 4)
		[ Button(Text, [this, Id]() { Viewing = Id; Toast.Reset(); Refresh(); }, C, 22) ];
	}

	// right: details
	FDFCharacterRow H;
	P->GetCharacter(Viewing, H);
	const bool bOwn = P->IsUnlocked(Viewing);
	const bool bSel = P->GetSelected() == Viewing;
	const FLinearColor HC = Hex(H.Color, Blue);

	TSharedRef<SVerticalBox> Info = SNew(SVerticalBox);
	Info->AddSlot().AutoHeight()[ Label(H.NameTH + TEXT("  ") + H.NameEN, 40, HC, true) ];
	Info->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[ Label(H.TitleTH, 22, Dim) ];
	Info->AddSlot().AutoHeight()[ Label(FString::Printf(TEXT("ธาตุ %s   ·   อาวุธ %s"),
		*P->ElementNameTH(H.Element), *P->WeaponNameTH(H.WeaponClass)), 20) ];
	Info->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[ Label(FString::Printf(
		TEXT("พลังชีวิต  %s\nพลังโจมตี %s\nความเร็ว   %s\nขนาดตัว   %s"),
		*Bar(H.HpMult), *Bar(H.DamageMult), *Bar(H.SpeedMult), *Bar(H.SizeMult)), 20, FLinearColor::White) ];
	Info->AddSlot().AutoHeight().Padding(0, 10, 0, 0)[ Label(TEXT("\"") + H.QuoteTH + TEXT("\""), 20, Gold) ];
	Info->AddSlot().AutoHeight().Padding(0, 6, 0, 0)[ Label(H.LookTH, 16, Dim) ];

	// skill slots
	Info->AddSlot().AutoHeight().Padding(0, 16, 0, 4)[ Label(bOwn
		? TEXT("ช่องสกิล (ช่อง 1 เป็นสกิลประจำตัว · ช่อง 2-3 กดเพื่อเปลี่ยน)")
		: TEXT("สกิลเริ่มต้น"), 20, FLinearColor::White, true) ];
	const TArray<FName> Slots = P->GetLoadout(Viewing);
	for (int32 S = 0; S < Slots.Num(); ++S)
	{
		FDFSkillRow SR;
		const bool bHas = P->GetSkill(Slots[S], SR);
		const int32 Lv = P->GetSkillLevel(Slots[S]);
		FString T = FString::Printf(TEXT("%d. %s"), S + 1, bHas ? *SR.NameTH : TEXT("(ว่าง)"));
		if (Lv > 0) T += FString::Printf(TEXT("  Lv%d"), Lv);
		if (S == 0) T += TEXT("  [ประจำตัว]");
		if (bHas) T += TEXT("  — ") + SR.DescTH;
		const FName Hero = Viewing;
		Info->AddSlot().AutoHeight().Padding(0, 3)
		[ Button(T, [this, Hero, S]() { Profile()->CycleLoadoutSlot(Hero, S); Refresh(); },
			bHas ? RarityColor(SR.Rarity) * FLinearColor(.5f, .5f, .5f, 1.f) : Panel, 17, bOwn && S > 0) ];
	}

	// action button
	TSharedRef<SWidget> Action = bSel
		? Button(TEXT("กำลังใช้ตัวนี้"), []() {}, Panel, 24, false)
		: bOwn
		? Button(TEXT("ใช้ตัวนี้"), [this]() {
			Profile()->Select(Viewing);
			if (UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this)) V->Play(this, Viewing, EDFVoiceEvent::Select);
			Refresh(); }, Green, 24)
		: Button(FString::Printf(TEXT("ปลดล็อก %d ทอง"), H.UnlockCost), [this]()
			{
				if (!Profile()->Unlock(Viewing)) Toast = TEXT("ทองไม่พอ ลองไปสู้เก็บทองก่อน");
				Refresh();
			}, Gold * FLinearColor(.8f, .8f, .8f, 1.f), 24, P->GetGold() >= H.UnlockCost);
	Info->AddSlot().AutoHeight().Padding(0, 16, 0, 0)[ Action ];

	// player-made voice pack for this hero
	if (UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this))
	{
		const FName Hero = Viewing;
		Info->AddSlot().AutoHeight().Padding(0, 18, 0, 4)[ Label(TEXT("เสียงพากย์ (อัดเสียงเองได้)"), 20, FLinearColor::White, true) ];
		Info->AddSlot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
			[ Button(V->PackLabel(V->GetAssignedPack(Hero)), [this, Hero]() {
				UDFVoiceSubsystem* VV = UDFVoiceSubsystem::Get(this);
				if (VV && !VV->CycleAssignedPack(Hero).IsEmpty()) VV->Play(this, Hero, EDFVoiceEvent::Select);
				Refresh(); }, Blue * FLinearColor(.7f, .7f, .7f, 1.f), 17) ]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
			[ Button(TEXT("เปิดโฟลเดอร์เสียง"), [this]() { if (UDFVoiceSubsystem* VV = UDFVoiceSubsystem::Get(this)) VV->OpenPacksFolder(); }, Panel, 17) ]
			+ SHorizontalBox::Slot().AutoWidth()
			[ Button(TEXT("โหลดใหม่"), [this]() { if (UDFVoiceSubsystem* VV = UDFVoiceSubsystem::Get(this)) VV->ScanPacks(); Refresh(); }, Panel, 17) ]
		];
	}
	if (!Toast.IsEmpty()) Info->AddSlot().AutoHeight().Padding(0, 6)[ Label(Toast, 18, Accent) ];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 20)[ TopBar(FText::FromString(TEXT("เลือกตัวละคร"))) ]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 24, 0)
			[ SNew(SBox).WidthOverride(300.f)[ List ] ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			[
				SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(Panel).Padding(24.f)
				[ SNew(SScrollBox) + SScrollBox::Slot()[ Info ] ]
			]
		];
}

// ================= AURA =================
TSharedRef<SWidget> UDFMenuWidget::BuildAura()
{
	UDFProfileSubsystem* P = Profile();
	auto DoRoll = [this](int32 N)
	{
		LastRolls = Profile()->RollAuras(N);
		for (const FName& Id : LastRolls)
		{
			FDFAuraRow A;
			if (Profile()->GetAura(Id, A) && A.Chance >= 1000)
			{
				if (UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this)) V->Play(this, Profile()->GetSelected(), EDFVoiceEvent::RareAura);
				break;
			}
		}
		Toast = LastRolls.Num() ? FString() : FString(TEXT("ทองไม่พอ"));
		Refresh();
	};
	auto OneIn = [](int32 N) { return FString::Printf(TEXT("1 ใน %s"), *FText::AsNumber(N).ToString()); };

	// last roll results (rarest first)
	TSharedRef<SVerticalBox> Results = SNew(SVerticalBox);
	TArray<FName> Sorted = LastRolls;
	Sorted.Sort([P](const FName& A, const FName& B) { FDFAuraRow X, Y; P->GetAura(A, X); P->GetAura(B, Y); return X.Chance > Y.Chance; });
	for (int32 I = 0; I < FMath::Min(Sorted.Num(), 10); ++I)
	{
		FDFAuraRow A; P->GetAura(Sorted[I], A);
		Results->AddSlot().AutoHeight().Padding(0, 2)
		[ Label(FString::Printf(TEXT("%s   (%s)"), *A.NameTH, *OneIn(A.Chance)), A.Chance >= 1000 ? 30 : 20, Hex(A.Color2, FLinearColor::White), A.Chance >= 1000) ];
	}
	if (!LastRolls.Num())
		Results->AddSlot().AutoHeight()[ Label(TEXT("หมุนเพื่อรับออร่า ยิ่งหายากยิ่งแรงและสวย ทุกครั้งที่ 10 โชคคูณ 2"), 20, Dim) ];

	// collection
	TSharedRef<SVerticalBox> Coll = SNew(SVerticalBox);
	for (const FName& Id : P->GetAuraIds())
	{
		FDFAuraRow A; P->GetAura(Id, A);
		const int32 N = P->AuraCount(Id);
		if (A.bSecret && N == 0) { Coll->AddSlot().AutoHeight().Padding(0, 2)[ Label(TEXT("???   (ความลับ)"), 18, Dim) ]; continue; }
		const bool bOn = P->GetEquippedAura() == Id;
		FString T = FString::Printf(TEXT("%s  %s   ·   HP +%d%%  ตี +%d%%  เร็ว +%d%%"),
			*A.NameTH, *OneIn(A.Chance), FMath::RoundToInt(A.HpBonus * 100), FMath::RoundToInt(A.DamageBonus * 100), FMath::RoundToInt(A.SpeedBonus * 100));
		if (N > 0) T += FString::Printf(TEXT("   x%d"), N);
		if (bOn) T += TEXT("   (ใส่อยู่)");
		Coll->AddSlot().AutoHeight().Padding(0, 2)
		[ Button(N > 0 ? T : FString::Printf(TEXT("ยังไม่มี  ·  %s"), *OneIn(A.Chance)),
			[this, Id]() { Profile()->EquipAura(Id); Refresh(); },
			N > 0 ? Hex(A.Color, Panel) * FLinearColor(.6f, .6f, .6f, 1.f) : Panel, 16, N > 0 && !bOn) ];
	}

	TSharedRef<SVerticalBox> Col = SNew(SVerticalBox);
	Col->AddSlot().AutoHeight().Padding(0, 0, 0, 16)[ TopBar(FText::FromString(TEXT("หมุนออร่า"))) ];
	Col->AddSlot().AutoHeight()[ Label(FString::Printf(TEXT("หมุนแล้ว %d ครั้ง   ·   โชคครั้งถัดไป x%.1f"), P->GetTotalRolls(), P->LuckForNextRoll()), 20, Gold) ];
	Col->AddSlot().AutoHeight().Padding(0, 10)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
		[ Button(FString::Printf(TEXT("หมุน 1 (%d)"), UDFProfileSubsystem::RollCost), [DoRoll]() { DoRoll(1); }, Blue, 24, P->GetGold() >= UDFProfileSubsystem::RollCost) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
		[ Button(FString::Printf(TEXT("หมุน 10 (%d)"), UDFProfileSubsystem::RollCost * 10), [DoRoll]() { DoRoll(10); },
			FLinearColor(0.55f, 0.35f, 0.9f), 24, P->GetGold() >= UDFProfileSubsystem::RollCost * 10) ]
#if !UE_BUILD_SHIPPING
		+ SHorizontalBox::Slot().AutoWidth()
		[ Button(TEXT("+1000 ทอง (ทดสอบ)"), [this]() { Profile()->AddGold(1000); Refresh(); }, Panel, 16) ]
#endif
	];
	if (!Toast.IsEmpty()) Col->AddSlot().AutoHeight()[ Label(Toast, 20, Accent) ];
	Col->AddSlot().FillHeight(1.f).Padding(0, 12, 0, 0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(.4f).Padding(0, 0, 16, 0)
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(20.f)
			[ SNew(SScrollBox) + SScrollBox::Slot()[ Results ] ] ]
		+ SHorizontalBox::Slot().FillWidth(.6f)
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(20.f)
			[ SNew(SScrollBox) + SScrollBox::Slot()[ Coll ] ] ]
	];
	return Col;
}

// ================= SECRET CLASSES =================
TSharedRef<SWidget> UDFMenuWidget::BuildClasses()
{
	UDFProfileSubsystem* P = Profile();
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (const FName& Id : P->GetClassIds())
	{
		FDFSecretClassRow K; P->GetSecretClass(Id, K);
		int32 Cur, Need; P->ClassProgress(Id, Cur, Need);
		const bool bOpen = P->IsClassUnlocked(Id);
		const bool bOn = P->GetEquippedClass() == Id;
		TSharedRef<SVerticalBox> Card = SNew(SVerticalBox);
		if (bOpen)
		{
			Card->AddSlot().AutoHeight()[ Label(K.NameTH + TEXT("  ") + K.NameEN, 28, Hex(K.Color, Gold), true) ];
			Card->AddSlot().AutoHeight()[ Label(K.DescTH, 18, FLinearColor::White) ];
			Card->AddSlot().AutoHeight().Padding(0, 4)[ Label(FString::Printf(TEXT("ท่าไม้ตาย (R): %s   ·   ติดตัว: %s"),
				*P->SkillNameTH(K.Ultimate), *K.PassiveTH), 18, Gold) ];
			Card->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
			[ bOn ? Button(TEXT("ถอดอาชีพ"), [this]() { Profile()->EquipClass(NAME_None); Refresh(); }, Panel, 18)
				  : Button(TEXT("ใช้อาชีพนี้"), [this, Id]() { Profile()->EquipClass(Id); Refresh(); }, Green, 18) ];
		}
		else
		{
			Card->AddSlot().AutoHeight()[ Label(TEXT("??? อาชีพลับ"), 26, Dim, true) ];
			Card->AddSlot().AutoHeight()[ Label(TEXT("เบาะแส: ") + K.HintTH, 18, FLinearColor::White) ];
			if (Need > 1 && Cur > 0)
				Card->AddSlot().AutoHeight()[ Label(FString::Printf(TEXT("ความคืบหน้า %d%%"), Cur * 100 / Need), 16, Gold) ];
		}
		List->AddSlot().AutoHeight().Padding(0, 6)
		[ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(bOn ? Hex(K.Color, Panel) * FLinearColor(.35f, .35f, .35f, 1.f) : Panel).Padding(18.f)[ Card ] ];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)[ TopBar(FText::FromString(TEXT("อาชีพลับ"))) ]
		+ SVerticalBox::Slot().AutoHeight()[ Label(TEXT("อาชีพลับปลดล็อกจากการเล่นแบบพิเศษ ใส่ได้กับทุกตัวละคร ได้ท่าไม้ตายปุ่ม R"), 20, Dim) ]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 12, 0, 0)[ SNew(SScrollBox) + SScrollBox::Slot()[ List ] ];
}
