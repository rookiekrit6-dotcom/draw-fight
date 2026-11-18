#include "DFProfileSubsystem.h"
#include "DFSaveGame.h"
#include "DFCombatComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

const TCHAR* UDFProfileSubsystem::SlotName = TEXT("DrawFight");

static UDataTable* LoadTable(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Game/DrawFight/Data/%s.%s"), Name, Name);
	UDataTable* T = LoadObject<UDataTable>(nullptr, *Path);
	if (!T) UE_LOG(LogTemp, Warning, TEXT("DrawFight: data table not found at %s"), *Path);
	return T;
}

void UDFProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Characters = LoadTable(TEXT("DT_Characters"));
	Skills = LoadTable(TEXT("DT_Skills"));
	Weapons = LoadTable(TEXT("DT_WeaponClasses"));
	Elements = LoadTable(TEXT("DT_Elements"));
	AuraTable = LoadTable(TEXT("DT_Auras"));
	ClassTable = LoadTable(TEXT("DT_SecretClasses"));

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		Save = Cast<UDFSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save)
	{
		Save = Cast<UDFSaveGame>(UGameplayStatics::CreateSaveGameObject(UDFSaveGame::StaticClass()));
		Save->Gacha.Gold = StartGold;
	}
	EnsureDefaults();
	SaveNow();
}

UDFProfileSubsystem* UDFProfileSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UDFProfileSubsystem>() : nullptr;
}

void UDFProfileSubsystem::EnsureDefaults()
{
	if (!Characters) return;
	for (const FName& Id : Characters->GetRowNames())
	{
		const FDFCharacterRow* R = Characters->FindRow<FDFCharacterRow>(Id, TEXT("DF"));
		if (R && R->UnlockCost <= 0) Save->Unlocked.AddUnique(Id);
	}
	if (Save->Selected.IsNone() || !Save->Unlocked.Contains(Save->Selected))
		Save->Selected = Save->Unlocked.Num() ? Save->Unlocked[0] : NAME_None;
	for (const FName& Id : Save->Unlocked) GetLoadout(Id); // creates missing loadouts
}

void UDFProfileSubsystem::Changed()
{
	SaveNow();
	OnProfileChanged.Broadcast();
}

void UDFProfileSubsystem::SaveNow()
{
	if (Save) UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}

void UDFProfileSubsystem::ResetProfile()
{
	Save = Cast<UDFSaveGame>(UGameplayStatics::CreateSaveGameObject(UDFSaveGame::StaticClass()));
	Save->Gacha.Gold = StartGold;
	EnsureDefaults();
	Changed();
}

// ---------- data ----------
TArray<FName> UDFProfileSubsystem::GetCharacterIds() const
{
	return Characters ? Characters->GetRowNames() : TArray<FName>();
}

bool UDFProfileSubsystem::GetCharacter(FName Id, FDFCharacterRow& Out) const
{
	const FDFCharacterRow* R = Characters ? Characters->FindRow<FDFCharacterRow>(Id, TEXT("DF"), false) : nullptr;
	if (R) Out = *R;
	return R != nullptr;
}

bool UDFProfileSubsystem::IsCreated() const { return Save && Save->bCreated; }
FString UDFProfileSubsystem::GetPlayerName() const { return Save ? Save->PlayerName : FString(); }
FName UDFProfileSubsystem::GetPlayerElement() const { return Save ? Save->Element : NAME_None; }

void UDFProfileSubsystem::CreateCharacter(FName Hero, const FString& Name, FName Element)
{
	if (!Save) return;
	if (!Hero.IsNone())
	{
		if (!Save->Unlocked.Contains(Hero)) Save->Unlocked.Add(Hero);
		Save->Selected = Hero;
	}
	Save->PlayerName = Name.TrimStartAndEnd().Left(16);
	Save->Element = Element;
	Save->bCreated = true;
	SaveNow();
}

TArray<FName> UDFProfileSubsystem::GetElementIds() const
{
	TArray<FName> Out;
	if (Elements)
		for (const FName& N : Elements->GetRowNames())
			if (N != FName(TEXT("none")) && N != FName(TEXT("inf"))) Out.Add(N);
	return Out;
}

FLinearColor UDFProfileSubsystem::ElementColor(FName Id) const
{
	const FDFElementRow* R = Elements ? Elements->FindRow<FDFElementRow>(Id, TEXT("DF"), false) : nullptr;
	return R && R->Color.StartsWith(TEXT("#")) ? FLinearColor(FColor::FromHex(R->Color)) : FLinearColor(1.f, .45f, .15f);
}

