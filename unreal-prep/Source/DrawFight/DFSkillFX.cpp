#include "DFSkillFX.h"
#include "DFCombatComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	UStaticMesh* ShapeMesh(int32 I)
	{
		static UStaticMesh* M[3] = { nullptr, nullptr, nullptr };
		static const TCHAR* P[3] = { TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Engine/BasicShapes/Cube.Cube") };
		if (!M[I]) M[I] = LoadObject<UStaticMesh>(nullptr, P[I]);
		return M[I];
	}
	UMaterialInterface* GlowMat()
	{
		static UMaterialInterface* M = nullptr;
		if (!M) M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DrawFight/FX/M_Glow.M_Glow"));
		return M;
	}
	FLinearColor Mix(FLinearColor A, FLinearColor B, float T) { return A * (1.f - T) + B * T; }
	const FLinearColor White(1.f, 1.f, 1.f);
	const FLinearColor Gold(1.f, .75f, .25f);
	const FLinearColor Fire(1.f, .35f, .08f);
	const FLinearColor Electric(.55f, .75f, 1.f);
	const FLinearColor Heal(.35f, 1.f, .55f);
	const FLinearColor Void(.55f, .2f, 1.f);
	const FLinearColor Poison(.45f, .9f, .2f);

	// skill -> archetype
	enum class EArch : uint8 { Burst, Quake, Projectile, Fan, Wave, Dash, Meteor, Sun, Bolt, Column, Shield, Haste, Vortex, Tornado, Cloud, Pause, Slash, Rain };
	EArch ArchOf(FName S)
	{
		static const TMap<FName, EArch> Map = {
			{ "slam", EArch::Quake }, { "quake", EArch::Quake }, { "leap", EArch::Quake }, { "blast", EArch::Burst }, { "spin", EArch::Burst },
			{ "rage", EArch::Burst }, { "counter", EArch::Burst }, { "eraser", EArch::Burst }, { "scribble", EArch::Fan },
			{ "throw", EArch::Fan }, { "volley", EArch::Rain }, { "inkspill", EArch::Projectile }, { "ruler", EArch::Slash }, { "pin", EArch::Fan },
			{ "clip", EArch::Projectile }, { "lash", EArch::Slash }, { "thrust", EArch::Dash }, { "charge", EArch::Dash }, { "shadow", EArch::Dash },
			{ "wave", EArch::Wave }, { "meteor", EArch::Meteor }, { "sun", EArch::Sun }, { "pageflip", EArch::Pause },
			{ "thunder", EArch::Bolt }, { "rift", EArch::Slash }, { "heal", EArch::Column }, { "barrier", EArch::Shield }, { "haste", EArch::Haste },
			{ "vortex", EArch::Vortex }, { "tornado", EArch::Tornado }, { "miasma", EArch::Cloud }, { "pause", EArch::Pause },
			{ "ult_demonlord", EArch::Vortex }, { "ult_riftwalk", EArch::Slash }, { "ult_beastcall", EArch::Fan }, { "ult_shadowarmy", EArch::Burst },
			{ "ult_soulharvest", EArch::Vortex }, { "ult_timerewind", EArch::Pause }, { "ult_dragonform", EArch::Sun },
			{ "ult_thousandblades", EArch::Rain }, { "ult_livingcanvas", EArch::Fan } };
		const EArch* A = Map.Find(S);
		return A ? *A : EArch::Burst;
	}
}

ADFSkillFX::ADFSkillFX()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->SetIntensity(0.f);
	Light->bUseInverseSquaredFalloff = false;
	Light->SetAttenuationRadius(1600.f);
	Light->SetCastShadows(false);
}

ADFSkillFX* ADFSkillFX::CastSkill(AActor* InCaster, FName Skill, int32 Level, FLinearColor Color, float Power)
{
	if (!InCaster || !InCaster->GetWorld()) return nullptr;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADFSkillFX* FX = InCaster->GetWorld()->SpawnActor<ADFSkillFX>(ADFSkillFX::StaticClass(), InCaster->GetActorLocation(), FRotator::ZeroRotator, SP);
	if (!FX) return nullptr;
	FX->Caster = InCaster;
	FX->Combat = InCaster->FindComponentByClass<UDFCombatComponent>();
	FX->Tint = Color;
	const float Base = FX->Combat.IsValid() ? FX->Combat->Damage : 12.f;
	FX->Build(Skill, Base * Power * (1.f + .25f * FMath::Max(0, Level - 1)));
	return FX;
}

