// DFProfileSubsystem.h — the player's profile: gold, unlocked heroes, chosen hero,
// skill loadouts and gacha. Lives for the whole game session (GameInstance subsystem),
// so the menu and the arena level see the same data. Saves after every change.
//
// Data tables are loaded from these asset paths (import the CSVs there):
//   /Game/DrawFight/Data/DT_Characters, DT_Skills, DT_WeaponClasses, DT_Elements
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DFDataTypes.h"
#include "DFGacha.h"
#include "DFProfileSubsystem.generated.h"

class UDFSaveGame;
class UDFCombatComponent;
class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDFOnProfileChanged);

UCLASS()
class UDFProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static constexpr int32 StartGold = 300;
	static constexpr int32 WinGold = 120;
	static constexpr int32 LossGold = 30;
	static constexpr int32 SlotCount = 3;
	static constexpr float BaseHP = 100.f;
	static constexpr float BaseDamage = 12.f;
	static constexpr float BaseReach = 170.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UDFProfileSubsystem* Get(const UObject* WorldContext);

	UPROPERTY(BlueprintAssignable) FDFOnProfileChanged OnProfileChanged;

	// --- data ---
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool HasData() const { return Characters && Skills; }
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") TArray<FName> GetCharacterIds() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool GetCharacter(FName Id, FDFCharacterRow& Out) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool GetSkill(FName Id, FDFSkillRow& Out) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FString SkillNameTH(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FString ElementNameTH(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FString WeaponNameTH(FName Id) const;
	// Weapon's own signature skill (DT_WeaponClasses.SignatureSkill)
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FName WeaponSignature(FName WeaponClass) const;

	// --- profile ---
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") int32 GetGold() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") int32 GetPity() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") int32 GetWins() const;
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void AddGold(int32 Amount);
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool IsUnlocked(FName Id) const;
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") bool Unlock(FName Id);
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") bool Select(FName Id);
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FName GetSelected() const;

	// --- skills ---
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") int32 GetSkillLevel(FName Skill) const;
	// Skills this hero may put in a slot: its 2 own skills, its weapon signature, and every gacha card owned
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") TArray<FName> GetUsableSkills(FName Hero) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") TArray<FName> GetLoadout(FName Hero) const;
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") bool SetLoadoutSlot(FName Hero, int32 Slot, FName Skill);
	// Moves the slot to the next usable skill not already in another slot
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") FName CycleLoadoutSlot(FName Hero, int32 Slot);

	// --- gacha (uses the selected hero's weapon + element for the odds) ---
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") TArray<FDFGachaResult> Pull(int32 Count);

	// --- fight ---
	// Applies the selected hero's stats to a fighter (call when the arena starts)
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void ApplySelectedTo(UDFCombatComponent* Combat, ACharacter* Body);
	// Returns the gold awarded
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") int32 RecordFight(bool bVictory);

	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void SaveNow();
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void ResetProfile();

	UPROPERTY() TObjectPtr<UDataTable> Characters;
	UPROPERTY() TObjectPtr<UDataTable> Skills;
	UPROPERTY() TObjectPtr<UDataTable> Weapons;
	UPROPERTY() TObjectPtr<UDataTable> Elements;

private:
	UPROPERTY() TObjectPtr<UDFSaveGame> Save;
	void EnsureDefaults();
	void Changed();
	static const TCHAR* SlotName;
};
