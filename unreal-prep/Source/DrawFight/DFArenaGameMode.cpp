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
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "DFArenaDressing.h"
#include "DFAudio.h"
#include "DFSkillFX.h"
#include "DFSaveGame.h"
#include "DFDataTypes.h"
#include "Engine/DataTable.h"

ADFArenaGameMode::ADFArenaGameMode()
{
	DefaultPawnClass = ADFPlayerCharacter::StaticClass();
	EnemyClass = ADFEnemyCharacter::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ADFArenaGameMode::StartPlay()
{
	Super::StartPlay();
	{
		FName Theme = TEXT("lab");
		if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this))
			if (Prof->Data()) Theme = Prof->Data()->Arena;
		if (!ADFArenaDressing::Build(GetWorld(), Theme, ArenaRadius)) DarkenArena();
		DFAudio::Music(this, TEXT("M_Battle"));
	}
#if !WITH_EDITOR
	// a character must be created before any fight (editor play-tests skip this)
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this))
		if (Prof->HasData() && !Prof->IsCreated()) { UGameplayStatics::OpenLevel(this, MenuLevel); return; }
#endif
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
	Kills = 0;
	PowerLevel = 0;
	Elapsed = 0.f;
	SpawnTimer = 1.f;
	NextBossAt = 60.f;
	bRunning = true;
	NextWaveDelay = -1.f;
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this))
		if (UDFSaveGame* S = Prof->Data())
		{
			bEndless = S->Mode == FName(TEXT("endless"));
			Chapter = FMath::Clamp(S->PlayChapter, 1, FMath::Max(1, S->Chapter));
		}
	TotalWaves = FMath::Min(3 + (Chapter - 1) / 3, 6);
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
	if (!bEndless) SpawnWave();
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
		C->WindUpTime = bBoss ? .9f : .75f;
		C->AttackCooldown = bBoss ? 1.4f : 1.6f;
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

// enemy of a DT_EnemyKinds type, tinted so each kind reads at a glance
ACharacter* ADFArenaGameMode::SpawnKind(FName Kind, float HPBase, float DmgBase, bool bBoss, float BossScale)
{
	static UDataTable* Kinds = nullptr;
	if (!Kinds) Kinds = LoadObject<UDataTable>(nullptr, TEXT("/Game/DrawFight/Data/DT_EnemyKinds.DT_EnemyKinds"));
	FDFEnemyKindRow K;
	if (const FDFEnemyKindRow* R = Kinds ? Kinds->FindRow<FDFEnemyKindRow>(Kind, TEXT("DF"), false) : nullptr) K = *R;
	const float Size = (bBoss ? 2.1f * BossScale : .85f) * FMath::Clamp(K.SizeMult, .5f, 1.8f);
	ACharacter* E = SpawnEnemy(Size, HPBase * K.HpMult, (bBoss ? 210.f : 270.f) * K.SpeedMult, DmgBase * K.DamageMult, bBoss);
	if (E)
		if (UMaterialInterface* G = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DrawFight/FX/M_Glow.M_Glow")))
		{
			const uint32 H = GetTypeHash(Kind);
			const FLinearColor Col = bBoss ? FLinearColor(1.f, .15f, .1f) : FLinearColor::MakeFromHSV8((uint8)(H % 255), 200, 255);
			UMaterialInstanceDynamic* MI = UMaterialInstanceDynamic::Create(G, E);
			MI->SetVectorParameterValue(TEXT("Color"), Col);
			MI->SetScalarParameterValue(TEXT("Glow"), bBoss ? 1.2f : .5f);
			MI->SetScalarParameterValue(TEXT("Opacity"), .5f);
			E->GetMesh()->SetOverlayMaterial(MI);
		}
	return E;
}

void ADFArenaGameMode::SpawnWave()
{
	Wave++;
	const float HPMul = 1.f + .22f * (Chapter - 1), DmgMul = 1.f + .15f * (Chapter - 1);
	const bool bBossWave = Wave >= TotalWaves;
	static const FName KindPool[] = { TEXT("normal"), TEXT("normal"), TEXT("small"), TEXT("small"), TEXT("giant") };
	const int32 N = bBossWave ? 2 : 2 + Wave + Chapter / 2;
	for (int32 I = 0; I < N; ++I)
		SpawnKind(KindPool[FMath::RandRange(0, Wave >= 2 ? 4 : 3)], (34.f + Wave * 8.f) * HPMul, 4.f * DmgMul);
	if (bBossWave)
	{
		static UDataTable* Bosses = nullptr;
		if (!Bosses) Bosses = LoadObject<UDataTable>(nullptr, TEXT("/Game/DrawFight/Data/DT_Bosses.DT_Bosses"));
		FDFBossRow B;
		BossName = TEXT("บอส");
		if (Bosses)
		{
			const TArray<FName> Rows = Bosses->GetRowNames();
			if (Rows.Num())
				if (const FDFBossRow* R = Bosses->FindRow<FDFBossRow>(Rows[(Chapter - 1) % Rows.Num()], TEXT("DF"), false)) { B = *R; BossName = B.NameTH; }
		}
		SpawnKind(B.Kind.IsNone() ? FName(TEXT("giant")) : B.Kind, 260.f * (1.f + .3f * (Chapter - 1)), 10.f * DmgMul, true, FMath::Clamp(B.SizeMult, .7f, 1.3f));
	}
	OnWaveStarted.Broadcast(Wave, bBossWave);
	if (bBossWave) Say(EDFVoiceEvent::BossAppear);
}

// endless: ever-faster spawns, a boss every minute, a power-up every 15 kills
void ADFArenaGameMode::TickEndless(float Dt)
{
	Elapsed += Dt;
	SpawnTimer -= Dt;
	const float Minutes = Elapsed / 60.f;
	if (SpawnTimer <= 0.f && Alive < 26)
	{
		SpawnTimer = FMath::Max(.9f, 3.2f - Minutes * .55f);
		const int32 N = 1 + FMath::Min(5, (int32)(Minutes * 1.5f));
		static const FName Pool[] = { TEXT("normal"), TEXT("small"), TEXT("small"), TEXT("giant") };
		for (int32 I = 0; I < N; ++I) SpawnKind(Pool[FMath::RandRange(0, Minutes > 1.f ? 3 : 2)], 30.f * (1.f + Minutes * .35f), 3.f * (1.f + Minutes * .2f));
	}
	if (Elapsed >= NextBossAt)
	{
		NextBossAt += 60.f;
		BossName = FString::Printf(TEXT("บอสนาทีที่ %d"), (int32)Minutes);
		SpawnKind(TEXT("giant"), 220.f * (1.f + Minutes * .5f), 8.f * (1.f + Minutes * .2f), true);
		OnWaveStarted.Broadcast(++Wave, true);
		Say(EDFVoiceEvent::BossAppear);
	}
}

void ADFArenaGameMode::EndFight(bool bVictory)
{
	bRunning = false;
	float Frac = 0.f;
	if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
		if (UDFCombatComponent* C = P->FindComponentByClass<UDFCombatComponent>()) Frac = C->HPFraction();
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this))
	{
		if (bEndless)
		{
			Prof->RecordEndless(Kills, Elapsed);
			LastReward = Prof->RecordFight(Kills >= 30, Frac, false);
			Prof->AddGold(Kills * 3);
			LastReward += Kills * 3;
		}
		else
		{
			LastReward = Prof->RecordFight(bVictory, Frac, bVictory && !bPlayerHit);
			if (bVictory && Prof->Data())
			{
				UDFSaveGame* S = Prof->Data();
				Prof->AddGold(20 * (Chapter - 1));
				LastReward += 20 * (Chapter - 1);
				if (Chapter >= S->Chapter && S->Chapter < 12) S->Chapter = Chapter + 1;
				S->PlayChapter = FMath::Min(Chapter + 1, S->Chapter);
				Prof->SaveNow();
			}
		}
	}
	Say(bVictory ? EDFVoiceEvent::Win : EDFVoiceEvent::Lose);
	DFAudio::Sfx(this, bVictory ? TEXT("S_Win") : TEXT("S_Lose"));
	OnFightEnded.Broadcast(bVictory);
}

