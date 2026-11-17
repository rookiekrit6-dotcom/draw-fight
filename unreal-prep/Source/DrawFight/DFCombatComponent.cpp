#include "DFCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

TArray<UDFCombatComponent*>& UDFCombatComponent::Registry()
{
	static TArray<UDFCombatComponent*> R;
	return R;
}

UDFCombatComponent::UDFCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UDFCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	HP = MaxHP;
	Registry().AddUnique(this);
	if (ACharacter* C = OwnerCharacter()) BaseWalkSpeed = C->GetCharacterMovement()->MaxWalkSpeed;
}

void UDFCombatComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Registry().Remove(this);
	Super::EndPlay(Reason);
}

ACharacter* UDFCombatComponent::OwnerCharacter() const { return Cast<ACharacter>(GetOwner()); }

void UDFCombatComponent::ApplySizeStats(float VolumeM3, float MoveTypeSpeedMul)
{
	// Same formulas as statsOf() in the web prototype
	const float V = FMath::Clamp(VolumeM3, .15f, 6.f);
	MaxHP = FMath::RoundToFloat(70.f + V * 42.f);
	HP = MaxHP;
	Radius = FMath::Clamp(FMath::Pow(V, 1.f / 3.f) * 55.f, 40.f, 110.f);
	if (ACharacter* C = OwnerCharacter())
	{
		BaseWalkSpeed = FMath::Clamp(760.f - V * 75.f, 360.f, 740.f) * MoveTypeSpeedMul;
		C->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}
}

bool UDFCombatComponent::TryAttack()
{
	if (bDead || StunLeft > 0.f || CooldownLeft > 0.f || AttackLeft > 0.f || WindUpLeft > 0.f) return false;
	// face the nearest opponent in range, like the web version
	if (UDFCombatComponent* T = FindNearestEnemy(Reach * 1.8f + 200.f))
	{
		FVector D = T->GetOwner()->GetActorLocation() - GetOwner()->GetActorLocation();
		D.Z = 0.f;
		if (!D.IsNearlyZero()) GetOwner()->SetActorRotation(D.Rotation());
	}
	if (WindUpTime > 0.f)
	{
		WindUpLeft = bIsBoss ? WindUpTime * 1.25f : WindUpTime;
		OnWindUp.Broadcast(this);
		return true;
	}
	StartSwing();
	return true;
}

void UDFCombatComponent::StartSwing()
{
	AttackLeft = AttackDuration;
	CooldownLeft = WindUpTime > 0.f ? 1.3f : AttackCooldown;
	bHitDone = false;
	OnAttackStarted.Broadcast(this);
}

void UDFCombatComponent::ResolveHit()
{
	bHitDone = true;
	const FVector Me = GetOwner()->GetActorLocation();
	const FVector Fwd = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
	for (UDFCombatComponent* O : Registry())
	{
		if (!O || O == this || O->Team == Team || O->bDead) continue;
		FVector D = O->GetOwner()->GetActorLocation() - Me;
		D.Z = 0.f;
		const float Dist = D.Size();
		const bool bInFront = Dist < 100.f || FVector::DotProduct(D / FMath::Max(Dist, 1.f), Fwd) > .2f;
		if (Dist - O->Radius < Reach + Radius * .4f && bInFront)
		{
			const float Dmg = Damage * FMath::FRandRange(.9f, 1.15f) * GetBuff(EDFBuff::Damage);
			if (O->ApplyDamage(Dmg, GetOwner()) > 0.f) OnAttackHit.Broadcast(this, O);
		}
	}
}

bool UDFCombatComponent::TryDash(FVector Direction)
{
	if (!CanDash() || StunLeft > 0.f) return false;
	Direction.Z = 0.f;
	DashDir = Direction.IsNearlyZero() ? GetOwner()->GetActorForwardVector() : Direction.GetSafeNormal();
	DashLeft = DashTime;
	DashCdLeft = DashCooldown;
	Invulnerable = FMath::Max(Invulnerable, DashInvulnerable);
	OnDashed.Broadcast(this);
	return true;
}

