// DFArenaGameMode.h
// 3 waves in a round arena: 3 small, 5 small, then a boss with 2 guards.
// 30% heal between waves. Victory / defeat events for the UI.
// Same rules as spawnWave() / end() in the web prototype.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Character.h"
#include "DFVoiceSubsystem.h"
#include "DFArenaGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDFOnWave, int32, Wave, bool, bBossWave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDFOnFightEnd, bool, bVictory);

UCLASS()
class ADFArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	void DarkenArena();
	ADFArenaGameMode();

	// Enemy pawn: any Character with DFCombatComponent + DFAutoPilotComponent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") TSubclassOf<ACharacter> EnemyClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") float ArenaRadius = 2200.f;    // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") float SpawnMin = 1100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") float SpawnMax = 1600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") float HealBetweenWaves = .3f;

	// Starts the fight 1 s after the level loads and shows the HUD (DFHudWidget)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") bool bAutoStart = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") TSubclassOf<class UDFHudWidget> DFHudClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena") FName MenuLevel = TEXT("MainMenu");
	UPROPERTY(BlueprintReadOnly, Category = "Arena") int32 LastReward = 0;   // gold from the last fight
	UPROPERTY(BlueprintReadOnly, Category = "Arena") TObjectPtr<class UDFHudWidget> Hud;
	UFUNCTION(BlueprintPure, Category = "Arena") bool IsRunning() const { return bRunning; }

	UPROPERTY(BlueprintAssignable) FDFOnWave OnWaveStarted;
	UPROPERTY(BlueprintAssignable) FDFOnFightEnd OnFightEnded;

	UFUNCTION(BlueprintCallable, Category = "Arena") void StartFight();
	virtual void StartPlay() override;
	UFUNCTION(BlueprintPure, Category = "Arena") int32 GetWave() const { return Wave; }
	UFUNCTION(BlueprintPure, Category = "Arena") int32 EnemiesLeft() const { return Alive; }
	// modes: story chapters (waves + named boss) or endless survival
	bool IsEndless() const { return bEndless; }
	int32 GetChapter() const { return Chapter; }
	int32 GetTotalWaves() const { return TotalWaves; }
	int32 GetKills() const { return Kills; }
	float GetElapsed() const { return Elapsed; }
	int32 GetPowerLevel() const { return PowerLevel; }
	FString BossName;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void SpawnWave();
	ACharacter* SpawnEnemy(float Scale, float HP, float Speed, float Damage, bool bBoss);
	ACharacter* SpawnKind(FName Kind, float HPBase, float DmgBase, bool bBoss = false, float BossScale = 1.f);
	void TickEndless(float Dt);
	void EndFight(bool bVictory);
	bool bEndless = false;
	int32 Chapter = 1, TotalWaves = 3, Kills = 0, PowerLevel = 0;
	float Elapsed = 0.f, SpawnTimer = 0.f, NextBossAt = 60.f;
	UFUNCTION() void HandleEnemyDied(class UDFCombatComponent* Who);
	UFUNCTION() void HandlePlayerDied(class UDFCombatComponent* Who);
	UFUNCTION() void HandlePlayerDamaged(class UDFCombatComponent* Victim, float Amount, AActor* Source);
	UFUNCTION() void HandlePlayerDashed(class UDFCombatComponent* Self);
	UFUNCTION() void HandlePlayerAttack(class UDFCombatComponent* Self);
	bool bPlayerHit = false;
	bool bLowHpSaid = false;
	void Say(EDFVoiceEvent Event);

	int32 Wave = 0;
	int32 Alive = 0;
	bool bRunning = false;
	float NextWaveDelay = -1.f;
};
