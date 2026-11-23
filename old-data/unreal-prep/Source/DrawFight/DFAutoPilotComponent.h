// DFAutoPilotComponent.h
// Walks toward the nearest opponent, attacks in range, dodges telegraphed hits
// and asks for skills when they are ready. Used for:
//   - the player's "Auto" button (bEnabled toggled by UI / key T)
//   - every enemy (always enabled, bDodge = false, bUseSkills = false)
// Ported from the web prototype's p.ai and enemy loop.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DFAutoPilotComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDFOnWantsSkill, int32, Slot);

UCLASS(ClassGroup = (DrawFight), meta = (BlueprintSpawnableComponent))
class UDFAutoPilotComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDFAutoPilotComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto") bool bEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto") bool bDodge = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto") bool bUseSkills = true;
	// Distance (cm) at which skills are considered worth casting
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto") float SkillRange = 1100.f;
	// Seconds between skill requests, so the AI does not dump everything at once
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Auto") float SkillGap = .8f;

	// Blueprint sets this every frame from the skill bar: which slots are off cooldown
	UPROPERTY(BlueprintReadWrite, Category = "Auto") TArray<bool> SkillReady;

	// Fired when the AI wants slot N cast; the skill system listens and casts it
	UPROPERTY(BlueprintAssignable) FDFOnWantsSkill OnWantsSkill;

	UFUNCTION(BlueprintCallable, Category = "Auto") void Toggle() { bEnabled = !bEnabled; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float SkillWait = 0.f;
};