FLinearColor UDFProfileSubsystem::GetPowerColor() const
{
	if (Save && !Save->Element.IsNone()) return ElementColor(Save->Element);
	FDFCharacterRow H;
	if (GetCharacter(GetSelected(), H) && H.Color.StartsWith(TEXT("#"))) return FLinearColor(FColor::FromHex(H.Color));
	return FLinearColor(1.f, .45f, .15f);
}

TArray<FName> UDFProfileSubsystem::GetAllSkillIds() const
{
	return Skills ? Skills->GetRowNames() : TArray<FName>();
}

bool UDFProfileSubsystem::GetSkill(FName Id, FDFSkillRow& Out) const
{
	const FDFSkillRow* R = Skills ? Skills->FindRow<FDFSkillRow>(Id, TEXT("DF"), false) : nullptr;
	if (R) Out = *R;
	return R != nullptr;
}

FString UDFProfileSubsystem::SkillNameTH(FName Id) const
{
	const FDFSkillRow* R = Skills ? Skills->FindRow<FDFSkillRow>(Id, TEXT("DF"), false) : nullptr;
	return R ? R->NameTH : Id.ToString();
}

FString UDFProfileSubsystem::ElementNameTH(FName Id) const
{
	const FDFElementRow* R = Elements ? Elements->FindRow<FDFElementRow>(Id, TEXT("DF"), false) : nullptr;
	return R ? R->NameTH : Id.ToString();
}

FString UDFProfileSubsystem::WeaponNameTH(FName Id) const
{
	const FDFWeaponClassRow* R = Weapons ? Weapons->FindRow<FDFWeaponClassRow>(Id, TEXT("DF"), false) : nullptr;
	return R ? R->NameTH : Id.ToString();
}

FName UDFProfileSubsystem::WeaponSignature(FName WeaponClass) const
{
	const FDFWeaponClassRow* R = Weapons ? Weapons->FindRow<FDFWeaponClassRow>(WeaponClass, TEXT("DF"), false) : nullptr;
	return R ? R->SignatureSkill : NAME_None;
}

// ---------- profile ----------
int32 UDFProfileSubsystem::GetGold() const { return Save ? Save->Gacha.Gold : 0; }
int32 UDFProfileSubsystem::GetPity() const { return Save ? Save->Gacha.PityCounter : 0; }
int32 UDFProfileSubsystem::GetWins() const { return Save ? Save->Wins : 0; }
FName UDFProfileSubsystem::GetSelected() const { return Save ? Save->Selected : NAME_None; }
bool UDFProfileSubsystem::IsUnlocked(FName Id) const { return Save && Save->Unlocked.Contains(Id); }

void UDFProfileSubsystem::AddGold(int32 Amount)
{
	if (!Save) return;
	Save->Gacha.Gold = FMath::Max(0, Save->Gacha.Gold + Amount);
	Changed();
}

bool UDFProfileSubsystem::Unlock(FName Id)
{
	FDFCharacterRow R;
	if (!Save || !GetCharacter(Id, R) || IsUnlocked(Id) || Save->Gacha.Gold < R.UnlockCost) return false;
	Save->Gacha.Gold -= R.UnlockCost;
	Save->Unlocked.Add(Id);
	GetLoadout(Id);
	Changed();
	return true;
}

bool UDFProfileSubsystem::Select(FName Id)
{
	if (!IsUnlocked(Id)) return false;
	Save->Selected = Id;
	Changed();
	return true;
}

// ---------- skills ----------
int32 UDFProfileSubsystem::GetSkillLevel(FName Skill) const
{
	const int32* L = Save ? Save->Gacha.SkillLevels.Find(Skill) : nullptr;
	return L ? *L : 0;
}

TArray<FName> UDFProfileSubsystem::GetUsableSkills(FName Hero) const
{
	TArray<FName> Out;
	FDFCharacterRow R;
	if (GetCharacter(Hero, R))
	{
		if (!R.Skill1.IsNone()) Out.AddUnique(R.Skill1);
		if (!R.Skill2.IsNone()) Out.AddUnique(R.Skill2);
		const FName Sig = WeaponSignature(R.WeaponClass);
		if (!Sig.IsNone()) Out.AddUnique(Sig);
		// starter skills so every hero can fill 3 slots even before owning cards
		static const FName Starters[] = { TEXT("haste"), TEXT("barrier"), TEXT("blast") };
		for (const FName& S : Starters) if (Out.Num() < SlotCount) Out.AddUnique(S);
	}
	if (Save)
		for (const TPair<FName, int32>& P : Save->Gacha.SkillLevels)
			if (P.Value > 0) Out.AddUnique(P.Key);
	return Out;
}