void ADFArenaGameMode::HandleEnemyDied(UDFCombatComponent* Who)
{
	if (Who && Who->GetOwner())
	{
		Who->GetOwner()->SetLifeSpan(.6f); // leave time for a death effect
		ADFSkillFX::Puff(Who->GetOwner(), Who->GetOwner()->GetActorLocation(), Who->bIsBoss ? FLinearColor(1.f, .3f, .1f) : FLinearColor(1.f, .6f, .3f), NAME_None, Who->bIsBoss ? 2.5f : 1.f);
	}
	Alive = FMath::Max(0, Alive - 1);
	Kills++;
	if (bEndless && bRunning)
	{
		if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this)) Prof->AddKill(Who && Who->bIsBoss);
		if (Kills % 15 == 0)
			if (ACharacter* P = UGameplayStatics::GetPlayerCharacter(this, 0))
				if (UDFCombatComponent* C = P->FindComponentByClass<UDFCombatComponent>())
				{
					PowerLevel++;
					C->Damage *= 1.12f;
					C->Heal(C->MaxHP * .25f);
					if (Hud) Hud->HandleSubtitle(FString::Printf(TEXT("พลังเพิ่ม! ระดับ %d  (โจมตี +12%%  ฟื้นเลือด)"), PowerLevel + 1), 2.5f);
				}
		return;
	}
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this)) Prof->AddKill(Who && Who->bIsBoss);
	if (bRunning && Alive > 0) Say(EDFVoiceEvent::Kill);
	if (!bRunning || Alive > 0) return;
	if (Wave >= TotalWaves)
	{
		EndFight(true);
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
	EndFight(false);
}

void ADFArenaGameMode::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bEndless && bRunning) TickEndless(Dt);
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

// repaint the template's grey/neon-green blockout as a dark stone arena so glowing skills stand out
void ADFArenaGameMode::DarkenArena()
{
	UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!Basic) return;
	const FLinearColor Tones[3] = { FLinearColor(.045f, .045f, .06f), FLinearColor(.07f, .065f, .085f), FLinearColor(.03f, .05f, .07f) };
	int32 I = 0;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		UStaticMeshComponent* SM = It->GetStaticMeshComponent();
		if (!SM) continue;
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Basic, this);
		M->SetVectorParameterValue(TEXT("Color"), Tones[I++ % 3]);
		for (int32 S = 0; S < SM->GetNumMaterials(); ++S) SM->SetMaterial(S, M);
	}
}
