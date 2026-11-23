#include "DFGacha.h"

static int32 RarityRank(EDFRarity R)
{
	switch (R) { case EDFRarity::Common: return 1; case EDFRarity::Rare: return 2; case EDFRarity::Legendary: return 3; default: return 0; }
}
static bool ListHas(const FString& List, FName Key)
{
	TArray<FString> Parts; List.ParseIntoArray(Parts, TEXT(";"), true);
	return Parts.Contains(Key.ToString());
}
// Melee / ranged weapon class groups (GA_GRP in the 2D game)
static EDFGachaGroup ClassGroup(FName Class)
{
	static const TSet<FName> Melee = { "sword","spear","hammer","dagger","fist","whip","shield" };
	static const TSet<FName> Ranged = { "bow","staff","bomb","knife","ring","gun","spray" };
	if (Melee.Contains(Class)) return EDFGachaGroup::Melee;
	if (Ranged.Contains(Class)) return EDFGachaGroup::Ranged;
	return EDFGachaGroup::Any;
}

float UDFGacha::FitMultiplier(const UDataTable* Skills, FName Skill, FName HeroClass, FName HeroElement)
{
	const FDFSkillRow* Row = Skills ? Skills->FindRow<FDFSkillRow>(Skill, TEXT("Gacha")) : nullptr;
	if (!Row) return 1.f;
	float M = 1.f;
	if (ListHas(Row->SignatureOfClass, HeroClass)) M = 4.f;
	else if (Row->GachaGroup != EDFGachaGroup::Any && Row->GachaGroup == ClassGroup(HeroClass)) M = 2.f;
	if (ListHas(Row->FavoredElements, HeroElement)) M *= 3.f;
	return M;
}

float UDFGacha::Weight(const UDataTable* Skills, FName Skill, FName HeroClass, FName HeroElement)
{
	const FDFSkillRow* Row = Skills ? Skills->FindRow<FDFSkillRow>(Skill, TEXT("Gacha")) : nullptr;
	if (!Row) return 0.f;
	const int32 R = RarityRank(Row->Rarity);
	if (R == 0) return 0.f; // signature skills are never pulled
	const float Base = R == 1 ? 60.f : R == 2 ? 30.f : 10.f;
	return Base * FitMultiplier(Skills, Skill, HeroClass, HeroElement);
}

TArray<FDFGachaResult> UDFGacha::Pull(const UDataTable* Skills, FDFGachaState& State, int32 Count,
	FName HeroClass, FName HeroElement, int32 RandomSeed)
{
	TArray<FDFGachaResult> Out;
	if (!Skills || (Count != 1 && Count != 10)) return Out;
	const int32 Cost = Count == 10 ? CostTen : CostSingle;
	if (State.Gold < Cost) return Out;
	State.Gold -= Cost;

	static const int32 Refund[4] = { 0, 25, 60, 150 };
	FRandomStream Rng(RandomSeed);
	bool bGood = false;
	const TArray<FName> Names = Skills->GetRowNames();

	for (int32 i = 0; i < Count; ++i)
	{
		State.PityCounter++;
		const int32 MinRank = State.PityCounter >= PityPulls ? 3 : (Count == 10 && i == 9 && !bGood) ? 2 : 1;

		float Total = 0.f;
		TArray<TPair<FName, float>> Pool;
		for (const FName& N : Names)
		{
			const FDFSkillRow* Row = Skills->FindRow<FDFSkillRow>(N, TEXT("Gacha"));
			if (!Row || RarityRank(Row->Rarity) < MinRank) continue;
			const float W = Weight(Skills, N, HeroClass, HeroElement);
			if (W > 0.f) { Pool.Add({ N, W }); Total += W; }
		}
		if (Pool.Num() == 0) break;

		float Roll = Rng.FRand() * Total;
		FName Pick = Pool.Last().Key;
		for (const auto& P : Pool) { Roll -= P.Value; if (Roll <= 0.f) { Pick = P.Key; break; } }

		const FDFSkillRow* Row = Skills->FindRow<FDFSkillRow>(Pick, TEXT("Gacha"));
		const int32 R = RarityRank(Row->Rarity);
		if (R >= 2) bGood = true;
		if (R == 3) State.PityCounter = 0;

		FDFGachaResult Res;
		Res.Skill = Pick;
		Res.Rarity = Row->Rarity;
		Res.bFitsHero = FitMultiplier(Skills, Pick, HeroClass, HeroElement) >= 3.f;
		int32& Lv = State.SkillLevels.FindOrAdd(Pick);
		if (Lv < MaxLevel) { Lv++; Res.NewLevel = Lv; }
		else { Res.Refund = Refund[R]; State.Gold += Res.Refund; }
		Out.Add(Res);
	}
	return Out;
}