// ---------- building blocks ----------
void ADFSkillFX::Piece(EShape Shape, FVector From, FVector To, FVector S0, FVector S1, float T0, float T1, FLinearColor C, float Glow, FRotator Rot, float Spin)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(ShapeMesh((int32)Shape));
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCastShadow(false);
	M->SetupAttachment(RootComponent);
	M->SetUsingAbsoluteLocation(true);
	M->SetUsingAbsoluteRotation(true);
	M->SetUsingAbsoluteScale(true);
	M->RegisterComponent();
	M->SetVisibility(false);
	FPiece P;
	P.Mesh = M;
	if (UMaterialInterface* G = GlowMat())
	{
		P.Mat = UMaterialInstanceDynamic::Create(G, this);
		P.Mat->SetVectorParameterValue(TEXT("Color"), C);
		P.Mat->SetScalarParameterValue(TEXT("Glow"), Glow);
		P.Mat->SetScalarParameterValue(TEXT("Opacity"), 0.f);
		M->SetMaterial(0, P.Mat);
	}
	P.From = From; P.To = To; P.S0 = S0; P.S1 = S1; P.Rot = Rot; P.T0 = T0; P.T1 = T1; P.Spin = Spin; P.Glow = Glow;
	Pieces.Add(P);
	Keep.Add(M);
	Life = FMath::Max(Life, T1 + .1f);
}

void ADFSkillFX::Bolt(FVector A, FVector B, float T0, float T1, FLinearColor C, float Width)
{
	// jagged lightning: segments between jittered points (cylinder is 100 units tall along Z)
	const int32 N = 7;
	FVector Prev = A;
	for (int32 I = 1; I <= N; ++I)
	{
		FVector P = FMath::Lerp(A, B, (float)I / N);
		if (I < N) P += FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 0);
		const FVector D = P - Prev;
		const FRotator R = FRotationMatrix::MakeFromZ(D.GetSafeNormal()).Rotator();
		const FVector Mid = (Prev + P) * .5f;
		const FVector S(Width, Width, D.Size() / 100.f);
		Piece(EShape::Cylinder, Mid, Mid, S, S * FVector(.3f, .3f, 1.f), T0, T1, C, 14.f, R);
		Prev = P;
	}
}

void ADFSkillFX::Ring(FVector Center, float R0, float R1, float T0, float T1, FLinearColor C, int32 Count, float Size)
{
	for (int32 I = 0; I < Count; ++I)
	{
		const float A = 2.f * PI * I / Count;
		const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0);
		Piece(EShape::Sphere, Center + Dir * R0, Center + Dir * R1, FVector(Size), FVector(Size * .3f), T0, T1, C, 8.f);
	}
}

void ADFSkillFX::Sparks(FVector Center, int32 Count, float Dist, float T0, float Dur, FLinearColor C, float Size, bool bUp)
{
	for (int32 I = 0; I < Count; ++I)
	{
		FVector Dir = FMath::VRand();
		if (bUp) Dir.Z = FMath::Abs(Dir.Z) * 1.2f + .2f;
		const float T = T0 + FMath::FRandRange(0.f, .08f);
		Piece(EShape::Sphere, Center, Center + Dir * Dist * FMath::FRandRange(.5f, 1.f), FVector(Size), FVector(Size * .1f), T, T + Dur * FMath::FRandRange(.7f, 1.2f), C, 10.f);
	}
}

void ADFSkillFX::Hit(float At, FVector Center, float Radius, float Damage, float Stun, float Pull, float HealFrac)
{
	FHit H;
	H.At = At; H.Center = Center; H.Radius = Radius; H.Damage = Damage; H.Stun = Stun; H.Pull = Pull; H.HealFrac = HealFrac;
	Hits.Add(H);
	Life = FMath::Max(Life, At + .1f);
}

