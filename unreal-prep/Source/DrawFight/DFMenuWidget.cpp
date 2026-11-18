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
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "DFMenuStage.h"
#include "DFMenuGameMode.h"
#include "DFAudio.h"
#include "DFArenaDressing.h"
#include "DFSaveGame.h"
#include "Widgets/Input/SSlider.h"

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
		.ColorAndOpacity(FSlateColor(Color)).AutoWrapText(Text.Len() > 34);
}

namespace
{
	// rounded, tintable button look shared by every menu button
	const FButtonStyle& FlatStyle()
	{
		static FButtonStyle S = []()
		{
			FButtonStyle B = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
			B.SetNormal(FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, 1.f, .82f), 14.f));
			B.SetHovered(FSlateRoundedBoxBrush(FLinearColor(1.f, 1.f, 1.f, 1.f), 14.f, FLinearColor(1.f, 1.f, 1.f, .9f), 2.f));
			B.SetPressed(FSlateRoundedBoxBrush(FLinearColor(.7f, .7f, .7f, 1.f), 14.f));
			B.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(.35f, .35f, .4f, .45f), 14.f));
			B.SetNormalPadding(FMargin(0.f));
			B.SetPressedPadding(FMargin(0.f));
			return B;
		}();
		return S;
	}
	const FSlateBrush* CardBrush()
	{
		static FSlateRoundedBoxBrush B(FLinearColor::White, 22.f, FLinearColor(1.f, 1.f, 1.f, .08f), 1.f);
		return &B;
	}
	const FSlateBrush* ChipBrush()
	{
		static FSlateRoundedBoxBrush B(FLinearColor::White, 8.f);
		return &B;
	}
}

const FSlateBrush* UDFMenuWidget::Icon(const TCHAR* Name)
{
	const FName Key(Name);
	if (const TSharedPtr<FSlateBrush>* B = Icons.Find(Key)) return B->Get();
	TSharedPtr<FSlateBrush> Br = MakeShared<FSlateBrush>();
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/DrawFight/UI/Icons/%s.%s"), Name, Name)))
	{
		IconTex.Add(T);
		Br->SetResourceObject(T);
		Br->ImageSize = FVector2D(64.f, 64.f);
		Br->DrawAs = ESlateBrushDrawType::Image;
	}
	else Br->DrawAs = ESlateBrushDrawType::NoDrawType;
	Icons.Add(Key, Br);
	return Br.Get();
}

ADFMenuStage* UDFMenuWidget::Stage() const
{
	const ADFMenuPlayerController* PC = Cast<ADFMenuPlayerController>(GetOwningPlayer());
	return PC ? PC->Stage.Get() : nullptr;
}

void UDFMenuWidget::SyncStage()
{
	ADFMenuStage* S = Stage();
	UDFProfileSubsystem* P = Profile();
	if (!S || !P) return;
	if (Page == EPage::Create)
	{
		S->SetFocus(1.f);
		S->ShowHero(NewHero.IsNone() ? P->GetSelected() : NewHero, NewElement.IsNone() ? FLinearColor(.9f, .9f, 1.f) : P->ElementColor(NewElement));
	}
	else
	{
		S->SetFocus(0.f);
		S->ShowHero(P->GetSelected(), P->GetPowerColor());
	}
}

TSharedRef<SWidget> UDFMenuWidget::Button(const FString& Text, TFunction<void()> OnClick, FLinearColor Color, int32 Size, bool bEnabled)
{
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	return SNew(SButton)
		.ButtonStyle(&FlatStyle())
		.ButtonColorAndOpacity(Color)
		.IsEnabled(bEnabled)
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.ContentPadding(FMargin(20.f, 11.f))
		.OnClicked_Lambda([Self, OnClick]()
		{
			if (Self.IsValid()) { DFAudio::Sfx(Self.Get(), TEXT("S_Click"), FVector::ZeroVector, .5f); OnClick(); }
			return FReply::Handled();
		})
		[
			Label(Text, Size, FLinearColor::White, true)
		];
}

