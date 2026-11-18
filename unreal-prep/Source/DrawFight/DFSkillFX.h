// DFSkillFX.h — skill visuals + hits built in code (no Niagara yet).
// Each skill maps to an archetype (meteor, bolt, burst, projectile, column, vortex, ...)
// tinted by the hero's element. Shapes are engine basic meshes with the additive
// M_Glow material (created by Content/Python/init_unreal.py), plus a real point light.
// Damage, stun, pull, heal and buffs land on the frame the visual hits.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFSkillFX.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UDFCombatComponent;

UCLASS()
class ADFSkillFX : public AActor
{
	GENERATED_BODY()
public:
	ADFSkillFX();

	// Spawns the effect for Skill (DT_Skills row name). Power scales damage (1 = normal, ultimates ~2.5).
	static ADFSkillFX* CastSkill(AActor* Caster, FName Skill, int32 Level, FLinearColor Color, float Power);

	virtual void Tick(float Dt) override;

private:
	enum class EShape : uint8 { Sphere, Cylinder, Cube };
	struct FPiece
	{
		UStaticMeshComponent* Mesh = nullptr;
		UMaterialInstanceDynamic* Mat = nullptr;
		FVector From, To, S0, S1;
		FRotator Rot;
		float T0 = 0, T1 = 1, Spin = 0, Glow = 6;
		bool bEaseOut = true;
	};
	struct FHit
	{
		float At = 0;
		FVector Center;
		float Radius = 300, Damage = 0, Stun = 0, Pull = 0, HealFrac = 0;
		bool bDone = false;
	};

	TArray<FPiece> Pieces;
	TArray<FHit> Hits;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Keep;
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	TWeakObjectPtr<AActor> Caster;
	TWeakObjectPtr<UDFCombatComponent> Combat;
	float Age = 0, Life = 2;
	float LightPeak = 0, LightT0 = 0, LightT1 = 1;
	FVector LightFrom, LightTo;
	FLinearColor Tint = FLinearColor::White;

	void Piece(EShape Shape, FVector From, FVector To, FVector S0, FVector S1, float T0, float T1, FLinearColor C, float Glow = 6.f, FRotator Rot = FRotator::ZeroRotator, float Spin = 0.f);
	void Bolt(FVector A, FVector B, float T0, float T1, FLinearColor C, float Width = .12f);
	void Ring(FVector Center, float R0, float R1, float T0, float T1, FLinearColor C, int32 Count = 24, float Size = .35f);
	void Sparks(FVector Center, int32 Count, float Dist, float T0, float Dur, FLinearColor C, float Size = .15f, bool bUp = true);
	void Hit(float At, FVector Center, float Radius, float Damage, float Stun = 0, float Pull = 0, float HealFrac = 0);
	void Flash(FVector At, float Peak, float T0, float T1, FVector To = FVector::ZeroVector);

	void Build(FName Skill, float Dmg);
	FVector Ground(FVector P) const;
	FVector TargetPoint(float MaxDist) const;
	TArray<FVector> Targets(int32 Max, float MaxDist) const;
};
