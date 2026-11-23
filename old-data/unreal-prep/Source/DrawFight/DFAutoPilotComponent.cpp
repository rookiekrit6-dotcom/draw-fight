#include "DFAutoPilotComponent.h"
#include "DFCombatComponent.h"
#include "GameFramework/Character.h"

UDFAutoPilotComponent::UDFAutoPilotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SkillReady.Init(false, 3);
}

void UDFAutoPilotComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(Dt, TickType, ThisTickFunction);
	if (!bEnabled) return;

	ACharacter* Me = Cast<ACharacter>(GetOwner());
	UDFCombatComponent* C = Me ? Me->FindComponentByClass<UDFCombatComponent>() : nullptr;
	if (!C || C->IsDead() || C->IsStunned()) return;

	SkillWait -= Dt;
	UDFCombatComponent* T = C->FindNearestEnemy();
	if (!T) return;

	const FVector MyPos = Me->GetActorLocation();
	FVector To = T->GetOwner()->GetActorLocation() - MyPos;
	To.Z = 0.f;
	const float Dist = To.Size() - T->Radius;

	// 1) dodge a telegraphed hit that is about to land
	if (bDodge && C->CanDash())
	{
		for (UDFCombatComponent* O : UDFCombatComponent::Registry())
		{
			if (!O || O->Team == C->Team || O->IsDead() || !O->IsWindingUp() || O->WindUpRemaining() > .3f) continue;
			const float D = FVector::Dist2D(O->GetOwner()->GetActorLocation(), MyPos);
			if (D < O->Reach + C->Radius + 60.f)
			{
				const FVector Away = (MyPos - O->GetOwner()->GetActorLocation()).GetSafeNormal2D();
				C->TryDash(FVector::CrossProduct(Away, FVector::UpVector) + Away * .5f); // sidestep
				return;
			}
		}
	}

	// 2) skills
	if (bUseSkills && SkillWait <= 0.f && Dist < SkillRange)
	{
		for (int32 I = 0; I < SkillReady.Num(); ++I)
			if (SkillReady[I]) { OnWantsSkill.Broadcast(I); SkillWait = SkillGap; SkillReady[I] = false; break; }
	}

	// 3) close in or attack
	if (C->IsBusy()) return;
	if (Dist > C->Reach * .8f) Me->AddMovementInput(To.GetSafeNormal());
	else C->TryAttack();
}