TSharedRef<SWidget> UDFMenuWidget::IconButton(const TCHAR* IconName, const FString& Text, const FString& Sub, TFunction<void()> OnClick, FLinearColor Color, bool bBig, bool bEnabled)
{
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	const float IconSize = bBig ? 46.f : 34.f;
	TSharedRef<SVerticalBox> Txt = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Label(Text, bBig ? 30 : 22, FLinearColor::White, true) ];
	if (!Sub.IsEmpty()) Txt->AddSlot().AutoHeight()[ Label(Sub, 15, FLinearColor(1.f, 1.f, 1.f, .7f)) ];
	return SNew(SButton)
		.ButtonStyle(&FlatStyle())
		.ButtonColorAndOpacity(Color)
		.IsEnabled(bEnabled)
		.ContentPadding(FMargin(bBig ? 24.f : 18.f, bBig ? 16.f : 11.f))
		.OnClicked_Lambda([Self, OnClick]() { if (Self.IsValid()) { DFAudio::Sfx(Self.Get(), TEXT("S_Click"), FVector::ZeroVector, .5f); OnClick(); } return FReply::Handled(); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 16, 0)
			[ SNew(SBox).WidthOverride(IconSize).HeightOverride(IconSize)[ SNew(SImage).Image(Icon(IconName)).ColorAndOpacity(FSlateColor(FLinearColor::White)) ] ]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)[ Txt ]
		];
}

TSharedRef<SWidget> UDFMenuWidget::Card(TSharedRef<SWidget> Inner, FLinearColor Tint, float Pad) const
{
	return SNew(SBorder).BorderImage(CardBrush()).BorderBackgroundColor(Tint).Padding(Pad)[ Inner ];
}

TSharedRef<SWidget> UDFMenuWidget::Wrap(TSharedRef<SWidget> PageW) const
{
	return Card(PageW, FLinearColor(0.02f, 0.025f, 0.045f, 0.9f), 32.f);
}

TSharedRef<SWidget> UDFMenuWidget::RebuildWidget()
{
	SAssignNew(Root, SBox);
	Refresh();
	return SNew(SBox).Padding(FMargin(48.f, 36.f))[ Root.ToSharedRef() ];
}

void UDFMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Root.Reset();
}

void UDFMenuWidget::ShowMain()
{
	UDFProfileSubsystem* P = Profile();
	if (P && P->HasData() && !P->IsCreated()) { ShowCreate(); return; }
	Page = EPage::Main; Toast.Reset(); Refresh();
}
void UDFMenuWidget::ShowCreate()
{
	Page = EPage::Create; Step = 0; Toast.Reset();
	if (UDFProfileSubsystem* P = Profile())
	{
		NewHero = P->GetSelected();
		NewName = P->GetPlayerName();
		NewElement = P->GetPlayerElement();
	}
	Refresh();
}
void UDFMenuWidget::ShowHeroes() { Page = EPage::Heroes; Toast.Reset(); Refresh(); }
void UDFMenuWidget::ShowAura()    { Page = EPage::Aura;    Toast.Reset(); LastRolls.Reset(); Refresh(); }
void UDFMenuWidget::ShowClasses() { Page = EPage::Classes; Toast.Reset(); Refresh(); }

void UDFMenuWidget::Refresh()
{
	if (!Root.IsValid()) return;
	UDFProfileSubsystem* P = Profile();
	if (!P || !P->HasData())
	{
		Root->SetContent(Card(Label(TEXT("ไม่พบตารางข้อมูล ให้ import CSV ไปไว้ที่ /Game/DrawFight/Data ก่อน\n(DT_Characters, DT_Skills, DT_WeaponClasses, DT_Elements)"), 22, Accent)));
		return;
	}
	switch (Page)
	{
	case EPage::Title:   Root->SetContent(BuildTitle()); break;
	case EPage::Create:  Root->SetContent(BuildCreate()); break;
	case EPage::Main:    Root->SetContent(BuildMain()); break;
	case EPage::Heroes:  Root->SetContent(Wrap(BuildHeroes())); break;
	case EPage::Aura:    Root->SetContent(Wrap(BuildAura())); break;
	case EPage::Classes: Root->SetContent(Wrap(BuildClasses())); break;
	case EPage::Fight:   Root->SetContent(BuildFight()); break;
	case EPage::Settings: Root->SetContent(BuildSettings()); break;
	}
	SyncStage();
}

