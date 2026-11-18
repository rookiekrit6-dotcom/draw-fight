// DFFighterCharacter.h — playable fighters in code, no Blueprint needed.
//   ADFFighterCharacter : shared body (mannequin mesh + anim, combat, autopilot, hit/attack anims, ragdoll on death)
//   ADFPlayerCharacter  : camera + keyboard/mouse/gamepad controls
//   ADFEnemyCharacter   : AI-driven body (DFArenaGameMode spawns these)
// Controls: WASD move · mouse look · wheel zoom · LMB/J attack · Shift dash · Space jump
//           1/2/3 skills · R ultimate (secret class) · T auto · Esc back to menu
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DFFighterCharacter.generated.h"

class UDFCombatComponent;
class UDFAutoPilotComponent;
class USpringArmComponent;
class UCameraComponent;
class UAnimSequence;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDFOnSkillCast, int32, Slot, FName, Skill);

UCLASS()
class ADFFighterCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	ADFFighterCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DrawFight") TObjectPtr<UDFCombatComponent> Combat;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DrawFight") TObjectPtr<UDFAutoPilotComponent> AutoPilot;

	// Mannequin attack/dash clips (played in the anim BP's DefaultSlot if it has one)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawFight|Anim") TArray<TObjectPtr<UAnimSequence>> AttackAnims;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawFight|Anim") TObjectPtr<UAnimSequence> DashAnim;

protected:
	virtual void BeginPlay() override;
	UFUNCTION() void HandleAttack(UDFCombatComponent* Self);
	UFUNCTION() void HandleDash(UDFCombatComponent* Self);
	UFUNCTION() void HandleDied(UDFCombatComponent* Self);
	void PlayClip(UAnimSequence* Clip, float Rate);
	int32 ComboIndex = 0;
};

UCLASS()
class ADFPlayerCharacter : public ADFFighterCharacter
{
	GENERATED_BODY()
public:
	ADFPlayerCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DrawFight") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DrawFight") TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawFight|Controls") float MouseSensitivity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawFight|Controls") float MinZoom = 350.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawFight|Controls") float MaxZoom = 1300.f;

	// Fired when a skill key is pressed and the slot is ready. The skill runner (Fable F1) listens here.
	UPROPERTY(BlueprintAssignable) FDFOnSkillCast OnSkillCast;
	// Until the real skill runner exists, a simple area blast stands in for every skill
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DrawFight|Skills") bool bPlaceholderSkills = true;

	UFUNCTION(BlueprintCallable, Category = "DrawFight") bool TryCastSlot(int32 Slot); // 0..2 = 1/2/3, 3 = R
	UFUNCTION(BlueprintPure, Category = "DrawFight") float SlotCooldownLeft(int32 Slot) const;
	UFUNCTION(BlueprintPure, Category = "DrawFight") float SlotCooldownTotal(int32 Slot) const;

	virtual void Tick(float Dt) override;

	// camera shake strength (units); skills add to it, it decays every frame
	void AddShake(float Amount) { Shake = FMath::Min(Shake + Amount, 60.f); }

protected:
	float Shake = 0.f;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* IC) override;
	UFUNCTION() void HandleAutoSkill(int32 Slot);
	// key events (bound directly so quick taps are never missed)
	void KeySkill1() { TryCastSlot(0); }
	void KeySkill2() { TryCastSlot(1); }
	void KeySkill3() { TryCastSlot(2); }
	void KeyUltimate() { TryCastSlot(3); }
	void KeyAttack();
	void KeyDash();
	void KeyJump();
	void KeyAuto();
	void KeyMenu();
	// dev: K / L cycle every skill in DT_Skills onto key 1 (no cooldown) to preview effects
	void KeyTestNext() { CycleTest(1); }
	void KeyTestPrev() { CycleTest(-1); }
	void CycleTest(int32 Step);
	FName TestSkill;
	int32 TestIdx = -1;
	bool CanAct() const;
	bool bKeyW = false, bKeyA = false, bKeyS = false, bKeyD = false;
	void ReadInput(float Dt);
	FName SlotSkill(int32 Slot) const;
	float CdLeft[4] = { 0, 0, 0, 0 };
	float CdTotal[4] = { 1, 1, 1, 1 };
	float TargetZoom = 650.f;
};

UCLASS()
class ADFEnemyCharacter : public ADFFighterCharacter
{
	GENERATED_BODY()
public:
	ADFEnemyCharacter();
};
