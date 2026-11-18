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

UCLASS()
class UDFMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Level opened by "เริ่มสู้" (must use DFArenaGameMode)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu") FName ArenaLevel = TEXT("ThirdPersonMap");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu") TObjectPtr<UFont> ThaiFont;

	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowMain();
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowHeroes();
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowAura();
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowClasses();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	enum class EPage : uint8 { Main, Heroes, Aura, Classes };
	EPage Page = EPage::Main;
	FName Viewing;                         // hero shown on the right of the hero page
	TArray<FName> LastRolls;
	FString Toast;                         // one-line message (not enough gold, etc.)

	TSharedPtr<SBox> Root;
	void Refresh();
	UDFProfileSubsystem* Profile() const;

	TSharedRef<SWidget> BuildMain();
	TSharedRef<SWidget> BuildHeroes();
	TSharedRef<SWidget> BuildAura();
	TSharedRef<SWidget> BuildClasses();
	TSharedRef<SWidget> TopBar(const FText& Title);

	FSlateFontInfo Font(int32 Size, bool bBold = false) const;
	TSharedRef<SWidget> Label(const FString& Text, int32 Size, FLinearColor Color = FLinearColor::White, bool bBold = false) const;
	TSharedRef<SWidget> Button(const FString& Text, TFunction<void()> OnClick, FLinearColor Color, int32 Size = 22, bool bEnabled = true);
};