TSharedRef<SWidget> UDFMenuWidget::TopBar(const FText& Title)
{
	UDFProfileSubsystem* P = Profile();
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[ SNew(SBox).WidthOverride(170.f)[ IconButton(TEXT("I_Back"), TEXT("กลับ"), FString(), [this]() { ShowMain(); }, Panel) ] ]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(24.f, 0.f).VAlign(VAlign_Center)
		[ Label(Title.ToString(), 36, FLinearColor::White, true) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
			[ SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)[ SNew(SImage).Image(Icon(TEXT("I_Coin"))).ColorAndOpacity(FSlateColor(Gold)) ] ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[ Label(FString::FromInt(P->GetGold()), 28, Gold, true) ]
		];
}

// ================= TITLE =================
TSharedRef<SWidget> UDFMenuWidget::BuildTitle()
{
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(600.f)
			[
				Card(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Label(TEXT("วาดแล้วสู้"), 92, FLinearColor::White, true) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 0, 8)[ Label(TEXT("D R A W   F I G H T"), 24, Gold, true) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 0, 34)[ Label(TEXT("วาดอาวุธ · ปั้นตัวละคร · สู้ให้สุดฝีมือ"), 22, Dim) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
					[ IconButton(TEXT("I_Play"), TEXT("เริ่มเกม"), FString(), [this]() { ShowMain(); }, Accent, true) ]
					+ SVerticalBox::Slot().AutoHeight()
					[ IconButton(TEXT("I_Exit"), TEXT("ออกจากเกม"), FString(), [Self]() {
						if (Self.IsValid()) UKismetSystemLibrary::QuitGame(Self.Get(), Self->GetOwningPlayer(), EQuitPreference::Quit, false); }, Panel) ],
				FLinearColor(0.f, 0.f, 0.f, .5f), 40.f)
			]
		];
}

