// DFHudWidget.h — fight HUD in code (no UMG layout): player HP, wave + enemies left,
// boss bar, skill slots, floating damage/heal numbers, enemy HP bars over heads,
// wave banners and the win/lose screen. DFArenaGameMode adds it automatically.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DFHudWidget.generated.h"

class SBox;
class UFont;
class UDFCombatComponent;

UCLASS()
class UDFHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD") TObjectPtr<UFont> ThaiFont;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD") bool bShowEnemyBars = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD") bool bShowNumbers = true;

	UFUNCTION() void HandleWave(int32 Wave, bool bBossWave);
	UFUNCTION() void HandleEnd(bool bVictory);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geo, float Dt) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Cull,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	struct FFloat { FVector World; FString Text; FLinearColor Color; float Age = 0.f; float Size = 22.f; };
	TArray<FFloat> Floats;
	TMap<TWeakObjectPtr<UDFCombatComponent>, float> LastHP;

	FString Banner;
	FLinearColor BannerColor = FLinearColor::White;
	float BannerLeft = 0.f;
	bool bEnded = false;

	TSharedPtr<SBox> EndBox;
	UDFCombatComponent* PlayerCombat() const;
	UDFCombatComponent* Boss() const;
	FSlateFontInfo Font(int32 Size) const;
	TSharedRef<SWidget> Label(TAttribute<FText> Text, int32 Size, FLinearColor Color) const;
	TSharedRef<SWidget> Bar(TAttribute<TOptional<float>> Pct, FLinearColor Color, float W, float H) const;
};