TArray<FName> UDFProfileSubsystem::GetLoadout(FName Hero) const
{
	if (!Save) return {};
	FDFCharacterRow HeroRow;
	const bool bHero = GetCharacter(Hero, HeroRow);
	if (FDFLoadout* L = Save->Loadouts.Find(Hero))
		if (L->Skills.Num() == SlotCount)
		{
			if (bHero && L->Skills[0] != HeroRow.Skill1) // keep slot 0 = hero's own skill
			{
				L->Skills.Remove(HeroRow.Skill1);
				L->Skills.Insert(HeroRow.Skill1, 0);
				L->Skills.SetNum(SlotCount);
			}
			for (FName& Slot : L->Skills) // fill empty slots
				if (Slot.IsNone())
					for (const FName& U : GetUsableSkills(Hero))
						if (!L->Skills.Contains(U)) { Slot = U; break; }
			return L->Skills;
		}
	// Default: own 2 skills + weapon signature
	TArray<FName> Def = GetUsableSkills(Hero);
	Def.SetNum(FMath::Min(Def.Num(), SlotCount));
	while (Def.Num() < SlotCount) Def.Add(NAME_None);
	Save->Loadouts.Add(Hero).Skills = Def; // Save is a UObject pointer, so this is allowed from const
	return Def;
}

bool UDFProfileSubsystem::SetLoadoutSlot(FName Hero, int32 Slot, FName Skill)
{
	if (!Save || Slot < 1 || Slot >= SlotCount || !IsUnlocked(Hero)) return false; // slot 0 = hero's own skill, locked
	if (!Skill.IsNone() && !GetUsableSkills(Hero).Contains(Skill)) return false;
	TArray<FName> L = GetLoadout(Hero);
	const int32 Other = L.IndexOfByKey(Skill);
	if (Other == 0) return false; // never move the locked skill
	if (!Skill.IsNone() && Other != INDEX_NONE && Other != Slot) L[Other] = L[Slot]; // swap
	L[Slot] = Skill;
	Save->Loadouts.FindOrAdd(Hero).Skills = L;
	Changed();
	return true;
}

FName UDFProfileSubsystem::CycleLoadoutSlot(FName Hero, int32 Slot)
{
	const TArray<FName> Usable = GetUsableSkills(Hero);
	const TArray<FName> L = GetLoadout(Hero);
	if (!Usable.Num() || Slot < 1 || Slot >= L.Num()) return NAME_None;
	int32 I = Usable.IndexOfByKey(L[Slot]);
	for (int32 Step = 0; Step < Usable.Num(); ++Step)
	{
		I = (I + 1) % Usable.Num();
		if (!L.Contains(Usable[I]))
		{
			SetLoadoutSlot(Hero, Slot, Usable[I]);
			return Usable[I];
		}
	}
	return L[Slot]; // nothing free to swap in
}

// ---------- gacha ----------
TArray<FDFGachaResult> UDFProfileSubsystem::Pull(int32 Count)
{
	FDFCharacterRow R;
	if (!Save || !Skills || !GetCharacter(Save->Selected, R)) return {};
	TArray<FDFGachaResult> Res = UDFGacha::Pull(Skills, Save->Gacha, Count, R.WeaponClass, R.Element, FMath::Rand());
	if (Res.Num()) Changed();
	return Res;
}

// ---------- fight ----------
void UDFProfileSubsystem::ApplySelectedTo(UDFCombatComponent* Combat, ACharacter* Body)
{
	FDFCharacterRow R;
	if (!GetCharacter(GetSelected(), R)) return;
	float HpM = R.HpMult, DmgM = R.DamageMult, SpdM = R.SpeedMult;
	FDFAuraRow A;
	if (GetAura(GetEquippedAura(), A)) { HpM *= 1.f + A.HpBonus; DmgM *= 1.f + A.DamageBonus; SpdM *= 1.f + A.SpeedBonus; }
	FDFSecretClassRow K;
	if (GetSecretClass(GetEquippedClass(), K)) { HpM *= K.HpMult; DmgM *= K.DamageMult; SpdM *= K.SpeedMult; }
	float WDmg = 1.f, WReach = 1.f;
	if (const FDFWeaponClassRow* W = Weapons ? Weapons->FindRow<FDFWeaponClassRow>(R.WeaponClass, TEXT("DF"), false) : nullptr)
	{
		WDmg = W->DamageMult;
		WReach = W->ReachMult;
	}
	if (Combat)
	{
		Combat->MaxHP = BaseHP * HpM;
		Combat->HP = Combat->MaxHP; // start full at the new max
		Combat->Damage = BaseDamage * DmgM * WDmg;
		Combat->Reach = BaseReach * WReach * R.SizeMult;
	}
	if (Body)
	{
		Body->SetActorScale3D(FVector(R.SizeMult));
		if (UCharacterMovementComponent* M = Body->GetCharacterMovement())
			M->MaxWalkSpeed *= SpdM;
	}
}

