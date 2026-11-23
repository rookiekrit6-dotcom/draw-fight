// DFAudio.h — tiny sound helper: one-shot effects and looping music from /Game/DrawFight/Audio,
// scaled by the volumes in the player's settings.
#pragma once

#include "CoreMinimal.h"

class UAudioComponent;

namespace DFAudio
{
	// Name = asset name, e.g. "S_Boom". Loc zero = 2D.
	void Sfx(const UObject* Ctx, const TCHAR* Name, FVector Loc = FVector::ZeroVector, float Volume = 1.f, float Pitch = 1.f);
	UAudioComponent* Music(const UObject* Ctx, const TCHAR* Name);
}
