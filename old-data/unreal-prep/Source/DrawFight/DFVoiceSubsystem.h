// DFVoiceSubsystem.h — player-made voice packs. Players record their own lines
// (any recorder, export as 16-bit PCM .wav) and drop a folder into
//   <Saved>/VoicePacks/<PackName>/voice.json + .wav files
// then pick the pack for a hero in the menu. Lines play on game events with a
// subtitle. No plugin needed (WAV is decoded here). See VOICE_PACKS.md.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DFVoiceSubsystem.generated.h"

UENUM(BlueprintType)
enum class EDFVoiceEvent : uint8
{
	Select, FightStart, Attack, Skill, Ultimate, Hurt, LowHP, Dash, Kill, BossAppear, Win, Lose, RareAura, MAX UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FDFVoiceLine
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString File;   // absolute path to the .wav
	UPROPERTY(BlueprintReadOnly) FString Text;   // subtitle, may be empty
};

USTRUCT(BlueprintType)
struct FDFVoicePack
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Id;      // folder name
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString Author;
	UPROPERTY(BlueprintReadOnly) FName ForHero;   // suggested hero, may be None
	TMap<EDFVoiceEvent, TArray<FDFVoiceLine>> Lines;
	int32 LineCount() const { int32 N = 0; for (const auto& P : Lines) N += P.Value.Num(); return N; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDFOnSubtitle, const FString&, Text, float, Seconds);

UCLASS()
class UDFVoiceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	static UDFVoiceSubsystem* Get(const UObject* WorldContext);

	UPROPERTY(BlueprintAssignable) FDFOnSubtitle OnSubtitle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice") float Volume = 1.f;

	UFUNCTION(BlueprintPure, Category = "Voice") static FString PacksFolder();
	UFUNCTION(BlueprintCallable, Category = "Voice") void ScanPacks();
	UFUNCTION(BlueprintPure, Category = "Voice") TArray<FString> GetPackIds() const;
	UFUNCTION(BlueprintPure, Category = "Voice") FString PackLabel(const FString& Id) const; // "name · author · N lines"
	UFUNCTION(BlueprintCallable, Category = "Voice") void AssignPack(FName Hero, const FString& PackId); // "" = none
	UFUNCTION(BlueprintPure, Category = "Voice") FString GetAssignedPack(FName Hero) const;
	UFUNCTION(BlueprintCallable, Category = "Voice") FString CycleAssignedPack(FName Hero);
	// Opens the packs folder in Explorer so players can drop files in
	UFUNCTION(BlueprintCallable, Category = "Voice") void OpenPacksFolder() const;

	// Plays a random line of the hero's pack for this event (respects per-event cooldowns)
	UFUNCTION(BlueprintCallable, Category = "Voice", meta = (WorldContext = "WorldContext"))
	bool Play(const UObject* WorldContext, FName Hero, EDFVoiceEvent Event);

private:
	struct FPcm { TArray<uint8> Data; int32 Rate = 0; int32 Channels = 0; float Seconds = 0.f; };
	TMap<FString, FDFVoicePack> Packs;
	TMap<FName, FString> Assigned;
	TMap<FString, FPcm> Cache;
	double NextAllowed[(int32)EDFVoiceEvent::MAX] = {};
	double AnyUntil = 0.0;     // a line is still playing

	bool LoadWav(const FString& Path, FPcm& Out);
	void LoadAssignments();
	void SaveAssignments() const;
	static bool ParseEvent(const FString& Key, EDFVoiceEvent& Out);
};