// ================= CHARACTER CREATION =================
TSharedRef<SWidget> UDFMenuWidget::BuildCreate()
{
	UDFProfileSubsystem* P = Profile();
	static const TCHAR* StepNames[4] = { TEXT("เลือกร่าง"), TEXT("ตั้งชื่อ"), TEXT("เลือกพลัง"), TEXT("ยืนยัน") };

	// step dots
	TSharedRef<SHorizontalBox> Dots = SNew(SHorizontalBox);
	for (int32 I = 0; I < 4; ++I)
	{
		const bool bOn = I <= Step;
		Dots->AddSlot().AutoWidth().Padding(0, 0, 10, 0).VAlign(VAlign_Center)
		[
			SNew(SBorder).BorderImage(ChipBrush()).BorderBackgroundColor(bOn ? Accent : FLinearColor(1, 1, 1, .12f)).Padding(FMargin(12.f, 4.f))
			[ Label(FString::Printf(TEXT("%d %s"), I + 1, StepNames[I]), 15, FLinearColor::White, I == Step) ]
		];
	}

	TSharedRef<SVerticalBox> Body = SNew(SVerticalBox);
	bool bCanNext = true;
	if (Step == 0)
	{
		Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[ Label(TEXT("เลือกร่างเริ่มต้นของคุณ (เปลี่ยนทีหลังได้)"), 20, Dim) ];
		TSharedRef<SWrapBox> Grid = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
		for (const FName& Id : P->GetCharacterIds())
		{
			FDFCharacterRow R;
			if (!P->GetCharacter(Id, R) || (R.UnlockCost > 0 && !P->IsUnlocked(Id))) continue;
			const bool bSel = Id == NewHero;
			const FLinearColor HC = Hex(R.Color, Blue);
			Grid->AddSlot()
			[
				SNew(SBox).WidthOverride(250.f)
				[
					SNew(SButton).ButtonStyle(&FlatStyle())
					.ButtonColorAndOpacity(bSel ? HC : HC * FLinearColor(.35f, .35f, .35f, .9f))
					.ContentPadding(FMargin(14.f, 10.f))
					.OnClicked_Lambda([this, Id]() { NewHero = Id; Refresh(); return FReply::Handled(); })
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[ Label(R.NameTH, 22, FLinearColor::White, true) ]
						+ SVerticalBox::Slot().AutoHeight()[ Label(R.TitleTH, 14, FLinearColor(1, 1, 1, .75f)) ]
					]
				]
			];
		}
		Grid->AddSlot()
		[
			SNew(SBox).WidthOverride(250.f)
			[ IconButton(TEXT("I_Folder"), TEXT("นำเข้า .glb"), TEXT("ใช้โมเดลของคุณเอง"), [this]() {
				const FString Dir = FPaths::ProjectSavedDir() / TEXT("ImportedCharacters");
				IFileManager::Get().MakeDirectory(*Dir, true);
				FPlatformProcess::ExploreFolder(*Dir);
				Toast = TEXT("วางไฟล์ .glb ไว้ในโฟลเดอร์ที่เปิดขึ้นมา (ระบบอ่านโมเดลจะเชื่อมในอัปเดตถัดไป)");
				Refresh(); }, Panel) ]
		];
		Body->AddSlot().AutoHeight()[ Grid ];
		bCanNext = !NewHero.IsNone();
	}
	else if (Step == 1)
	{
		Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[ Label(TEXT("ชื่อที่จะแสดงในเกม (ไม่เกิน 16 ตัวอักษร)"), 20, Dim) ];
		Body->AddSlot().AutoHeight()
		[
			SNew(SBox).HeightOverride(64.f)
			[
				SNew(SEditableTextBox)
				.Text(FText::FromString(NewName))
				.HintText(FText::FromString(TEXT("พิมพ์ชื่อของคุณ")))
				.Font(Font(28, true))
				.OnTextChanged_Lambda([this](const FText& T) { NewName = T.ToString().Left(16); })
				.OnTextCommitted_Lambda([this](const FText& T, ETextCommit::Type How) {
					NewName = T.ToString().Left(16);
					if (How == ETextCommit::OnEnter && !NewName.TrimStartAndEnd().IsEmpty()) { Step = 2; Refresh(); } })
			]
		];
		bCanNext = true; // checked on click (text box updates live)
	}
	else if (Step == 2)
	{
		Body->AddSlot().AutoHeight().Padding(0, 0, 0, 12)[ Label(TEXT("ธาตุของคุณ = สีและพลังของสกิลทุกท่า"), 20, Dim) ];
		TSharedRef<SWrapBox> Grid = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
		for (const FName& E : P->GetElementIds())
		{
			const bool bSel = E == NewElement;
			const FLinearColor EC = P->ElementColor(E);
			Grid->AddSlot()
			[
				SNew(SBox).WidthOverride(170.f)
				[
					SNew(SButton).ButtonStyle(&FlatStyle())
					.ButtonColorAndOpacity(bSel ? EC : EC * FLinearColor(.4f, .4f, .4f, .85f))
					.ContentPadding(FMargin(12.f, 12.f))
					.HAlign(HAlign_Center)
					.OnClicked_Lambda([this, E]() { NewElement = E; Refresh(); return FReply::Handled(); })
					[ Label(P->ElementNameTH(E), 24, FLinearColor::White, true) ]
				]
			];
		}
		Body->AddSlot().AutoHeight()[ Grid ];
		bCanNext = !NewElement.IsNone();
	}
	else
	{
		FDFCharacterRow R; P->GetCharacter(NewHero, R);
		Body->AddSlot().AutoHeight()[ Label(NewName.TrimStartAndEnd(), 52, P->ElementColor(NewElement), true) ];
		Body->AddSlot().AutoHeight().Padding(0, 4, 0, 18)[ Label(FString::Printf(TEXT("%s · %s  —  ธาตุ%s"), *R.NameTH, *R.TitleTH, *P->ElementNameTH(NewElement)), 22, FLinearColor::White) ];
		Body->AddSlot().AutoHeight()[ Label(TEXT("\"") + R.QuoteTH + TEXT("\""), 20, Gold) ];
	}
	if (!Toast.IsEmpty()) Body->AddSlot().AutoHeight().Padding(0, 12, 0, 0)[ Label(Toast, 17, Gold) ];

	// footer
	TSharedRef<SHorizontalBox> Foot = SNew(SHorizontalBox);
	if (Step > 0 || P->IsCreated())
		Foot->AddSlot().AutoWidth()
		[ SNew(SBox).WidthOverride(170.f)[ IconButton(TEXT("I_Back"), TEXT("ย้อนกลับ"), FString(), [this]() {
			Toast.Reset();
			if (Step > 0) { --Step; Refresh(); } else ShowMain(); }, Panel) ] ];
	Foot->AddSlot().FillWidth(1.f)[ SNew(SSpacer) ];
	if (Step < 3)
		Foot->AddSlot().AutoWidth()
		[ SNew(SBox).WidthOverride(200.f)[ IconButton(TEXT("I_Next"), TEXT("ถัดไป"), FString(), [this]() {
			Toast.Reset();
			if (Step == 1 && NewName.TrimStartAndEnd().IsEmpty()) { Toast = TEXT("ใส่ชื่อก่อนนะ"); Refresh(); return; }
			++Step; Refresh(); }, Blue, false, bCanNext) ] ];
	else
		Foot->AddSlot().AutoWidth()
		[ SNew(SBox).WidthOverride(300.f)[ IconButton(TEXT("I_Check"), TEXT("เริ่มผจญภัย"), FString(), [this]() {
			Profile()->CreateCharacter(NewHero, NewName, NewElement);
			Page = EPage::Main; Toast.Reset(); Refresh(); }, Accent, true) ] ];

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(820.f)
			[
				Card(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Label(P->IsCreated() ? TEXT("แก้ไขตัวละคร") : TEXT("สร้างตัวละคร"), 44, FLinearColor::White, true) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 22)[ Dots ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 26)[ SNew(SBox).MinDesiredHeight(240.f)[ Body ] ]
					+ SVerticalBox::Slot().AutoHeight()[ Foot ],
				FLinearColor(0.f, 0.f, 0.f, .6f), 36.f)
			]
		];
}

