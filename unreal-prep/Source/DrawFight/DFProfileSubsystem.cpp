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
	}
	if (Save)
		for (const TPair<FName, int32>& P : Save->Gacha.SkillLevels)
			if (P.Value > 0) Out.AddUnique(P.Key);
	return Out;
}

TArray<FName> UDFProfileSubsystem::GetLoadout(FName Hero) const
{
	if (!Save) return {};
	if (const FDFLoadout* L = Save->Loadouts.Find(Hero))
		if (L->Skills.Num() == SlotCount) return L->Skills;
	// Default: own 2 skills + weapon signature
	TArray<FName> Def = GetUsableSkills(Hero);
	Def.SetNum(FMath::Min(Def.Num(), SlotCount));
	while (Def.Num() < SlotCount) Def.Add(NAME_None);
	Save->Loadouts.Add(Hero).Skills = Def; // Save is a UObject pointer, so this is allowed from const
	return Def;
}

bool UDFProfileSubsystem::SetLoadoutSlot(FName Hero, int32 Slot, FName Skill)
{
	if (!Save || Slot < 0 || Slot >= SlotCount || !IsUnlocked(Hero)) return false;
	if (!Skill.IsNone() && !GetUsableSkills(Hero).Contains(Skill)) return false;
	TArray<FName> L = GetLoadout(Hero);
	const int32 Other = L.IndexOfByKey(Skill);
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
	if (!Usable.Num() || Slot < 0 || Slot >= L.Num()) return NAME_None;
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
	float WDmg = 1.f, WReach = 1.f;
	if (const FDFWeaponClassRow* W = Weapons ? Weapons->FindRow<FDFWeaponClassRow>(R.WeaponClass, TEXT("DF"), false) : nullptr)
	{
		WDmg = W->DamageMult;
		WReach = W->ReachMult;
	}
	if (Combat)
	{
		Combat->MaxHP = BaseHP * R.HpMult;
		Combat->HP = Combat->MaxHP; // start full at the new max
		Combat->Damage = BaseDamage * R.DamageMult * WDmg;
		Combat->Reach = BaseReach * WReach * R.SizeMult;
	}
	if (Body)
	{
		Body->SetActorScale3D(FVector(R.SizeMult));
		if (UCharacterMovementComponent* M = Body->GetCharacterMovement())
			M->MaxWalkSpeed *= R.SpeedMult;
	}
}

int32 UDFProfileSubsystem::RecordFight(bool bVictory)
{
	if (!Save) return 0;
	const int32 G = bVictory ? WinGold : LossGold;
	(bVictory ? Save->Wins : Save->Losses)++;
	Save->Gacha.Gold += G;
	Changed();
	return G;
}
