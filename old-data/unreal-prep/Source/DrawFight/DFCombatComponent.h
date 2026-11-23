// DFCombatComponent.h
// HP, basic attack, dash, buffs, stun and wind-up telegraphs for ANY character
// (player or enemy). Ported from the 3D web prototype (df3d/index.html: mkEnt,
// hurt, step). Units: Unreal centimetres (web metres x 100).
//
// Visuals/sound are NOT done here: bind the Blueprint events (OnAttackStarted,
// OnAttackHit, OnDamaged, OnWindUp, ...) to play montages, Niagara and sounds.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DFCombatComponent.generated.h"

class UDFCombatComponent;

UENUM(BlueprintType)
enum class EDFBuff : uint8 { Damage, Speed, Guard };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDFOnDamaged, UDFCombatComponent*, Victim, float, Amount, AActor*, Source);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDFOnHealed, UDFCombatComponent*, Target, float, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDFOnSimple, UDFCombatComponent*, Self);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDFOnHit, UDFCombatComponent*, Attacker, UDFCombatComponent*, Victim);

UCLASS(ClassGroup = (DrawFight), meta = (BlueprintSpawnableComponent))
class UDFCombatComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDFCombatComponent();

	// ---- stats (set from character size / weapon class, see ApplySizeStats) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") int32 Team = 0;          // 0 = player side, 1 = enemies
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float MaxHP = 100.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats") float HP = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float Damage = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float Reach = 170.f;      // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float Radius = 60.f;      // body radius cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float AttackDuration = .28f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float AttackCooldown = .42f;
	// > 0 means "telegraph first" (enemies). Player uses 0.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float WindUpTime = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") bool bIsBoss = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats") float KnockbackCm = 60.f;

	// ---- dash ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash") float DashTime = .2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash") float DashSpeedMul = 3.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash") float DashCooldown = .9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash") float DashInvulnerable = .3f;

	// ---- actions ----
	UFUNCTION(BlueprintCallable, Category = "DrawFight") bool TryAttack();
	UFUNCTION(BlueprintCallable, Category = "DrawFight") bool TryDash(FVector Direction);
	UFUNCTION(BlueprintCallable, Category = "DrawFight") float ApplyDamage(float Amount, AActor* Source);
	UFUNCTION(BlueprintCallable, Category = "DrawFight") void Heal(float Amount);
	UFUNCTION(BlueprintCallable, Category = "DrawFight") void AddBuff(EDFBuff Kind, float Multiplier, float Duration);
	UFUNCTION(BlueprintCallable, Category = "DrawFight") void Stun(float Duration);
	UFUNCTION(BlueprintCallable, Category = "DrawFight") void SetInvulnerable(float Duration) { Invulnerable = FMath::Max(Invulnerable, Duration); }

	// Size-based stats like the web prototype: volume (m^3) -> hp, speed, radius.
	UFUNCTION(BlueprintCallable, Category = "DrawFight") void ApplySizeStats(float VolumeM3, float MoveTypeSpeedMul = 1.f);

	// ---- queries ----
	UFUNCTION(BlueprintPure, Category = "DrawFight") bool IsDead() const { return bDead; }
	UFUNCTION(BlueprintPure, Category = "DrawFight") bool IsStunned() const { return StunLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "DrawFight") bool IsBusy() const { return AttackLeft > 0.f || WindUpLeft > 0.f || StunLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "DrawFight") bool IsWindingUp() const { return WindUpLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "DrawFight") float WindUpRemaining() const { return WindUpLeft; }
	UFUNCTION(BlueprintPure, Category = "DrawFight") bool CanDash() const { return DashCdLeft <= 0.f && !bDead; }
	UFUNCTION(BlueprintPure, Category = "DrawFight") float GetBuff(EDFBuff Kind) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight") float HPFraction() const { return MaxHP > 0.f ? HP / MaxHP : 0.f; }
	// Nearest living opponent within MaxDist (cm), or nullptr.
	UFUNCTION(BlueprintPure, Category = "DrawFight") UDFCombatComponent* FindNearestEnemy(float MaxDist = 5000.f) const;

	// ---- events for Blueprint (montages, Niagara, sounds, UI numbers) ----
	UPROPERTY(BlueprintAssignable) FDFOnSimple OnWindUp;        // show the red ground warning
	UPROPERTY(BlueprintAssignable) FDFOnSimple OnAttackStarted; // play attack montage
	UPROPERTY(BlueprintAssignable) FDFOnHit OnAttackHit;        // hit spark at victim
	UPROPERTY(BlueprintAssignable) FDFOnDamaged OnDamaged;      // floating damage number
	UPROPERTY(BlueprintAssignable) FDFOnHealed OnHealed;        // floating heal number
	UPROPERTY(BlueprintAssignable) FDFOnSimple OnDashed;
	UPROPERTY(BlueprintAssignable) FDFOnSimple OnBlocked;       // guard buff ate the hit
	UPROPERTY(BlueprintAssignable) FDFOnSimple OnDied;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	static TArray<UDFCombatComponent*>& Registry(); // all live components (cheap target search)

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void StartSwing();
	void ResolveHit();
	class ACharacter* OwnerCharacter() const;

	bool bDead = false;
	bool bHitDone = true;
	float AttackLeft = 0.f, CooldownLeft = 0.f, WindUpLeft = 0.f;
	float DashLeft = 0.f, DashCdLeft = 0.f, Invulnerable = 0.f, StunLeft = 0.f;
	float BaseWalkSpeed = 0.f;
	FVector DashDir = FVector::ZeroVector;
	float BuffMul[3] = { 1.f, 1.f, 1.f };
	float BuffLeft[3] = { 0.f, 0.f, 0.f };
};
