// DFDataTypes.h
// Row structs for the data tables in /data. Each struct matches one CSV
// (column names == property names) so the CSVs import directly as DataTables:
// Content Browser > Import > pick CSV > choose the matching row struct.
// Ported from the 2D game "วาดแล้วสู้" (draw-fight/index.html).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DFDataTypes.generated.h"

UENUM(BlueprintType)
enum class EDFRarity : uint8 { Signature, Common, Rare, Legendary };

UENUM(BlueprintType)
enum class EDFGachaGroup : uint8 { Any, Melee, Ranged };

// DT_Skills.csv
USTRUCT(BlueprintType)
struct FDFSkillRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Cooldown = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDFRarity Rarity = EDFRarity::Common;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString WeaponHintTH;
	// ';'-separated weapon class keys whose signature skill this is
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SignatureOfClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EDFGachaGroup GachaGroup = EDFGachaGroup::Any;
	// ';'-separated element keys that favor this skill in gacha
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString FavoredElements;
	// Index of the effect shape used for the 2D card icon (reference only)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 IconShape = -1;
	// Filled in by hand in the editor once the 3D effect exists
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<class UNiagaraSystem> Effect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<class UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<class UTexture2D> CardArt;
};

// DT_Elements.csv
USTRUCT(BlueprintType)
struct FDFElementRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SuffixTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Color;      // "#rrggbb"
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TextColor;  // "#rrggbb" or empty
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString EffectTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString EffectEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SoundFreq = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Beats;      // ';'-separated element keys
};

// DT_WeaponClasses.csv
USTRUCT(BlueprintType)
struct FDFWeaponClassRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SignatureSkill;
	// 0 = melee. Value is in 2D pixels; rescale for 3D (suggest x0.04 = metres)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RangedDistance2D = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DamageMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReachMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float CooldownMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SigMoves;        // move;attack;effect defaults
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DrawnShapeTypes; // ';'-separated
};

// DT_Perks.csv
USTRUCT(BlueprintType)
struct FDFPerkRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString CostTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString CostEN;
};

// DT_Passives.csv
USTRUCT(BlueprintType)
struct FDFPassiveRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescEN;
};

// DT_EnemyKinds.csv
USTRUCT(BlueprintType)
struct FDFEnemyKindRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HpMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DamageMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SizeMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpeedMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool Heavy = false;
};

// DT_Bosses.csv
USTRUCT(BlueprintType)
struct FDFBossRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Kind;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName WeaponShape;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SizeMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Pattern;
};

// DT_SignatureParts.csv  (building blocks of a weapon's own skill)
USTRUCT(BlueprintType)
struct FDFSignaturePartRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Part;   // Move / Attack / Effect
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Key;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Weight = 0;
};

// DT_SignatureKeywords.csv  (weapon name keyword -> signature skill recipe)
USTRUCT(BlueprintType)
struct FDFSignatureKeywordRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Keywords; // ';'-separated, Thai + English
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Move;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Attack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Effect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Anim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SkillNameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SkillNameEN;
};

// DT_Characters.csv  (ready-made heroes the player can pick)
USTRUCT(BlueprintType)
struct FDFCharacterRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TitleTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TitleEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Element;      // DT_Elements row
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName WeaponClass;  // DT_WeaponClasses row
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Skill1;       // DT_Skills row
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Skill2;       // DT_Skills row
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Passive;      // DT_Passives row
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HpMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DamageMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpeedMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SizeMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 UnlockCost = 0; // gold, 0 = free from the start
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Color;        // "#rrggbb"
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString LookTH;       // brief for the 3D artist
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString LookEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString QuoteTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString QuoteEN;
	// Filled in by hand once the model exists (empty = use the template mannequin)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<class USkeletalMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftClassPtr<class UAnimInstance> AnimClass;
};

// DT_Auras.csv  (rolled with luck; Chance = "1 in N")
USTRUCT(BlueprintType)
struct FDFAuraRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Chance = 2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HpBonus = 0.f;      // +0.10 = +10%
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DamageBonus = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpeedBonus = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Color;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Color2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString EffectTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString EffectEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bSecret = false;     // hidden in the list until owned
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<class UNiagaraSystem> Effect;
};

// DT_SecretClasses.csv  (hidden jobs unlocked by doing something special)
USTRUCT(BlueprintType)
struct FDFSecretClassRow : public FTableRowBase
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString NameEN;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DescTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Ultimate;            // DT_Skills row (key R)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString PassiveTH;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HpMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float DamageMult = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpeedMult = 1.f;
	// Kills, Rolls, Wins, NoHitWins, LowHpWins, AuraOwned, BossKills, WeaponKills, Draws
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Condition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ConditionValue = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ConditionParam;      // aura / weapon class key
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString HintTH;            // shown while still locked
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Color;
};