// ================= LOBBY =================
TSharedRef<SWidget> UDFMenuWidget::BuildMain()
{
	UDFProfileSubsystem* P = Profile();
	FDFCharacterRow Hero;
	P->GetCharacter(P->GetSelected(), Hero);
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	const FLinearColor PC = P->GetPowerColor();

	FString Extra;
	{
		FDFAuraRow A; FDFSecretClassRow K;
		if (P->GetAura(P->GetEquippedAura(), A)) Extra += FString::Printf(TEXT("ออร่า %s"), *A.NameTH);
		if (P->GetSecretClass(P->GetEquippedClass(), K)) Extra += (Extra.IsEmpty() ? TEXT("") : TEXT("   ·   ")) + FString::Printf(TEXT("อาชีพ %s"), *K.NameTH);
	}

	TSharedRef<SVerticalBox> Menu = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
		[ IconButton(TEXT("I_Swords"), TEXT("เข้าสู้"), TEXT("เนื้อเรื่อง · ฝ่าคลื่นไม่รู้จบ · เลือกด่าน"), [this]() { Page = EPage::Fight; Toast.Reset(); Refresh(); }, Accent, true) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[ IconButton(TEXT("I_Bag"), TEXT("ตัวละครและสกิล"), TEXT("จัดสกิล · เปลี่ยนร่าง · เสียงพากย์"), [this]() { ShowHeroes(); }, Blue) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[ IconButton(TEXT("I_Sparkle"), TEXT("ออร่า"), TEXT("สุ่มออร่าหายาก"), [this]() { ShowAura(); }, FLinearColor(0.55f, 0.35f, 0.9f)) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[ IconButton(TEXT("I_Crown"), TEXT("อาชีพลับ"), TEXT("ปลดล็อกด้วยเงื่อนไขลับ"), [this]() { ShowClasses(); }, FLinearColor(0.75f, 0.55f, 0.12f)) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[ IconButton(TEXT("I_User"), TEXT("แก้ไขตัวละคร"), TEXT("ชื่อ · ร่าง · ธาตุ"), [this]() { ShowCreate(); }, Panel) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
		[ IconButton(TEXT("I_Gear"), TEXT("ตั้งค่า"), TEXT("เสียง · กราฟิก · เมาส์"), [this]() { Page = EPage::Settings; Toast.Reset(); Refresh(); }, FLinearColor(0.2f, 0.22f, 0.28f)) ]
		+ SVerticalBox::Slot().AutoHeight()
		[ IconButton(TEXT("I_Exit"), TEXT("ออกจากเกม"), FString(), [Self]() {
			if (Self.IsValid()) UKismetSystemLibrary::QuitGame(Self.Get(), Self->GetOwningPlayer(), EQuitPreference::Quit, false); }, FLinearColor(0.12f, 0.12f, 0.16f)) ];

	TSharedRef<SWidget> Wallet = Card(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
		[ SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)[ SNew(SImage).Image(Icon(TEXT("I_Coin"))).ColorAndOpacity(FSlateColor(Gold)) ] ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 24, 0)[ Label(FString::FromInt(P->GetGold()), 28, Gold, true) ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Label(FString::Printf(TEXT("ชนะ %d"), P->GetWins()), 22, FLinearColor::White, true) ],
		FLinearColor(0.f, 0.f, 0.f, .55f), 16.f);

	TSharedRef<SVerticalBox> PlateBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Label(P->GetPlayerName().IsEmpty() ? Hero.NameTH : P->GetPlayerName(), 46, PC, true) ]
		+ SVerticalBox::Slot().AutoHeight()[ Label(FString::Printf(TEXT("%s · %s  —  ธาตุ%s"), *Hero.NameTH, *Hero.TitleTH,
			*P->ElementNameTH(P->GetPlayerElement().IsNone() ? Hero.Element : P->GetPlayerElement())), 19, FLinearColor::White) ];
	if (!Extra.IsEmpty()) PlateBox->AddSlot().AutoHeight().Padding(0, 6, 0, 0)[ Label(Extra, 17, Gold) ];
	TSharedRef<SWidget> Plate = Card(PlateBox, FLinearColor(0.f, 0.f, 0.f, .5f), 22.f);

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(470.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(4, 0, 0, 18)[ Label(TEXT("วาดแล้วสู้"), 54, FLinearColor::White, true) ]
				+ SVerticalBox::Slot().AutoHeight()[ Menu ]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)[ Wallet ]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)[ SNew(SBox).MaxDesiredWidth(560.f)[ Plate ] ];
}

// ================= FIGHT SETUP =================
TSharedRef<SWidget> UDFMenuWidget::BuildFight()
{
	UDFProfileSubsystem* P = Profile();
	UDFSaveGame* S = P->Data();
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	if (!S) return Card(Label(TEXT("ไม่มีเซฟ"), 24, Accent));
	const bool bStory = S->Mode != FName(TEXT("endless"));
	S->PlayChapter = FMath::Clamp(S->PlayChapter, 1, S->Chapter);

	// mode cards
	TSharedRef<SHorizontalBox> Modes = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 10, 0)
		[ IconButton(TEXT("I_Crown"), TEXT("เนื้อเรื่อง"), FString::Printf(TEXT("บทที่ %d จาก %d ที่ปลดล็อก · จบด้วยบอส"), S->PlayChapter, S->Chapter),
			[this, S]() { S->Mode = TEXT("story"); Profile()->SaveNow(); Refresh(); }, bStory ? Accent : Panel) ]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[ IconButton(TEXT("I_Swords"), TEXT("ฝ่าคลื่นไม่รู้จบ"), FString::Printf(TEXT("สถิติ ล้ม %d ตัว · %d:%02d"), S->BestKills, (int32)S->BestTime / 60, (int32)S->BestTime % 60),
			[this, S]() { S->Mode = TEXT("endless"); Profile()->SaveNow(); Refresh(); }, !bStory ? Accent : Panel) ];

	TSharedRef<SHorizontalBox> ChapterRow = SNew(SHorizontalBox);
	if (bStory)
		ChapterRow->AddSlot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[ Button(TEXT("<"), [this, S]() { S->PlayChapter = FMath::Max(1, S->PlayChapter - 1); Refresh(); }, Panel, 22, S->PlayChapter > 1) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0)[ Label(FString::Printf(TEXT("บทที่ %d"), S->PlayChapter), 30, FLinearColor::White, true) ]
			+ SHorizontalBox::Slot().AutoWidth()[ Button(TEXT(">"), [this, S]() { S->PlayChapter = FMath::Min(S->Chapter, S->PlayChapter + 1); Refresh(); }, Panel, 22, S->PlayChapter < S->Chapter) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0)[ Label(FString::Printf(TEXT("ศัตรูแรงขึ้น %d%% · %d คลื่น"), 22 * (S->PlayChapter - 1), FMath::Min(3 + (S->PlayChapter - 1) / 3, 6)), 17, Dim) ]
		];

	// arenas
	TSharedRef<SWrapBox> Arenas = SNew(SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(10.f, 10.f));
	for (const FName& T : ADFArenaDressing::Themes())
	{
		const bool bSel = S->Arena == T;
		const FLinearColor C = ADFArenaDressing::ThemeColor(T);
		Arenas->AddSlot()
		[
			SNew(SBox).WidthOverride(200.f).HeightOverride(86.f)
			[
				SNew(SButton).ButtonStyle(&FlatStyle())
				.ButtonColorAndOpacity(bSel ? C : C * FLinearColor(.3f, .3f, .3f, .9f))
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.OnClicked_Lambda([this, S, T]() { S->Arena = T; Profile()->SaveNow(); Refresh(); return FReply::Handled(); })
				[ Label(ADFArenaDressing::ThemeNameTH(T), 22, FLinearColor::White, true) ]
			]
		];
	}

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(880.f)
			[
				Card(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(150.f)[ IconButton(TEXT("I_Back"), TEXT("กลับ"), FString(), [this]() { ShowMain(); }, Panel) ] ]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20, 0)[ Label(TEXT("เตรียมสู้"), 40, FLinearColor::White, true) ]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ Label(TEXT("โหมด"), 20, Dim, true) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)[ Modes ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)[ ChapterRow ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ Label(TEXT("ด่าน"), 20, Dim, true) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 24)[ Arenas ]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[ SNew(SBox).WidthOverride(320.f)[ IconButton(TEXT("I_Play"), TEXT("เริ่มสู้!"), ADFArenaDressing::ThemeNameTH(S->Arena), [Self]() {
						if (!Self.IsValid()) return;
						Self->Profile()->SaveNow();
						UGameplayStatics::OpenLevel(Self.Get(), Self->ArenaLevel); }, Accent, true) ] ],
				FLinearColor(0.f, 0.f, 0.f, .62f), 34.f)
			]
		];
}

