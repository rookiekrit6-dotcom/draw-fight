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