void ADFSkillFX::Flash(FVector At, float Peak, float T0, float T1, FVector To)
{
	LightFrom = At;
	LightTo = To.IsZero() ? At : To;
	LightPeak = Peak; LightT0 = T0; LightT1 = T1;
	Light->SetLightColor(Mix(Tint, White, .3f));
}

FVector ADFSkillFX::Ground(FVector P) const
{
	FHitResult HR;
	const FVector A = P + FVector(0, 0, 500), B = P - FVector(0, 0, 3000);
	FCollisionQueryParams Q;
	Q.AddIgnoredActor(Caster.Get());
	if (GetWorld()->LineTraceSingleByChannel(HR, A, B, ECC_WorldStatic, Q)) return HR.ImpactPoint;
	return P - FVector(0, 0, 90);
}

FVector ADFSkillFX::TargetPoint(float MaxDist) const
{
	const AActor* C = Caster.Get();
	if (Combat.IsValid())
		if (UDFCombatComponent* E = Combat->FindNearestEnemy(MaxDist))
			return E->GetOwner()->GetActorLocation();
	return C->GetActorLocation() + C->GetActorForwardVector() * 600.f;
}

TArray<FVector> ADFSkillFX::Targets(int32 Max, float MaxDist) const
{
	TArray<TPair<float, FVector>> L;
	const FVector Me = Caster->GetActorLocation();
	const int32 Team = Combat.IsValid() ? Combat->Team : 0;
	for (UDFCombatComponent* O : UDFCombatComponent::Registry())
		if (O && O->Team != Team && !O->IsDead() && O->GetOwner())
		{
			const float D = FVector::Dist(O->GetOwner()->GetActorLocation(), Me);
			if (D < MaxDist) L.Add({ D, O->GetOwner()->GetActorLocation() });
		}
	L.Sort([](const TPair<float, FVector>& A, const TPair<float, FVector>& B) { return A.Key < B.Key; });
	TArray<FVector> Out;
	for (int32 I = 0; I < L.Num() && I < Max; ++I) Out.Add(L[I].Value);
	return Out;
}

