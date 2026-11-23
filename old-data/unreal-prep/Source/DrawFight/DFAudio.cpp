#include "DFAudio.h"
#include "DFProfileSubsystem.h"
#include "DFSaveGame.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	USoundBase* Load(const TCHAR* Name)
	{
		static TMap<FString, USoundBase*> Cache;
		if (USoundBase** S = Cache.Find(Name)) return *S;
		USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/DrawFight/Audio/%s.%s"), Name, Name));
		if (S) S->AddToRoot();
		Cache.Add(Name, S);
		return S;
	}
	void Vols(const UObject* Ctx, float& Master, float& Sfx, float& Mus)
	{
		Master = 1.f; Sfx = 1.f; Mus = .6f;
		if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(Ctx))
			if (UDFSaveGame* S = P->Data()) { Master = S->MasterVolume; Sfx = S->SfxVolume; Mus = S->MusicVolume; }
	}
}

void DFAudio::Sfx(const UObject* Ctx, const TCHAR* Name, FVector Loc, float Volume, float Pitch)
{
	USoundBase* S = Load(Name);
	if (!S || !Ctx) return;
	float M, X, Mu; Vols(Ctx, M, X, Mu);
	const float V = Volume * M * X;
	if (V <= .01f) return;
	if (Loc.IsZero()) UGameplayStatics::PlaySound2D(Ctx, S, V, Pitch);
	else UGameplayStatics::PlaySoundAtLocation(Ctx, S, Loc, V, Pitch);
}

UAudioComponent* DFAudio::Music(const UObject* Ctx, const TCHAR* Name)
{
	USoundBase* S = Load(Name);
	if (!S || !Ctx) return nullptr;
	float M, X, Mu; Vols(Ctx, M, X, Mu);
	return UGameplayStatics::SpawnSound2D(Ctx, S, FMath::Max(.001f, M * Mu), 1.f, 0.f, nullptr, true);
}
