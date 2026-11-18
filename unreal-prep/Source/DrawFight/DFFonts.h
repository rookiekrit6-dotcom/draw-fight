// DFFonts.h — Thai-capable UI font loaded straight from .ttf files (no font asset needed).
// Files: Content/DrawFight/Fonts/BaiJamjuree-Regular.ttf and -Bold.ttf (Thai + Latin, OFL)
#pragma once
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

struct FDFFonts
{
	static FSlateFontInfo Get(int32 Size, bool bBold);
};
