// DFSkillPack.h
// Loads and validates player-made skill files (.dfskill, JSON) exactly like the
// web prototype (skCheck / skPower / skCd in df3d/index.html), so a file made
// on the web works in the Unreal game and vice versa.
//
// This is data only. Playing a step (spawning Niagara, dealing damage) is the
// job of the skill runner (reserved for Fable, see PLAN.md).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DFSkillPack.generated.h"

USTRUCT(BlueprintType)
struct FDFSkillStep
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) float At = 0.f;   // seconds after cast, 0..6
	UPROPERTY(BlueprintReadOnly) FName Do;         // shot, area, fall, beam, ...
	// Every other field as text; read with UDFSkillPack::Num / Str / Bool
	UPROPERTY(BlueprintReadOnly) TMap<FName, FString> Params;
};

USTRUCT(BlueprintType)
struct FDFSkillAnimKey
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) float T = 0.f;                 // 0..1 of Duration
	UPROPERTY(BlueprintReadOnly) TMap<FName, FRotator> Bones;   // degrees, bone ids: hips spine chest neck head armL foreL handL armR foreR handR thighL shinL footL thighR shinR footR
};

USTRUCT(BlueprintType)
struct FDFSkillPack
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) FString Author;
	UPROPERTY(BlueprintReadOnly) FLinearColor Color = FLinearColor(1.f, .69f, .13f);
	UPROPERTY(BlueprintReadOnly) float DeclaredCooldown = 4.f;
	UPROPERTY(BlueprintReadOnly) TArray<FDFSkillStep> Steps;          // sorted by At
	UPROPERTY(BlueprintReadOnly) TMap<FString, FString> ImagesBase64; // name -> data URL (png/jpeg/webp)
	UPROPERTY(BlueprintReadOnly) bool bHasAnim = false;
	UPROPERTY(BlueprintReadOnly) float AnimDuration = .6f;
	UPROPERTY(BlueprintReadOnly) TArray<FDFSkillAnimKey> AnimKeys;
};

UCLASS()
class UDFSkillPack : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Parses + validates. On failure returns false and a Thai message in Error.
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Skills")
	static bool ParseSkillPack(const FString& Json, FDFSkillPack& Out, FString& Error);

	UFUNCTION(BlueprintCallable, Category = "DrawFight|Skills")
	static bool LoadSkillPackFile(const FString& Path, FDFSkillPack& Out, FString& Error);

	// Same formula as the web: sum of damage-like values
	UFUNCTION(BlueprintPure, Category = "DrawFight|Skills")
	static int32 Power(const FDFSkillPack& Pack);

	// max(declared, 1.5, round(power/14, 1 decimal)) - stronger skills wait longer
	UFUNCTION(BlueprintPure, Category = "DrawFight|Skills")
	static float EffectiveCooldown(const FDFSkillPack& Pack);

	// Clamped number param, exactly like num(v, default, min, max) on the web
	UFUNCTION(BlueprintPure, Category = "DrawFight|Skills")
	static float Num(const FDFSkillStep& Step, FName Key, float Default, float Min, float Max);

	UFUNCTION(BlueprintPure, Category = "DrawFight|Skills")
	static FString Str(const FDFSkillStep& Step, FName Key, const FString& Default);

	UFUNCTION(BlueprintPure, Category = "DrawFight|Skills")
	static bool Bool(const FDFSkillStep& Step, FName Key);

	static const TSet<FName>& KnownSteps();
};
