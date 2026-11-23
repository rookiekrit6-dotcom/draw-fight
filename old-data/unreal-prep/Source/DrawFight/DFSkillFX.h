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
	static ADFSkillFX* CastSkill(AActor* Caster, FName Skill, int32 Level, FLinearColor Color, float Power, FName Element = NAME_None);

	// visual-only burst (dash trails, deaths): no damage
	static void Puff(AActor* Around, FVector Loc, FLinearColor Color, FName Element, float Scale, bool bTrail = false);

	virtual void Tick(float Dt) override;

private:
	enum class EShape : uint8 { Sphere, Cylinder, Cube, Plane, Cone };
	// textured sprites (painted with code, imported by init_unreal.py)
	enum class ETex : uint8 { SoftGlow, Ring, Spark, Star, Slash, Smoke1, Smoke2, Smoke3, Shock, Crack,
		Circle1, Circle2, Circle3, Flame, Shard, Leaf, Bolt, Bubble, Drop, Spore, Gear, Swirl, Count };
	enum class EFace : uint8 { None, Camera, Flat, Axial };
	struct FPiece
	{
		UStaticMeshComponent* Mesh = nullptr;
		UMaterialInstanceDynamic* Mat = nullptr;
		FVector From, To, S0, S1;
		FRotator Rot;
		float T0 = 0, T1 = 1, Spin = 0, Glow = 6;
		bool bEaseOut = true;
		// physics particles: From + Vel*t + gravity, stretched along velocity when bStreak
		bool bPhys = false, bStreak = false;
		FVector Vel = FVector::ZeroVector;
		float Grav = 0.f, Drag = 0.f;
		// sprite facing
		EFace Face = EFace::None;
		FVector Axis = FVector::ZeroVector;
		float Roll = 0.f, RollSpin = 0.f;
		// motion helpers: follow an actor (From/To become offsets), curve through Ctrl, orbit around the path point
		TWeakObjectPtr<AActor> Follow, Chase; // Chase: To tracks this actor (homing)
		FVector LastFollow = FVector::ZeroVector;
		bool bCurve = false;
		FVector Ctrl = FVector::ZeroVector;
		float OrbitR = 0.f, OrbitSpeed = 0.f, OrbitPhase = 0.f;
		bool bFlicker = false;
	};
	struct FHit
	{
		float At = 0;
		FVector Center;
		float Radius = 300, Damage = 0, Stun = 0, Pull = 0, HealFrac = 0;
		bool bDone = false;
		// extras
		float Lift = 0, Push = 0, SlowMul = 0, SlowDur = 0, Drain = 0, FxScale = 0;
		bool bAtCaster = false;            // centre = caster location + Offset (rotated by caster yaw)
		FVector Offset = FVector::ZeroVector;
		TWeakObjectPtr<AActor> Target;     // centre = this actor (skip if gone)
		bool bPullToCaster = false;
		bool bShot = false;                // turret shot: fire a bolt from Center at the nearest enemy in Radius
	};

	TArray<FPiece> Pieces;
	// ---- dense instanced particles (one draw call per texture) ----
	struct FMote
	{
		uint8 Tex = 0; int32 Index = 0;
		FVector P = FVector::ZeroVector, V = FVector::ZeroVector;
		float Grav = 0, Drag = 0, T0 = 0, T1 = 1, S0 = .2f, S1 = 0, Roll = 0, RollSpin = 0, Glow = 4;
		FLinearColor C = FLinearColor::White;
		bool bStreak = false;
		TWeakObjectPtr<AActor> Follow; FVector LastFollow = FVector::ZeroVector;
	};
	TArray<FMote> Motes;
	UPROPERTY() TMap<uint8, TObjectPtr<class UInstancedStaticMeshComponent>> MoteISM;
	FMote& Mote(ETex Tex, FVector P, FVector V, float T0, float Dur, float S0, float S1, FLinearColor C, float Glow = 4.f);
	void TickMotes(float Dt);
	// element look
	FName Element;
	ETex ElemTex() const;
	void ElementBurst(FVector Center, int32 Count, float Speed, float T0, float Dur, FLinearColor C, float Size = .35f, bool bUp = true);
	void CastCircle(FVector Feet, float Size, float Dur, FLinearColor C, bool bUlt);
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
	// one textured sprite; sizes are in metres-ish (1 = 100 units). Dust = soft translucent (not glowing)
	void Sprite(ETex Tex, EFace Face, FVector From, FVector To, float S0, float S1, float T0, float T1, FLinearColor C, float Glow = 4.f, float Roll = 0.f, float RollSpin = 0.f, bool bDust = false, float Stretch = 1.f);
	void ToSprite(FPiece& P, ETex Tex, EFace Face, bool bDust, FLinearColor C, float Glow);
	FVector CamLoc;
	void Bolt(FVector A, FVector B, float T0, float T1, FLinearColor C, float Width = .12f);
	void Ring(FVector Center, float R0, float R1, float T0, float T1, FLinearColor C, int32 Count = 24, float Size = .35f);
	void Sparks(FVector Center, int32 Count, float Dist, float T0, float Dur, FLinearColor C, float Size = .15f, bool bUp = true);
	FHit& Hit(float At, FVector Center, float Radius, float Damage, float Stun = 0, float Pull = 0, float HealFrac = 0);
	FPiece& Last() { return Pieces.Last(); }
	TArray<AActor*> TargetActors(int32 Max, float MaxDist) const;
	void Beam(FVector Start, FVector Dir, float Len, float T0, float Dur, FLinearColor C, FLinearColor Hot, float Width, float Dmg);
	void Crystal(FVector Base, float Height, float T0, float Dur, FLinearColor C, float Tilt = 12.f);
	void SpawnAlly(FVector At, float Life, float DmgMul);
	// end-of-effect restore (transform / vanish)
	float RestoreAt = -1.f;
	FVector RestoreScale = FVector::OneVector;
	bool bRestoreHidden = false, bRestored = false;
	void Streaks(FVector Center, int32 Count, float Speed, float T0, float Dur, FLinearColor C, float Width = .05f, float Grav = -1600.f, bool bUp = true);
	void Embers(FVector Center, int32 Count, float Spread, float T0, float Dur, FLinearColor C);
	void Smoke(FVector Center, int32 Count, float Size, float T0, float Dur, FLinearColor C);
	void Impact(FVector Center, float Scale, float T0, FLinearColor C, FLinearColor Hot);
	float ShakeOnHit = 8.f, StopOnHit = 0.f;
	bool bImpactFelt = false;
	int32 SfxPlayed = 0;
	void Feel();
	void Flash(FVector At, float Peak, float T0, float T1, FVector To = FVector::ZeroVector);

	void Build(FName Skill, float Dmg);
	FVector Ground(FVector P) const;
	FVector TargetPoint(float MaxDist) const;
	TArray<FVector> Targets(int32 Max, float MaxDist) const;
};