int32 UDFProfileSubsystem::RecordFight(bool bVictory, float PlayerHPFraction, bool bNoHit)
{
	if (!Save) return 0;
	const int32 G = bVictory ? WinGold : LossGold;
	(bVictory ? Save->Wins : Save->Losses)++;
	Save->Gacha.Gold += G;
	bHasLastCard = false;
	if (bVictory)
	{
		if (bNoHit) Save->NoHitWins++;
		if (PlayerHPFraction > 0.f && PlayerHPFraction < LowHpLine) Save->LowHpWins++;
		// one free skill card: reuse the card odds by paying the single price for the player
		FDFCharacterRow R;
		if (Skills && GetCharacter(Save->Selected, R))
		{
			Save->Gacha.Gold += UDFGacha::CostSingle;
			const TArray<FDFGachaResult> Res = UDFGacha::Pull(Skills, Save->Gacha, 1, R.WeaponClass, R.Element, FMath::Rand());
			if (Res.Num()) { LastCard = Res[0]; bHasLastCard = true; }
			else Save->Gacha.Gold -= UDFGacha::CostSingle;
		}
	}
	CheckClassUnlocks();
	Changed();
	return G;
}

void UDFProfileSubsystem::AddKill(bool bBoss)
{
	if (!Save) return;
	Save->Kills++;
	if (bBoss) Save->BossKills++;
	FDFCharacterRow R;
	if (GetCharacter(Save->Selected, R)) Save->WeaponKills.FindOrAdd(R.WeaponClass)++;
	// no save here (called often); RecordFight saves at the end of the fight
}

void UDFProfileSubsystem::RecordDraw()
{
	if (!Save) return;
	Save->Draws++;
	CheckClassUnlocks();
	Changed();
}

// ---------- auras ----------
TArray<FName> UDFProfileSubsystem::GetAuraIds() const
{
	TArray<FName> Ids = AuraTable ? AuraTable->GetRowNames() : TArray<FName>();
	Ids.Sort([this](const FName& A, const FName& B)
	{
		const FDFAuraRow* RA = AuraTable->FindRow<FDFAuraRow>(A, TEXT("DF"), false);
		const FDFAuraRow* RB = AuraTable->FindRow<FDFAuraRow>(B, TEXT("DF"), false);
		return (RA ? RA->Chance : 0) < (RB ? RB->Chance : 0);
	});
	return Ids;
}

bool UDFProfileSubsystem::GetAura(FName Id, FDFAuraRow& Out) const
{
	const FDFAuraRow* R = AuraTable ? AuraTable->FindRow<FDFAuraRow>(Id, TEXT("DF"), false) : nullptr;
	if (R) Out = *R;
	return R != nullptr;
}

int32 UDFProfileSubsystem::AuraCount(FName Id) const
{
	const int32* N = Save ? Save->Auras.Find(Id) : nullptr;
	return N ? *N : 0;
}

int32 UDFProfileSubsystem::GetTotalRolls() const { return Save ? Save->TotalRolls : 0; }
FName UDFProfileSubsystem::GetEquippedAura() const { return Save ? Save->EquippedAura : NAME_None; }

float UDFProfileSubsystem::LuckForNextRoll() const
{
	float Luck = 1.f;
	if (Save && (Save->TotalRolls + 1) % LuckEvery == 0) Luck *= 2.f;
	if (GetEquippedClass() == FName(TEXT("riftwalker"))) Luck *= 1.5f; // Rift Walker bends fate
	return Luck;
}

