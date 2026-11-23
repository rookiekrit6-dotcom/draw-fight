// DFGacha.h — gacha rules ported 1:1 from the 2D game (see data/gacha_and_rules.json).
// Pure logic, no UI. Weight = base(rarity) * fit, fit = 4 (own class signature)
// or 2 (same melee/ranged group), then x3 if the hero's element favors the skill.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DFDataTypes.h"
#include "DFGacha.generated.h"

USTRUCT(BlueprintType)
struct FDFGachaState
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Gold = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 PityCounter = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, int32> SkillLevels; // 0..3
};

USTRUCT(BlueprintType)
struct FDFGachaResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FName Skill;
	UPROPERTY(BlueprintReadOnly) EDFRarity Rarity = EDFRarity::Common;
	UPROPERTY(BlueprintReadOnly) int32 NewLevel = 0;   // 0 when duplicate at max
	UPROPERTY(BlueprintReadOnly) int32 Refund = 0;
	UPROPERTY(BlueprintReadOnly) bool bFitsHero = false; // fit >= 3
};

UCLASS()
class UDFGacha : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static constexpr int32 CostSingle = 60;
	static constexpr int32 CostTen = 540;
	static constexpr int32 PityPulls = 30;
	static constexpr int32 MaxLevel = 3;

	UFUNCTION(BlueprintPure, Category = "DrawFight|Gacha")
	static float FitMultiplier(const UDataTable* Skills, FName Skill, FName HeroClass, FName HeroElement);

	UFUNCTION(BlueprintPure, Category = "DrawFight|Gacha")
	static float Weight(const UDataTable* Skills, FName Skill, FName HeroClass, FName HeroElement);

	// Returns empty array if not enough gold. Count must be 1 or 10.
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Gacha")
	static TArray<FDFGachaResult> Pull(const UDataTable* Skills, UPARAM(ref) FDFGachaState& State,
		int32 Count, FName HeroClass, FName HeroElement, int32 RandomSeed);
};
