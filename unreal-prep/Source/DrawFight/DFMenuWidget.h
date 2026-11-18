// DFMenuWidget.h — the whole front-end in code: main menu, hero select (+ skill slots),
// aura rolling and secret classes. No UMG layout needed: add it to the viewport (DFMenuPlayerController does).
// If Thai text shows as boxes, import a Thai .ttf (e.g. Bai Jamjuree / Noto Sans Thai)
// as a Font asset and set ThaiFont on a Blueprint child of this widget.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DFGacha.h"
#include "DFMenuWidget.generated.h"

class SBox;
class UFont;
class UDFProfileSubsystem;
class ADFMenuStage;
class UTexture2D;
struct FSlateBrush;

UCLASS()
class UDFMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Level opened by "เริ่มสู้" (must use DFArenaGameMode)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu") FName ArenaLevel = TEXT("Lvl_ThirdPerson");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu") TObjectPtr<UFont> ThaiFont;

	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowMain();   // lobby (or creation if no character yet)
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowCreate();
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowHeroes();
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowAura();
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowClasses();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	enum class EPage : uint8 { Title, Create, Main, Heroes, Aura, Classes };
	EPage Page = EPage::Title;
	// creation wizard
	int32 Step = 0;
	FName NewHero, NewElement;
	FString NewName;
	// icon textures (kept alive) + their brushes
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> IconTex;
	TMap<FName, TSharedPtr<FSlateBrush>> Icons;
	const FSlateBrush* Icon(const TCHAR* Name);
	ADFMenuStage* Stage() const;
	void SyncStage();
	FName Viewing;                         // hero shown on the right of the hero page
	TArray<FName> LastRolls;
	FString Toast;                         // one-line message (not enough gold, etc.)

	TSharedPtr<SBox> Root;
	void Refresh();
	UDFProfileSubsystem* Profile() const;

	TSharedRef<SWidget> BuildTitle();
	TSharedRef<SWidget> BuildCreate();
	TSharedRef<SWidget> BuildMain();
	TSharedRef<SWidget> Card(TSharedRef<SWidget> Inner, FLinearColor Tint = FLinearColor(0.f, 0.f, 0.f, .62f), float Pad = 28.f) const;
	TSharedRef<SWidget> IconButton(const TCHAR* IconName, const FString& Text, const FString& Sub, TFunction<void()> OnClick, FLinearColor Color, bool bBig = false, bool bEnabled = true);
	TSharedRef<SWidget> Wrap(TSharedRef<SWidget> Page) const; // dark panel for the old full pages
	TSharedRef<SWidget> BuildHeroes();
	TSharedRef<SWidget> BuildAura();
	TSharedRef<SWidget> BuildClasses();
	TSharedRef<SWidget> TopBar(const FText& Title);

	FSlateFontInfo Font(int32 Size, bool bBold = false) const;
	TSharedRef<SWidget> Label(const FString& Text, int32 Size, FLinearColor Color = FLinearColor::White, bool bBold = false) const;
	TSharedRef<SWidget> Button(const FString& Text, TFunction<void()> OnClick, FLinearColor Color, int32 Size = 22, bool bEnabled = true);
};
