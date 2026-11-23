// DFSaveGame.h — everything the player owns, written to Saved/SaveGames/DrawFight.sav
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DFGacha.h"
#include "DFSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FDFLoadout
{
	GENERATED_BODY()
	// Exactly 3 skill slots (DT_Skills row names)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FName> Skills;
};

UCLASS()
class UDFSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY(SaveGame) int32 Version = 1;
	UPROPERTY(SaveGame) FDFGachaState Gacha;          // gold, pity, skill card levels
	UPROPERTY(SaveGame) TArray<FName> Unlocked;       // DT_Characters rows
	UPROPERTY(SaveGame) FName Selected;
	// character creation (must be done once before playing)
	UPROPERTY(SaveGame) bool bCreated = false;
	UPROPERTY(SaveGame) FString PlayerName;
	UPROPERTY(SaveGame) FName Element;              // chosen power colour (DT_Elements)
	// what the next fight is
	UPROPERTY(SaveGame) FName Arena = TEXT("lab");
	UPROPERTY(SaveGame) FName Mode = TEXT("story");  // story | endless
	UPROPERTY(SaveGame) int32 Chapter = 1;          // highest story chapter unlocked
	UPROPERTY(SaveGame) int32 PlayChapter = 1;
	UPROPERTY(SaveGame) int32 BestKills = 0;        // endless record
	UPROPERTY(SaveGame) float BestTime = 0.f;
	// settings
	UPROPERTY(SaveGame) float MasterVolume = 1.f;
	UPROPERTY(SaveGame) float MusicVolume = .6f;
	UPROPERTY(SaveGame) float SfxVolume = 1.f;
	UPROPERTY(SaveGame) float MouseSens = 1.f;
	UPROPERTY(SaveGame) int32 Quality = 2;          // 0 low .. 3 epic
	UPROPERTY(SaveGame) TMap<FName, FDFLoadout> Loadouts;
	UPROPERTY(SaveGame) int32 Wins = 0;
	UPROPERTY(SaveGame) int32 Losses = 0;

	// auras
	UPROPERTY(SaveGame) TMap<FName, int32> Auras;     // how many times each was rolled
	UPROPERTY(SaveGame) FName EquippedAura;
	UPROPERTY(SaveGame) int32 TotalRolls = 0;

	// secret classes
	UPROPERTY(SaveGame) TArray<FName> Classes;        // unlocked
	UPROPERTY(SaveGame) FName EquippedClass;

	// counters for unlock conditions
	UPROPERTY(SaveGame) int32 Kills = 0;
	UPROPERTY(SaveGame) int32 BossKills = 0;
	UPROPERTY(SaveGame) int32 NoHitWins = 0;
	UPROPERTY(SaveGame) int32 LowHpWins = 0;
	UPROPERTY(SaveGame) int32 Draws = 0;
	UPROPERTY(SaveGame) TMap<FName, int32> WeaponKills;
};
