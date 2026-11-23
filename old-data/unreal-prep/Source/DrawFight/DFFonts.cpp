#include "DFFonts.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

FSlateFontInfo FDFFonts::Get(int32 Size, bool bBold)
{
	static TSharedPtr<FCompositeFont> Font;
	static bool bTried = false;
	if (!bTried)
	{
		bTried = true;
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("DrawFight/Fonts/"));
		const FString Reg = Dir + TEXT("BaiJamjuree-Regular.ttf");
		const FString Bold = Dir + TEXT("BaiJamjuree-Bold.ttf");
		if (FPaths::FileExists(Reg))
		{
			Font = MakeShared<FStandaloneCompositeFont>(FName(TEXT("Regular")), Reg, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			if (FPaths::FileExists(Bold))
				Font->DefaultTypeface.AppendFont(FName(TEXT("Bold")), Bold, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DrawFight: Thai font not found at %s"), *Reg);
		}
	}
	if (Font.IsValid()) return FSlateFontInfo(Font, Size, bBold ? FName(TEXT("Bold")) : FName(TEXT("Regular")));
	return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
}