// ---------- archetypes ----------
void ADFSkillFX::Build(FName Skill, float Dmg)
{
	AActor* C = Caster.Get();
	const FVector Me = C->GetActorLocation();
	const FVector Feet = Ground(Me);
	FVector Fwd = TargetPoint(2500.f) - Me; Fwd.Z = 0; Fwd = Fwd.GetSafeNormal();
	if (Fwd.IsNearlyZero()) Fwd = C->GetActorForwardVector();
	const FLinearColor T = Tint, Hot = Mix(Tint, White, .5f);
	const bool bUlt = Skill.ToString().StartsWith(TEXT("ult_"));
	const float Big = bUlt ? 1.8f : 1.f;

	switch (ArchOf(Skill))
	{
	case EArch::Quake: // ground slam: shock rings + debris + dust column
	{
		Piece(EShape::Cylinder, Feet, Feet, FVector(.5f, .5f, .02f), FVector(9.f, 9.f, .02f) * Big, 0.f, .45f, Hot, 10.f);
		Ring(Feet + FVector(0, 0, 20), 60, 650 * Big, 0.f, .5f, T, 32, .4f);
		Ring(Feet + FVector(0, 0, 10), 40, 420 * Big, .12f, .6f, Hot, 20, .3f);
		for (int32 I = 0; I < 14; ++I)
		{
			const FVector D = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0).GetSafeNormal() * FMath::FRandRange(80.f, 420.f) * Big;
			Piece(EShape::Cube, Feet + D * .3f, Feet + D + FVector(0, 0, FMath::FRandRange(80.f, 260.f)), FVector(.25f), FVector(.08f), .02f, .7f, Mix(T, Gold, .4f), 3.f, FRotator(FMath::FRandRange(0.f, 90.f), FMath::FRandRange(0.f, 90.f), 0), 400.f);
		}
		Flash(Feet + FVector(0, 0, 100), 60.f, 0.f, .45f);
		Hit(.05f, Feet, 520.f * Big, Dmg * 2.2f, .4f);
		break;
	}
	case EArch::Burst: // nova around self
	{
		Piece(EShape::Sphere, Me, Me, FVector(.3f), FVector(7.f) * Big, 0.f, .4f, Hot, 8.f);
		Ring(Me, 50, 600 * Big, 0.f, .45f, T, 28, .35f);
		Sparks(Me, 24, 500 * Big, 0.f, .5f, T);
		Flash(Me, 70.f, 0.f, .4f);
		Hit(.08f, Me, 480.f * Big, Dmg * 2.f);
		break;
	}
	case EArch::Projectile: case EArch::Fan: case EArch::Rain:
	{
		const bool bFan = ArchOf(Skill) == EArch::Fan, bRain = ArchOf(Skill) == EArch::Rain;
		const int32 N = bRain ? (bUlt ? 24 : 12) : bFan ? (bUlt ? 9 : 5) : 1;
		const FVector Tgt = TargetPoint(2500.f);
		for (int32 I = 0; I < N; ++I)
		{
			FVector From, To; float T0;
			if (bRain)
			{
				To = Ground(Tgt + FVector(FMath::FRandRange(-350.f, 350.f), FMath::FRandRange(-350.f, 350.f), 0) * Big);
				From = To + FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), 1400);
				T0 = .05f * I;
			}
			else
			{
				const float A = bFan ? FMath::DegreesToRadians(-30.f + 60.f * I / FMath::Max(1, N - 1)) : 0.f;
				const FVector D = Fwd.RotateAngleAxis(FMath::RadiansToDegrees(A), FVector::UpVector);
				From = Me + D * 80 + FVector(0, 0, 40);
				To = From + D * 1400;
				T0 = .03f * I;
			}
			const float Fly = bRain ? .35f : .45f;
			const FRotator R = FRotationMatrix::MakeFromZ((To - From).GetSafeNormal()).Rotator();
			Piece(EShape::Cylinder, From, To, FVector(.12f, .12f, .9f), FVector(.12f, .12f, .9f), T0, T0 + Fly, Hot, 12.f, R);
			Piece(EShape::Sphere, From, To, FVector(.35f), FVector(.35f), T0, T0 + Fly, T, 8.f);
			Sparks(To, 6, 160, T0 + Fly, .3f, T, .1f);
			if (bRain) Hit(T0 + Fly, To, 180.f, Dmg * .7f);
			else for (int32 K = 1; K <= 4; ++K) Hit(T0 + Fly * K / 4.f, FMath::Lerp(From, To, K / 4.f), 150.f, Dmg * (bFan ? .5f : 1.2f) / 2.f);
		}
		Flash(Me + Fwd * 200, 35.f, 0.f, .5f, Me + Fwd * 1200);
		break;
	}
	case EArch::Wave: case EArch::Slash: // crescent slash travelling forward
	{
		const int32 Waves = ArchOf(Skill) == EArch::Slash && bUlt ? 4 : 1;
		for (int32 W = 0; W < Waves; ++W)
		{
			const FVector Dir = Waves > 1 ? Fwd.RotateAngleAxis(90.f * W, FVector::UpVector) : Fwd;
			const FVector Side = FVector::CrossProduct(Dir, FVector::UpVector);
			const float T0 = .1f * W;
			for (int32 I = -6; I <= 6; ++I)
			{
				const float Bend = FMath::Square(I / 6.f) * -120.f;
				const FVector Off = Side * I * 45.f + Dir * Bend + FVector(0, 0, 40);
				Piece(EShape::Sphere, Me + Off, Me + Off + Dir * 1300, FVector(.45f, .45f, .2f), FVector(.3f, .3f, .1f), T0, T0 + .55f, I == 0 ? Hot : T, 10.f);
			}
			for (int32 K = 1; K <= 6; ++K) Hit(T0 + .55f * K / 6.f, Me + Dir * 1300 * K / 6.f, 260.f, Dmg * .5f);
		}
		Flash(Me, 45.f, 0.f, .55f, Me + Fwd * 1300);
		break;
	}
	case EArch::Dash: // dash through: afterimages + trail
	{
		const FVector End = Me + Fwd * 650.f;
		for (int32 I = 0; I < 8; ++I)
		{
			const FVector P = FMath::Lerp(Me, End, I / 7.f);
			Piece(EShape::Cylinder, P, P, FVector(.5f, .5f, 1.8f), FVector(.1f, .1f, 1.8f), .02f * I, .02f * I + .35f, T, 6.f);
		}
		if (ACharacter* Ch = Cast<ACharacter>(C)) Ch->LaunchCharacter(Fwd * 2600.f + FVector(0, 0, 150), true, true);
		Sparks(End, 16, 300, .2f, .4f, Hot);
		for (int32 K = 1; K <= 4; ++K) Hit(.05f * K, FMath::Lerp(Me, End, K / 4.f), 220.f, Dmg * .55f);
		Flash(Me, 40.f, 0.f, .35f, End);
		break;
	}
	case EArch::Meteor: case EArch::Sun:
	{
		const bool bSun = ArchOf(Skill) == EArch::Sun;
		const int32 N = bUlt ? 5 : 1;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector Land = Ground(TargetPoint(2500.f) + (I ? FVector(FMath::FRandRange(-500.f, 500.f), FMath::FRandRange(-500.f, 500.f), 0) : FVector::ZeroVector));
			const FVector Sky = Land + FVector(-500, -300, 2200);
			const float Fall = bSun ? 1.1f : .75f, T0 = .25f * I, Sz = (bSun ? 4.5f : 2.2f);
			// warning circle on the ground
			Piece(EShape::Cylinder, Land, Land, FVector(5.f, 5.f, .01f) * Sz / 2.2f, FVector(5.5f, 5.5f, .01f) * Sz / 2.2f, T0, T0 + Fall, FLinearColor(1, .1f, .05f), 3.f);
			// rock core + fire shell + trail
			Piece(EShape::Sphere, Sky, Land, FVector(Sz * .6f), FVector(Sz * .6f), T0, T0 + Fall, Mix(Fire, T, .3f), 4.f, FRotator::ZeroRotator, 200.f);
			Piece(EShape::Sphere, Sky, Land, FVector(Sz), FVector(Sz * 1.1f), T0, T0 + Fall, bSun ? Gold : Fire, 10.f);
			for (int32 K = 1; K <= 6; ++K)
			{
				const float d = .06f * K;
				Piece(EShape::Sphere, Sky, Land, FVector(Sz * (1.f - K * .12f)), FVector(Sz * .2f), T0 + d, T0 + Fall + d, Fire, 6.f);
			}
			// impact
			const float Ti = T0 + Fall;
			Piece(EShape::Sphere, Land, Land, FVector(1.f), FVector(Sz * 4.f), Ti, Ti + .5f, Mix(Gold, White, .5f), 14.f);
			Ring(Land + FVector(0, 0, 20), 100, 900 * Sz / 2.2f, Ti, Ti + .6f, Fire, 36, .5f);
			Sparks(Land, 30, 700, Ti, .7f, Gold, .2f);
			Hit(Ti, Land, 450.f * Sz / 2.2f, Dmg * (bSun ? 4.5f : 3.5f) / FMath::Sqrt((float)N), .3f);
		}
		const FVector L0 = Ground(TargetPoint(2500.f));
		Flash(L0 + FVector(-500, -300, 2200), 90.f, 0.f, (bSun ? 1.1f : .75f) + .5f + .25f * (N - 1), L0 + FVector(0, 0, 150));
		break;
	}
	case EArch::Bolt: // chain lightning from the sky onto up to 4 enemies
	{
		TArray<FVector> Ts = Targets(bUlt ? 8 : 4, 2200.f);
		if (!Ts.Num()) Ts.Add(TargetPoint(2200.f));
		for (int32 I = 0; I < Ts.Num(); ++I)
		{
			const FVector G = Ground(Ts[I]);
			const float T0 = .12f * I;
			Bolt(G + FVector(0, 0, 1800), G, T0, T0 + .25f, Electric, .14f);
			Bolt(G + FVector(0, 0, 1800), G, T0 + .05f, T0 + .3f, White, .06f);
			Piece(EShape::Sphere, G, G, FVector(.5f), FVector(4.f), T0, T0 + .35f, Electric, 14.f);
			Sparks(G, 14, 350, T0, .35f, Electric, .1f);
			Hit(T0 + .02f, G, 260.f, Dmg * 2.6f, .6f);
		}
		Flash(Ts[0] + FVector(0, 0, 400), 120.f, 0.f, .2f + .12f * Ts.Num());
		break;
	}
	case EArch::Column: // healing pillar
	{
		Piece(EShape::Cylinder, Feet + FVector(0, 0, 400), Feet + FVector(0, 0, 400), FVector(1.f, 1.f, 8.f), FVector(2.2f, 2.2f, 8.f), 0.f, 1.f, Heal, 5.f);
		Ring(Feet + FVector(0, 0, 10), 300, 80, 0.f, .8f, Heal, 20, .3f);
		for (int32 I = 0; I < 18; ++I)
		{
			const FVector P = Feet + FVector(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f), 0);
			const float T0 = FMath::FRandRange(0.f, .6f);
			Piece(EShape::Sphere, P, P + FVector(0, 0, FMath::FRandRange(300.f, 700.f)), FVector(.15f), FVector(.02f), T0, T0 + .6f, Mix(Heal, White, .4f), 10.f);
		}
		Flash(Feet + FVector(0, 0, 200), 50.f, 0.f, 1.f);
		Hit(.3f, Me, 1.f, 0.f, 0.f, 0.f, bUlt ? .5f : .25f);
		break;
	}
	case EArch::Shield: case EArch::Haste:
	{
		const bool bShield = ArchOf(Skill) == EArch::Shield;
		Piece(EShape::Sphere, Me, Me, FVector(2.f), FVector(bShield ? 3.2f : 2.6f), 0.f, bShield ? 1.2f : .6f, bShield ? Mix(T, Electric, .4f) : Mix(T, Gold, .5f), 3.f, FRotator::ZeroRotator, 60.f);
		for (int32 K = 0; K < 3; ++K)
			Ring(Feet + FVector(0, 0, 40 + K * 60), 160, bShield ? 170 : 420, .1f * K, .1f * K + .6f, T, 18, .2f);
		if (Combat.IsValid()) Combat->AddBuff(bShield ? EDFBuff::Guard : EDFBuff::Speed, bShield ? .5f : 1.4f, bShield ? 4.f : 5.f);
		Flash(Me, 30.f, 0.f, .6f);
		break;
	}
	case EArch::Vortex: case EArch::Tornado: case EArch::Cloud:
	{
		const EArch A = ArchOf(Skill);
		const FVector Tgt = Ground(TargetPoint(1800.f));
		const FLinearColor Col = A == EArch::Cloud ? Poison : A == EArch::Vortex ? Mix(Void, T, .3f) : Mix(T, White, .3f);
		const float Dur = 2.2f, R = (A == EArch::Cloud ? 380.f : 320.f) * Big;
		if (A == EArch::Tornado)
			for (int32 K = 0; K < 8; ++K)
			{
				const FVector P = Tgt + FVector(0, 0, 60 + K * 70);
				Piece(EShape::Cylinder, P, P + Fwd * 400, FVector(.6f + K * .25f, .6f + K * .25f, .5f), FVector(.8f + K * .3f, .8f + K * .3f, .5f), 0.f, Dur, Col, 4.f, FRotator::ZeroRotator, 500.f + K * 60.f);
			}
		else
		{
			Piece(EShape::Cylinder, Tgt, Tgt, FVector(R / 50.f, R / 50.f, .02f), FVector(R / 60.f, R / 60.f, .02f), 0.f, Dur, Col, 4.f, FRotator::ZeroRotator, 180.f);
			for (int32 K = 0; K < 5; ++K) Ring(Tgt + FVector(0, 0, 30 + K * 25), R, A == EArch::Vortex ? 30.f : R * 1.1f, K * .4f, K * .4f + .8f, Col, 16, .3f);
			if (A == EArch::Cloud)
				for (int32 I = 0; I < 14; ++I)
				{
					const FVector P = Tgt + FVector(FMath::FRandRange(-R, R), FMath::FRandRange(-R, R), FMath::FRandRange(30.f, 160.f));
					Piece(EShape::Sphere, P, P + FVector(0, 0, 80), FVector(1.2f), FVector(2.2f), FMath::FRandRange(0.f, .6f), Dur, Col, 1.5f);
				}
		}
		for (int32 K = 0; K < 8; ++K) Hit(.25f * K + .1f, Tgt, R + 60.f, Dmg * .45f, 0.f, A == EArch::Cloud ? 0.f : 380.f);
		Flash(Tgt + FVector(0, 0, 150), 50.f, 0.f, Dur);
		break;
	}
	case EArch::Pause: // freeze everything: grey shock sphere + stun all
	{
		Piece(EShape::Sphere, Me, Me, FVector(.5f), FVector(40.f), 0.f, .8f, FLinearColor(.8f, .85f, 1.f), 2.f);
		Ring(Me, 100, 1500, 0.f, .8f, White, 40, .5f);
		Flash(Me, 150.f, 0.f, .6f);
		Hit(.2f, Me, 1800.f, Dmg * .8f, bUlt ? 3.5f : 2.2f);
		if (bUlt) Hit(.2f, Me, 1.f, 0.f, 0.f, 0.f, .35f);
		break;
	}
	}
}