// ================= SETTINGS =================
TSharedRef<SWidget> UDFMenuWidget::Slider(const FString& Name, float* Value, float Min, float Max)
{
	TWeakObjectPtr<UDFMenuWidget> Self(this);
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(SBox).WidthOverride(220.f)[ Label(Name, 22, FLinearColor::White, true) ] ]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SSlider).MinValue(Min).MaxValue(Max).Value(*Value)
			.SliderBarColor(FLinearColor(1, 1, 1, .25f)).SliderHandleColor(Accent)
			.OnValueChanged_Lambda([Value](float V) { *Value = V; })
			.OnMouseCaptureEnd_Lambda([Self]() { if (Self.IsValid() && Self->Profile()) Self->Profile()->SaveNow(); })
		];
}

TSharedRef<SWidget> UDFMenuWidget::BuildSettings()
{
	UDFProfileSubsystem* P = Profile();
	UDFSaveGame* S = P->Data();
	if (!S) return Card(Label(TEXT("ไม่มีเซฟ"), 24, Accent));
	static const TCHAR* Q[4] = { TEXT("ต่ำ"), TEXT("กลาง"), TEXT("สูง"), TEXT("สุด") };
	TSharedRef<SHorizontalBox> Quality = SNew(SHorizontalBox);
	for (int32 I = 0; I < 4; ++I)
		Quality->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
		[ Button(Q[I], [this, S, I]() { S->Quality = I; Profile()->ApplySettings(); Profile()->SaveNow(); Refresh(); }, S->Quality == I ? Accent : Panel, 20) ];

	return SNew(SOverlay)
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(760.f)
			[
				Card(SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 22)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(150.f)[ IconButton(TEXT("I_Back"), TEXT("กลับ"), FString(), [this]() { Profile()->SaveNow(); ShowMain(); }, Panel) ] ]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(20, 0)[ Label(TEXT("ตั้งค่า"), 40, FLinearColor::White, true) ]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[ Slider(TEXT("เสียงรวม"), &S->MasterVolume, 0.f, 1.f) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[ Slider(TEXT("เพลง"), &S->MusicVolume, 0.f, 1.f) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[ Slider(TEXT("เสียงเอฟเฟกต์"), &S->SfxVolume, 0.f, 1.f) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[ Slider(TEXT("ความไวเมาส์"), &S->MouseSens, .3f, 2.5f) ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 18, 0, 8)[ Label(TEXT("คุณภาพกราฟิก"), 22, FLinearColor::White, true) ]
					+ SVerticalBox::Slot().AutoHeight()[ Quality ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 22, 0, 0)[ Label(TEXT("ปุ่ม: WASD เดิน · คลิกซ้าย/J ตี · 1 2 3 สกิล · R ท่าไม้ตาย · Shift หลบ · Space กระโดด · T ออโต้ · ล้อเมาส์ ซูม"), 17, Dim) ],
				FLinearColor(0.f, 0.f, 0.f, .62f), 34.f)
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
