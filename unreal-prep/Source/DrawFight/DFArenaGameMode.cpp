#include "DFArenaGameMode.h"
#include "DFCombatComponent.h"
#include "DFAutoPilotComponent.h"
#include "DFProfileSubsystem.h"
#include "DFFighterCharacter.h"
#include "DFHudWidget.h"
#include "DFVoiceSubsystem.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ADFArenaGameMode::ADFArenaGameMode()
{
	DefaultPawnClass = ADFPlayerCharacter::StaticClass();
	EnemyClass = ADFEnemyCharacter::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ADFArenaGameMode::StartPlay()
{
	Super::StartPlay();
	if (!bAutoStart) return;
	FTimerHandle T;
	GetWorldTimerManager().SetTimer(T, [this]()
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			UClass* C = DFHudClass ? DFHudClass.Get() : UDFHudWidget::StaticClass();
			Hud = CreateWidget<UDFHudWidget>(PC, C);
			if (Hud) Hud->AddToViewport();
		}
		StartFight();
	}, 1.f, false);
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
			if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this)) Prof->ApplySelectedTo(C, P);
			C->OnDied.AddUniqueDynamic(this, &ADFArenaGameMode::HandlePlayerDied);
			C->OnDamaged.AddUniqueDynamic(this, &ADFArenaGameMode::HandlePlayerDamaged);
			C->OnDashed.AddUniqueDynamic(this, &ADFArenaGameMode::HandlePlayerDashed);
			C->OnAttackStarted.AddUniqueDynamic(this, &ADFArenaGameMode::HandlePlayerAttack);
		}
	bPlayerHit = false;
	bLowHpSaid = false;
	Say(EDFVoiceEvent::FightStart);
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
	if (Wave == 3) Say(EDFVoiceEvent::BossAppear);
}

void ADFArenaGameMode::HandleEnemyDied(UDFCombatComponent* Who)
{
	if (Who && Who->GetOwner()) Who->GetOwner()->SetLifeSpan(.6f); // leave time for a death effect
	Alive = FMath::Max(0, Alive - 1);
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this)) Prof->AddKill(Who && Who->bIsBoss);
	if (bRunning && Alive > 0) Say(EDFVoiceEvent::Kill);
	if (!bRunning || Alive > 0) return;
	if (Wave >= 3)
	{
		bRunning = false;
		float Frac = 1.f;
		if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
			if (UDFCombatComponent* C = P->FindComponentByClass<UDFCombatComponent>()) Frac = C->HPFraction();
		if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this)) LastReward = Prof->RecordFight(true, Frac, !bPlayerHit);
		Say(EDFVoiceEvent::Win);
		OnFightEnded.Broadcast(true);
		return;
	}
	if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
		if (UDFCombatComponent* C = P->FindComponentByClass<UDFCombatComponent>()) C->Heal(C->MaxHP * HealBetweenWaves);
	NextWaveDelay = 1.f; // short breather before the next wave
}

void ADFArenaGameMode::HandlePlayerDamaged(UDFCombatComponent* Victim, float Amount, AActor* Source)
{
	if (Amount <= 0.f || !bRunning) return;
	bPlayerHit = true;
	if (!bLowHpSaid && Victim && Victim->HPFraction() < .25f && !Victim->IsDead()) { bLowHpSaid = true; Say(EDFVoiceEvent::LowHP); }
	else Say(EDFVoiceEvent::Hurt);
}

void ADFArenaGameMode::HandlePlayerDashed(UDFCombatComponent* Self) { if (bRunning) Say(EDFVoiceEvent::Dash); }
void ADFArenaGameMode::HandlePlayerAttack(UDFCombatComponent* Self) { if (bRunning) Say(EDFVoiceEvent::Attack); }

void ADFArenaGameMode::Say(EDFVoiceEvent Event)
{
	UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this);
	UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this);
	if (V && P) V->Play(this, P->GetSelected(), Event);
}

void ADFArenaGameMode::HandlePlayerDied(UDFCombatComponent* Who)
{
	if (!bRunning) return;
	bRunning = false;
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this)) LastReward = Prof->RecordFight(false, 0.f, false);
	Say(EDFVoiceEvent::Lose);
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
