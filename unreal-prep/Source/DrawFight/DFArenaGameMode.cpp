#include "DFArenaGameMode.h"
#include "DFCombatComponent.h"
#include "DFAutoPilotComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ADFArenaGameMode::ADFArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADFArenaGameMode::StartFight()
{
	Wave = 0;
	Alive = 0;
	bRunning = true;
	NextWaveDelay = -1.f;
	if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
		if (UDFCombatComponent* C = P->FindComponentByClass<UDFCombatComponent>())
		{
			C->Team = 0;
			C->OnDied.AddUniqueDynamic(this, &ADFArenaGameMode::HandlePlayerDied);
		}
	SpawnWave();
}

ACharacter* ADFArenaGameMode::SpawnEnemy(float Scale, float HP, float Speed, float Damage, bool bBoss)
{
	if (!EnemyClass) return nullptr;
	const float A = FMath::FRandRange(0.f, 2.f * PI);
	const float R = bBoss ? 1400.f : FMath::FRandRange(SpawnMin, SpawnMax);
	const FVector Pos(FMath::Cos(A) * R, FMath::Sin(A) * R, 120.f * Scale);
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ACharacter* E = GetWorld()->SpawnActor<ACharacter>(EnemyClass, Pos, FRotator::ZeroRotator, SP);
	if (!E) return nullptr;
	E->SetActorScale3D(FVector(Scale));
	E->GetCharacterMovement()->MaxWalkSpeed = Speed;
	if (UDFCombatComponent* C = E->FindComponentByClass<UDFCombatComponent>())
	{
		C->Team = 1;
		C->MaxHP = HP;
		C->Damage = Damage;
		C->bIsBoss = bBoss;
		C->WindUpTime = .6f;
		C->Reach = 125.f * Scale;
		C->Radius = 50.f * Scale;
		C->OnDied.AddUniqueDynamic(this, &ADFArenaGameMode::HandleEnemyDied);
	}
	if (UDFAutoPilotComponent* AP = E->FindComponentByClass<UDFAutoPilotComponent>())
	{
		AP->bEnabled = true;
		AP->bDodge = false;
		AP->bUseSkills = false;
	}
	Alive++;
	return E;
}

void ADFArenaGameMode::SpawnWave()
{
	Wave++;
	const int32 N = Wave == 1 ? 3 : Wave == 2 ? 5 : 2;
	for (int32 I = 0; I < N; ++I)
		SpawnEnemy(.85f, 34.f + Wave * 8.f, 250.f + Wave * 25.f, 7.f, false);
	if (Wave == 3) SpawnEnemy(2.1f, 260.f, 210.f, 16.f, true);
	OnWaveStarted.Broadcast(Wave, Wave == 3);
}

void ADFArenaGameMode::HandleEnemyDied(UDFCombatComponent* Who)
{
	if (Who && Who->GetOwner()) Who->GetOwner()->SetLifeSpan(.6f); // leave time for a death effect
	Alive = FMath::Max(0, Alive - 1);
	if (!bRunning || Alive > 0) return;
	if (Wave >= 3)
	{
		bRunning = false;
		OnFightEnded.Broadcast(true);
		return;
	}
	if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
		if (UDFCombatComponent* C = P->FindComponentByClass<UDFCombatComponent>()) C->Heal(C->MaxHP * HealBetweenWaves);
	NextWaveDelay = 1.f; // short breather before the next wave
}

void ADFArenaGameMode::HandlePlayerDied(UDFCombatComponent* Who)
{
	if (!bRunning) return;
	bRunning = false;
	OnFightEnded.Broadcast(false);
}

void ADFArenaGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (NextWaveDelay > 0.f)
	{
		NextWaveDelay -= Dt;
		if (NextWaveDelay <= 0.f && bRunning) SpawnWave();
	}
	// keep everyone inside the arena ring
	for (UDFCombatComponent* C : UDFCombatComponent::Registry())
	{
		if (!C || !C->GetOwner()) continue;
		FVector L = C->GetOwner()->GetActorLocation();
		const float D = FVector(L.X, L.Y, 0.f).Size();
		const float Max = ArenaRadius - 60.f;
		if (D > Max) { L.X *= Max / D; L.Y *= Max / D; C->GetOwner()->SetActorLocation(L); }
	}
}