// ---------- update ----------
void ADFSkillFX::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;

	for (FPiece& P : Pieces)
	{
		if (!P.Mesh) continue;
		const bool bOn = Age >= P.T0 && Age <= P.T1;
		P.Mesh->SetVisibility(bOn);
		if (!bOn) continue;
		const float A = FMath::Clamp((Age - P.T0) / FMath::Max(.01f, P.T1 - P.T0), 0.f, 1.f);
		const float E = 1.f - FMath::Square(1.f - A); // ease out
		P.Mesh->SetWorldLocation(FMath::Lerp(P.From, P.To, P.From == P.To ? 0.f : A));
		P.Mesh->SetWorldScale3D(FMath::Lerp(P.S0, P.S1, E));
		if (P.Spin != 0.f) P.Mesh->SetWorldRotation(P.Rot + FRotator(0, P.Spin * Age, 0));
		else P.Mesh->SetWorldRotation(P.Rot);
		const float Op = FMath::Min(1.f, A / .1f) * (A > .6f ? (1.f - A) / .4f : 1.f);
		if (P.Mat) P.Mat->SetScalarParameterValue(TEXT("Opacity"), Op);
	}

	if (LightPeak > 0.f)
	{
		const float A = FMath::Clamp((Age - LightT0) / FMath::Max(.01f, LightT1 - LightT0), 0.f, 1.f);
		Light->SetWorldLocation(FMath::Lerp(LightFrom, LightTo, A));
		Light->SetIntensity(Age < LightT0 || Age > LightT1 ? 0.f : LightPeak * FMath::Sin(A * PI));
	}

	const int32 Team = Combat.IsValid() ? Combat->Team : 0;
	for (FHit& H : Hits)
	{
		if (H.bDone || Age < H.At) continue;
		H.bDone = true;
		if (H.HealFrac > 0.f && Combat.IsValid()) Combat->Heal(Combat->MaxHP * H.HealFrac);
		if (H.Damage <= 0.f && H.Stun <= 0.f && H.Pull <= 0.f) continue;
		for (UDFCombatComponent* O : UDFCombatComponent::Registry())
		{
			if (!O || O->Team == Team || O->IsDead() || !O->GetOwner()) continue;
			const FVector P = O->GetOwner()->GetActorLocation();
			if (FVector::Dist2D(P, H.Center) > H.Radius + O->Radius) continue;
			if (H.Damage > 0.f) O->ApplyDamage(H.Damage, Caster.Get());
			if (H.Stun > 0.f) O->Stun(H.Stun);
			if (H.Pull > 0.f)
				if (ACharacter* Ch = Cast<ACharacter>(O->GetOwner()))
				{
					FVector To = H.Center - P; To.Z = 0;
					if (To.Size() > 60.f) Ch->LaunchCharacter(To.GetSafeNormal() * H.Pull, true, false);
				}
		}
	}

	if (Age > Life) Destroy();
}
