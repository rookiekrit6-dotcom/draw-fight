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
	UPROPERTY(SaveGame) TMap<FName, FDFLoadout> Loadouts;
	UPROPERTY(SaveGame) int32 Wins = 0;
	UPROPERTY(SaveGame) int32 Losses = 0;
};
