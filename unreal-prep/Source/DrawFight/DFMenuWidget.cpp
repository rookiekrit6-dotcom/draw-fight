#include "DFMenuWidget.h"
#include "DFProfileSubsystem.h"
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

	const TCHAR* RarityTH(EDFRarity R)
	{
		switch (R)
		{
		case EDFRarity::Legendary: return TEXT("ตำนาน");
		case EDFRarity::Rare:      return TEXT("หายาก");
		case EDFRarity::Signature: return TEXT("ประจำตัว");
		default:                   return TEXT("ธรรมดา");
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
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
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
void UDFMenuWidget::ShowGacha()  { Page = EPage::Gacha;  Toast.Reset(); LastPull.Reset(); Refresh(); }

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
	case EPage::Gacha:  Root->SetContent(BuildGacha()); break;
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
				*Hero.NameTH, *Hero.TitleTH, P->GetGold(), P->GetWins()), 22, Hex(Hero.Color, Gold))
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
				[ Button(TEXT("สุ่มการ์ดสกิล"), [this]() { ShowGacha(); }, FLinearColor(0.55f, 0.35f, 0.9f), 24) ]
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
		? TEXT("ช่องสกิล (กดเพื่อเปลี่ยน · การ์ดจากกาชาใส่ได้ทุกตัว)")
		: TEXT("สกิลเริ่มต้น"), 20, FLinearColor::White, true) ];
	const TArray<FName> Slots = P->GetLoadout(Viewing);
	for (int32 S = 0; S < Slots.Num(); ++S)
	{
		FDFSkillRow SR;
		const bool bHas = P->GetSkill(Slots[S], SR);
		const int32 Lv = P->GetSkillLevel(Slots[S]);
		FString T = FString::Printf(TEXT("%d. %s"), S + 1, bHas ? *SR.NameTH : TEXT("(ว่าง)"));
		if (Lv > 0) T += FString::Printf(TEXT("  Lv%d"), Lv);
		if (bHas) T += TEXT("  — ") + SR.DescTH;
		const FName Hero = Viewing;
		Info->AddSlot().AutoHeight().Padding(0, 3)
		[ Button(T, [this, Hero, S]() { Profile()->CycleLoadoutSlot(Hero, S); Refresh(); },
			bHas ? RarityColor(SR.Rarity) * FLinearColor(.5f, .5f, .5f, 1.f) : Panel, 17, bOwn) ];
	}

	// action button
	TSharedRef<SWidget> Action = bSel
		? Button(TEXT("กำลังใช้ตัวนี้"), []() {}, Panel, 24, false)
		: bOwn
		? Button(TEXT("ใช้ตัวนี้"), [this]() { Profile()->Select(Viewing); Refresh(); }, Green, 24)
		: Button(FString::Printf(TEXT("ปลดล็อก %d ทอง"), H.UnlockCost), [this]()
			{
				if (!Profile()->Unlock(Viewing)) Toast = TEXT("ทองไม่พอ ลองไปสู้เก็บทองก่อน");
				Refresh();
			}, Gold * FLinearColor(.8f, .8f, .8f, 1.f), 24, P->GetGold() >= H.UnlockCost);
	Info->AddSlot().AutoHeight().Padding(0, 16, 0, 0)[ Action ];
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

// ================= GACHA =================
TSharedRef<SWidget> UDFMenuWidget::BuildGacha()
{
	UDFProfileSubsystem* P = Profile();
	FDFCharacterRow Hero;
	P->GetCharacter(P->GetSelected(), Hero);

	auto DoPull = [this](int32 N)
	{
		LastPull = Profile()->Pull(N);
		Toast = LastPull.Num() ? FString() : FString(TEXT("ทองไม่พอ"));
		Refresh();
	};

	TSharedRef<SVerticalBox> Results = SNew(SVerticalBox);
	for (const FDFGachaResult& R : LastPull)
	{
		FDFSkillRow SR;
		P->GetSkill(R.Skill, SR);
		FString T = FString::Printf(TEXT("[%s]  %s"), RarityTH(R.Rarity), *SR.NameTH);
		T += R.NewLevel > 0 ? FString::Printf(TEXT("   Lv%d"), R.NewLevel) : FString::Printf(TEXT("   เต็มแล้ว คืน %d ทอง"), R.Refund);
		if (R.bFitsHero) T += TEXT("   * เข้ากับตัวละคร");
		Results->AddSlot().AutoHeight().Padding(0, 3)[ Label(T, 22, RarityColor(R.Rarity), R.Rarity == EDFRarity::Legendary) ];
	}
	if (!LastPull.Num())
		Results->AddSlot().AutoHeight()[ Label(TEXT("กดสุ่มเพื่อรับการ์ดสกิล ได้ซ้ำจะอัปเลเวล (สูงสุด Lv3)"), 20, Dim) ];

	TSharedRef<SVerticalBox> Col = SNew(SVerticalBox);
	Col->AddSlot().AutoHeight().Padding(0, 0, 0, 20)[ TopBar(FText::FromString(TEXT("สุ่มการ์ดสกิล"))) ];
	Col->AddSlot().AutoHeight()[ Label(FString::Printf(
		TEXT("ตัวละคร %s (%s · %s) — สกิลที่เข้ากับอาวุธและธาตุจะออกง่ายขึ้น"),
		*Hero.NameTH, *P->WeaponNameTH(Hero.WeaponClass), *P->ElementNameTH(Hero.Element)), 20, Dim) ];
	Col->AddSlot().AutoHeight().Padding(0, 6, 0, 16)[ Label(FString::Printf(
		TEXT("การันตีตำนานใน %d ครั้ง"), FMath::Max(0, UDFGacha::PityPulls - P->GetPity())), 20, Gold) ];
	Col->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
		[ Button(FString::Printf(TEXT("สุ่ม 1 ครั้ง (%d)"), UDFGacha::CostSingle), [DoPull]() { DoPull(1); },
			Blue, 24, P->GetGold() >= UDFGacha::CostSingle) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
		[ Button(FString::Printf(TEXT("สุ่ม 10 ครั้ง (%d)"), UDFGacha::CostTen), [DoPull]() { DoPull(10); },
			FLinearColor(0.55f, 0.35f, 0.9f), 24, P->GetGold() >= UDFGacha::CostTen) ]
#if !UE_BUILD_SHIPPING
		+ SHorizontalBox::Slot().AutoWidth()
		[ Button(TEXT("+1000 ทอง (ทดสอบ)"), [this]() { Profile()->AddGold(1000); Refresh(); }, Panel, 16) ]
#endif
	];
	if (!Toast.IsEmpty()) Col->AddSlot().AutoHeight().Padding(0, 8)[ Label(Toast, 20, Accent) ];
	Col->AddSlot().FillHeight(1.f).Padding(0, 20, 0, 0)
	[
		SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(Panel).Padding(20.f)
		[ SNew(SScrollBox) + SScrollBox::Slot()[ Results ] ]
	];
	return Col;
}