float UDFCombatComponent::ApplyDamage(float Amount, AActor* Source)
{
	if (bDead || Invulnerable > 0.f || Amount <= 0.f) return 0.f;
	Amount *= GetBuff(EDFBuff::Guard);
	if (Amount < .5f) { OnBlocked.Broadcast(this); return 0.f; }
	HP -= Amount;
	OnDamaged.Broadcast(this, Amount, Source);

	if (Source)
	{
		FVector K = GetOwner()->GetActorLocation() - Source->GetActorLocation();
		K.Z = 0.f;
		const float Kb = KnockbackCm * (bIsBoss ? .25f : 1.f);
		if (ACharacter* C = OwnerCharacter()) C->LaunchCharacter(K.GetSafeNormal() * Kb * 6.f, false, false);
	}
	if (HP <= 0.f)
	{
		HP = 0.f;
		bDead = true;
		WindUpLeft = AttackLeft = 0.f;
		OnDied.Broadcast(this);
	}
	return Amount;
}

void UDFCombatComponent::Heal(float Amount)
{
	if (bDead || Amount <= 0.f) return;
	const float Before = HP;
	HP = FMath::Min(MaxHP, HP + Amount);
	if (HP > Before) OnHealed.Broadcast(this, HP - Before);
}

void UDFCombatComponent::AddBuff(EDFBuff Kind, float Multiplier, float Duration)
{
	const int32 I = (int32)Kind;
	BuffMul[I] = FMath::Clamp(Multiplier, 0.f, 3.f);
	BuffLeft[I] = FMath::Clamp(Duration, 0.f, 12.f);
}

float UDFCombatComponent::GetBuff(EDFBuff Kind) const
{
	const int32 I = (int32)Kind;
	return BuffLeft[I] > 0.f ? BuffMul[I] : 1.f;
}

void UDFCombatComponent::Stun(float Duration)
{
	if (bDead) return;
	StunLeft = FMath::Max(StunLeft, bIsBoss ? Duration * .5f : Duration);
	WindUpLeft = 0.f; // a stun cancels the telegraphed attack
}

UDFCombatComponent* UDFCombatComponent::FindNearestEnemy(float MaxDist) const
{
	UDFCombatComponent* Best = nullptr;
	float BestD = MaxDist;
	const FVector Me = GetOwner()->GetActorLocation();
	for (UDFCombatComponent* O : Registry())
	{
		if (!O || O == this || O->Team == Team || O->bDead) continue;
		const float D = FVector::Dist2D(O->GetOwner()->GetActorLocation(), Me) - O->Radius;
		if (D < BestD) { BestD = D; Best = O; }
	}
	return Best;
}

void UDFCombatComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(Dt, TickType, ThisTickFunction);
	if (bDead) return;

	CooldownLeft -= Dt; DashCdLeft -= Dt; Invulnerable -= Dt;
	for (int32 I = 0; I < 3; ++I) if (BuffLeft[I] > 0.f) BuffLeft[I] -= Dt;

	ACharacter* C = OwnerCharacter();
	if (C && BaseWalkSpeed > 0.f)
		C->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * GetBuff(EDFBuff::Speed) * (StunLeft > 0.f ? 0.f : 1.f);

	if (StunLeft > 0.f) { StunLeft -= Dt; return; }

	if (WindUpLeft > 0.f)
	{
		WindUpLeft -= Dt;
		if (WindUpLeft <= 0.f) StartSwing();
	}
	if (AttackLeft > 0.f)
	{
		AttackLeft -= Dt;
		if (!bHitDone && AttackLeft < AttackDuration * .55f) ResolveHit();
	}
	if (DashLeft > 0.f && C)
	{
		DashLeft -= Dt;
		C->AddActorWorldOffset(DashDir * BaseWalkSpeed * DashSpeedMul * Dt, true);
	}
}