TArray<FName> UDFProfileSubsystem::RollAuras(int32 Count)
{
	TArray<FName> Out;
	Count = FMath::Clamp(Count, 1, 100);
	if (!Save || !AuraTable || Save->Gacha.Gold < RollCost * Count) return Out;
	const TArray<FName> Ids = GetAuraIds();
	if (!Ids.Num()) return Out;
	Save->Gacha.Gold -= RollCost * Count;
	for (int32 I = 0; I < Count; ++I)
	{
		const float Luck = LuckForNextRoll();
		FName Got = Ids[0];
		// rarest first: each aura gets its own "Luck in Chance" check (like Sol's RNG)
		for (int32 J = Ids.Num() - 1; J >= 0; --J)
		{
			FDFAuraRow A;
			if (!GetAura(Ids[J], A) || A.Chance <= 1) continue;
			if (FMath::FRand() * A.Chance < Luck) { Got = Ids[J]; break; }
		}
		Save->TotalRolls++;
		Save->Auras.FindOrAdd(Got)++;
		Out.Add(Got);
		// auto-equip if the new one is rarer than what is worn
		FDFAuraRow New, Cur;
		if (GetAura(Got, New) && (!GetAura(Save->EquippedAura, Cur) || New.Chance > Cur.Chance)) Save->EquippedAura = Got;
	}
	CheckClassUnlocks();
	Changed();
	return Out;
}

bool UDFProfileSubsystem::EquipAura(FName Id)
{
	if (!Save || (!Id.IsNone() && AuraCount(Id) <= 0)) return false;
	Save->EquippedAura = Id;
	Changed();
	return true;
}

// ---------- secret classes ----------
TArray<FName> UDFProfileSubsystem::GetClassIds() const { return ClassTable ? ClassTable->GetRowNames() : TArray<FName>(); }

bool UDFProfileSubsystem::GetSecretClass(FName Id, FDFSecretClassRow& Out) const
{
	const FDFSecretClassRow* R = ClassTable ? ClassTable->FindRow<FDFSecretClassRow>(Id, TEXT("DF"), false) : nullptr;
	if (R) Out = *R;
	return R != nullptr;
}

bool UDFProfileSubsystem::IsClassUnlocked(FName Id) const { return Save && Save->Classes.Contains(Id); }
FName UDFProfileSubsystem::GetEquippedClass() const { return Save ? Save->EquippedClass : NAME_None; }

FName UDFProfileSubsystem::GetUltimate() const
{
	FDFSecretClassRow K;
	return GetSecretClass(GetEquippedClass(), K) ? K.Ultimate : NAME_None;
}

bool UDFProfileSubsystem::EquipClass(FName Id)
{
	if (!Save || (!Id.IsNone() && !IsClassUnlocked(Id))) return false;
	Save->EquippedClass = Id;
	Changed();
	return true;
}

int32 UDFProfileSubsystem::ConditionValue(const FDFSecretClassRow& R) const
{
	const FName C = R.Condition;
	if (C == TEXT("Kills")) return Save->Kills;
	if (C == TEXT("Rolls")) return Save->TotalRolls;
	if (C == TEXT("Wins")) return Save->Wins;
	if (C == TEXT("NoHitWins")) return Save->NoHitWins;
	if (C == TEXT("LowHpWins")) return Save->LowHpWins;
	if (C == TEXT("BossKills")) return Save->BossKills;
	if (C == TEXT("Draws")) return Save->Draws;
	if (C == TEXT("AuraOwned")) return AuraCount(R.ConditionParam) > 0 ? 1 : 0;
	if (C == TEXT("WeaponKills")) { const int32* N = Save->WeaponKills.Find(R.ConditionParam); return N ? *N : 0; }
	return 0;
}

void UDFProfileSubsystem::ClassProgress(FName Id, int32& Current, int32& Needed) const
{
	Current = 0; Needed = 1;
	FDFSecretClassRow R;
	if (!Save || !GetSecretClass(Id, R)) return;
	Needed = FMath::Max(1, R.ConditionValue);
	Current = FMath::Min(ConditionValue(R), Needed);
}

void UDFProfileSubsystem::CheckClassUnlocks()
{
	if (!Save || !ClassTable) return;
	for (const FName& Id : ClassTable->GetRowNames())
	{
		if (Save->Classes.Contains(Id)) continue;
		int32 Cur, Need;
		ClassProgress(Id, Cur, Need);
		if (Cur >= Need) { Save->Classes.Add(Id); NewClasses.AddUnique(Id); }
	}
}

TArray<FName> UDFProfileSubsystem::PopNewClasses()
{
	TArray<FName> Out = MoveTemp(NewClasses);
	NewClasses.Reset();
	return Out;
}
