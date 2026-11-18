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
	static constexpr int32 RollCost = 10;
	static constexpr int32 LuckEvery = 10;      // every 10th roll has double luck
	static constexpr float LowHpLine = .1f;     // "won with HP below 10%"


	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UDFProfileSubsystem* Get(const UObject* WorldContext);

	UPROPERTY(BlueprintAssignable) FDFOnProfileChanged OnProfileChanged;

	// --- data ---
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool HasData() const { return Characters && Skills; }
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") TArray<FName> GetCharacterIds() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool GetCharacter(FName Id, FDFCharacterRow& Out) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool GetSkill(FName Id, FDFSkillRow& Out) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") TArray<FName> GetAllSkillIds() const;
	// ---- character creation ----
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool IsCreated() const;
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void CreateCharacter(FName Hero, const FString& Name, FName Element);
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FString GetPlayerName() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FName GetPlayerElement() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") TArray<FName> GetElementIds() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FLinearColor ElementColor(FName Id) const;
	// colour of the player's skills: chosen element, else the hero colour
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FLinearColor GetPowerColor() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") FName GetPowerElement() const;
	// ---- next fight + settings ----
	UDFSaveGame* Data() const { return Save; }
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void RecordEndless(int32 Kills, float Seconds);
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void ApplySettings();
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

	// --- skill cards: no shop any more, one free card drops after every win ---
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") TArray<FDFGachaResult> Pull(int32 Count);
	UFUNCTION(BlueprintPure, Category = "DrawFight|Profile") bool GetLastCardDrop(FDFGachaResult& Out) const { Out = LastCard; return bHasLastCard; }

	// --- auras (roll with luck, equip one) ---
	UFUNCTION(BlueprintPure, Category = "DrawFight|Aura") TArray<FName> GetAuraIds() const;   // common first
	UFUNCTION(BlueprintPure, Category = "DrawFight|Aura") bool GetAura(FName Id, FDFAuraRow& Out) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Aura") int32 AuraCount(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Aura") int32 GetTotalRolls() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Aura") float LuckForNextRoll() const;
	// Spends RollCost per roll; returns the aura rolled each time (empty if not enough gold)
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Aura") TArray<FName> RollAuras(int32 Count);
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Aura") bool EquipAura(FName Id);
	UFUNCTION(BlueprintPure, Category = "DrawFight|Aura") FName GetEquippedAura() const;

	// --- secret classes ---
	UFUNCTION(BlueprintPure, Category = "DrawFight|Class") TArray<FName> GetClassIds() const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Class") bool GetSecretClass(FName Id, FDFSecretClassRow& Out) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Class") bool IsClassUnlocked(FName Id) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight|Class") void ClassProgress(FName Id, int32& Current, int32& Needed) const;
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Class") bool EquipClass(FName Id); // None = no class
	UFUNCTION(BlueprintPure, Category = "DrawFight|Class") FName GetEquippedClass() const;
	// The equipped class's ultimate skill (key R), or None
	UFUNCTION(BlueprintPure, Category = "DrawFight|Class") FName GetUltimate() const;
	// Classes unlocked since the last call (for a "secret class unlocked!" popup)
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Class") TArray<FName> PopNewClasses();
	// Call when the player finishes drawing a weapon (counts toward Divine Painter)
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Class") void RecordDraw();

	// --- fight ---
	// Applies the selected hero's stats to a fighter (call when the arena starts)
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void ApplySelectedTo(UDFCombatComponent* Combat, ACharacter* Body);
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void AddKill(bool bBoss);
	// Returns the gold awarded. A win also drops one free skill card (see GetLastCardDrop)
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") int32 RecordFight(bool bVictory, float PlayerHPFraction = 1.f, bool bNoHit = false);

	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void SaveNow();
	UFUNCTION(BlueprintCallable, Category = "DrawFight|Profile") void ResetProfile();

	UPROPERTY() TObjectPtr<UDataTable> Characters;
	UPROPERTY() TObjectPtr<UDataTable> Skills;
	UPROPERTY() TObjectPtr<UDataTable> Weapons;
	UPROPERTY() TObjectPtr<UDataTable> Elements;
	UPROPERTY() TObjectPtr<UDataTable> AuraTable;
	UPROPERTY() TObjectPtr<UDataTable> ClassTable;

private:
	UPROPERTY() TObjectPtr<UDFSaveGame> Save;
	void EnsureDefaults();
	void CheckClassUnlocks();
	int32 ConditionValue(const FDFSecretClassRow& R) const;
	TArray<FName> NewClasses;
	FDFGachaResult LastCard;
	bool bHasLastCard = false;
	void Changed();
	static const TCHAR* SlotName;
};
