#include "DFSkillFX.h"
#include "DFCombatComponent.h"
#include "DFFighterCharacter.h"
#include "DFAudio.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
	UStaticMesh* ShapeMesh(int32 I)
	{
		static UStaticMesh* M[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		static const TCHAR* P[5] = { TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Engine/BasicShapes/Plane.Plane"), TEXT("/Engine/BasicShapes/Cone.Cone") };
		if (!M[I]) M[I] = LoadObject<UStaticMesh>(nullptr, P[I]);
		return M[I];
	}
	UMaterialInterface* GlowMat()
	{
		static UMaterialInterface* M = nullptr;
		if (!M) M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DrawFight/FX/M_Glow.M_Glow"));
		return M;
	}
	UMaterialInterface* SpriteMat(bool bDust)
	{
		static UMaterialInterface* M[2] = { nullptr, nullptr };
		if (!M[bDust]) M[bDust] = LoadObject<UMaterialInterface>(nullptr, bDust ? TEXT("/Game/DrawFight/FX/M_Dust.M_Dust") : TEXT("/Game/DrawFight/FX/M_Sprite.M_Sprite"));
		return M[bDust];
	}
	UTexture* SpriteTex(int32 I)
	{
		static UTexture* T[32] = {};
		static const TCHAR* N[] = { TEXT("T_SoftGlow"), TEXT("T_Ring"), TEXT("T_Spark"), TEXT("T_Star"), TEXT("T_Slash"), TEXT("T_Smoke1"), TEXT("T_Smoke2"), TEXT("T_Smoke3"), TEXT("T_Shock"), TEXT("T_Crack"),
			TEXT("T_Circle1"), TEXT("T_Circle2"), TEXT("T_Circle3"), TEXT("T_Flame"), TEXT("T_Shard"), TEXT("T_Leaf"), TEXT("T_Bolt"), TEXT("T_Bubble"), TEXT("T_Drop"), TEXT("T_Spore"), TEXT("T_Gear"), TEXT("T_Swirl") };
		if (I < 0 || I >= UE_ARRAY_COUNT(N)) return nullptr;
		if (!T[I]) T[I] = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/DrawFight/FX/Tex/%s.%s"), N[I], N[I]));
		return T[I];
	}
	UMaterialInterface* ParticleMat()
	{
		static UMaterialInterface* M = nullptr;
		if (!M) M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DrawFight/FX/M_Particle.M_Particle"));
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
	const FLinearColor Ice(.55f, .85f, 1.f);
	const FLinearColor Blood(1.f, .08f, .12f);

	// skill -> archetype
	enum class EArch : uint8 { Burst, Quake, Projectile, Fan, Wave, Dash, Meteor, Sun, Bolt, Column, Shield, Haste, Vortex, Tornado, Cloud, Pause, Slash, Rain,
		Homing, Boomerang, Ricochet, SpikeLine, Mines, Laser, Breath, Blink, LeapSlam, Hook, Juggle, Freeze, Wall, Chill,
		Drain, Vanish, Warcry, Beast, Clone, Orbit, Turret, Transform, Charge, Mark, Parry, Combo };
	EArch ArchOf(FName S)
	{
		static const TMap<FName, EArch> Map = {
			{ "slam", EArch::Quake }, { "quake", EArch::Quake }, { "leap", EArch::LeapSlam }, { "blast", EArch::Burst }, { "spin", EArch::Burst },
			{ "rage", EArch::Burst }, { "counter", EArch::Parry }, { "eraser", EArch::Burst }, { "scribble", EArch::Fan },
			{ "throw", EArch::Fan }, { "volley", EArch::Rain }, { "inkspill", EArch::Projectile }, { "ruler", EArch::Slash }, { "pin", EArch::Fan },
			{ "clip", EArch::Projectile }, { "lash", EArch::Slash }, { "thrust", EArch::Dash }, { "charge", EArch::Dash }, { "shadow", EArch::Dash },
			{ "wave", EArch::Wave }, { "meteor", EArch::Meteor }, { "sun", EArch::Sun }, { "pageflip", EArch::Pause },
			{ "thunder", EArch::Bolt }, { "rift", EArch::Slash }, { "heal", EArch::Column }, { "barrier", EArch::Shield }, { "haste", EArch::Haste },
			{ "vortex", EArch::Vortex }, { "tornado", EArch::Tornado }, { "miasma", EArch::Cloud }, { "pause", EArch::Pause },
			{ "ult_demonlord", EArch::Transform }, { "ult_riftwalk", EArch::Slash }, { "ult_beastcall", EArch::Beast }, { "ult_shadowarmy", EArch::Clone },
			{ "ult_soulharvest", EArch::Vortex }, { "ult_timerewind", EArch::Pause }, { "ult_dragonform", EArch::Transform },
			{ "ult_thousandblades", EArch::Rain }, { "ult_livingcanvas", EArch::Fan },
			// full skill-pattern set
			{ "seeker", EArch::Homing }, { "boomerang", EArch::Boomerang }, { "ricochet", EArch::Ricochet }, { "spikeline", EArch::SpikeLine },
			{ "mines", EArch::Mines }, { "beam", EArch::Laser }, { "breath", EArch::Breath }, { "blink", EArch::Blink }, { "hook", EArch::Hook },
			{ "uppercut", EArch::Juggle }, { "frost", EArch::Freeze }, { "wall", EArch::Wall }, { "chill", EArch::Chill }, { "bloodpact", EArch::Drain },
			{ "vanish", EArch::Vanish }, { "warcry", EArch::Warcry }, { "wolf", EArch::Beast }, { "clone", EArch::Clone }, { "orbit", EArch::Orbit },
			{ "turret", EArch::Turret }, { "chargeshot", EArch::Charge }, { "mark", EArch::Mark }, { "combo", EArch::Combo } };
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

ADFSkillFX* ADFSkillFX::CastSkill(AActor* InCaster, FName Skill, int32 Level, FLinearColor Color, float Power, FName InElement)
{
	if (!InCaster || !InCaster->GetWorld()) return nullptr;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADFSkillFX* FX = InCaster->GetWorld()->SpawnActor<ADFSkillFX>(ADFSkillFX::StaticClass(), InCaster->GetActorLocation(), FRotator::ZeroRotator, SP);
	if (!FX) return nullptr;
	FX->Caster = InCaster;
	FX->Combat = InCaster->FindComponentByClass<UDFCombatComponent>();
	FX->Tint = Color;
	FX->Element = InElement;
	{
		const bool bUltCast = Skill.ToString().StartsWith(TEXT("ult_"));
		FX->CastCircle(FX->Ground(InCaster->GetActorLocation()) + FVector(0, 0, 4), bUltCast ? 7.f : 4.2f, bUltCast ? 1.6f : .9f, Color, bUltCast);
	}
	const float Base = FX->Combat.IsValid() ? FX->Combat->Damage : 12.f;
	FX->Build(Skill, Base * Power * (1.f + .25f * FMath::Max(0, Level - 1)));
	{
		const FVector L = InCaster->GetActorLocation();
		DFAudio::Sfx(FX, TEXT("S_Cast"), L, .55f, FMath::FRandRange(.9f, 1.1f));
		switch (ArchOf(Skill))
		{
		case EArch::Meteor: case EArch::Sun: case EArch::Breath: case EArch::Transform: case EArch::Laser: case EArch::Charge: DFAudio::Sfx(FX, TEXT("S_Fire"), L, .8f); break;
		case EArch::Bolt: case EArch::Ricochet: case EArch::Turret: DFAudio::Sfx(FX, TEXT("S_Zap"), L, .7f); break;
		case EArch::Freeze: case EArch::Chill: case EArch::SpikeLine: case EArch::Pause: DFAudio::Sfx(FX, TEXT("S_Ice"), L, .8f); break;
		case EArch::Column: case EArch::Shield: case EArch::Haste: case EArch::Drain: case EArch::Warcry: DFAudio::Sfx(FX, TEXT("S_Heal"), L, .7f); break;
		case EArch::Wave: case EArch::Slash: case EArch::Combo: case EArch::Boomerang: case EArch::Blink: case EArch::Dash: case EArch::Juggle: DFAudio::Sfx(FX, TEXT("S_Slash"), L, .9f, FMath::FRandRange(.9f, 1.15f)); break;
		default: break;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("DrawFight FX %s: %d pieces, %d hits, glow material %s"), *Skill.ToString(), FX->Pieces.Num(), FX->Hits.Num(), GlowMat() ? TEXT("ok") : TEXT("MISSING"));
	return FX;
}

void ADFSkillFX::Puff(AActor* Around, FVector Loc, FLinearColor Color, FName InElement, float Scale, bool bTrail)
{
	if (!Around || !Around->GetWorld()) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADFSkillFX* FX = Around->GetWorld()->SpawnActor<ADFSkillFX>(ADFSkillFX::StaticClass(), Loc, FRotator::ZeroRotator, SP);
	if (!FX) return;
	FX->Caster = Around; FX->Tint = Color; FX->Element = InElement; FX->Life = .4f;
	if (bTrail)
	{
		// afterimage streak behind a dash + element motes
		const FVector Back = -Around->GetActorForwardVector();
		for (int32 I = 0; I < 18; ++I)
		{
			FMote& M = FX->Mote(I % 3 ? FX->ElemTex() : ETex::Spark, Loc + FVector(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-60.f, 80.f)),
				Back * FMath::FRandRange(200.f, 700.f) + FVector(0, 0, FMath::FRandRange(0.f, 120.f)), FMath::FRandRange(0.f, .12f), FMath::FRandRange(.25f, .5f), .25f, .02f, Color, 4.f);
			M.Drag = 3.f; M.bStreak = I % 3 == 0;
		}
		FX->Sprite(ETex::SoftGlow, EFace::Camera, Loc, Loc + Back * 200.f, 3.f, 1.f, 0.f, .25f, Color, 2.f);
	}
	else
	{
		FX->ElementBurst(Loc, FMath::RoundToInt(22 * Scale), 700.f * Scale, 0.f, .8f, Color, .3f);
		FX->Streaks(Loc, FMath::RoundToInt(8 * Scale), 900.f * Scale, 0.f, .5f, Mix(Color, White, .3f), .05f);
		FX->Sprite(ETex::Star, EFace::Camera, Loc, Loc, .5f * Scale, 4.f * Scale, 0.f, .2f, Mix(Color, White, .3f), 8.f);
		FX->Smoke(Loc, FMath::RoundToInt(3 * Scale), .8f * Scale, 0.f, .9f, Color);
	}
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
	if (P.Mat) P.Mat->SetScalarParameterValue(TEXT("Glow"), Glow * 2.5f); // feed the bloom
	// soft painted sprites instead of solid balls (the look of the old web version)
	if (SpriteMat(false))
	{
		if (Shape == EShape::Sphere)
		{
			const float K = 2.4f;
			P.S0 = FVector(S0.X * K, S0.X * K, 1); P.S1 = FVector(S1.X * K, S1.X * K, 1);
			ToSprite(P, Glow >= 12.f ? ETex::Star : ETex::SoftGlow, EFace::Camera, false, C, Glow);
			P.Roll = FMath::FRandRange(0.f, 360.f);
		}
		else if (Shape == EShape::Cylinder && S0.Z <= .05f && S1.Z <= .05f)
		{
			P.S0 = FVector(S0.X * 1.8f, S0.Y * 1.8f, 1); P.S1 = FVector(S1.X * 1.8f, S1.Y * 1.8f, 1);
			ToSprite(P, ETex::SoftGlow, EFace::Flat, false, C, Glow);
		}
		else if (Shape == EShape::Cylinder && S0.X <= .2f)
		{
			// thin rods (bolts, arrows): a streak sprite along the rod
			P.Axis = Rot.RotateVector(FVector::UpVector);
			P.S0 = FVector(S0.Z * 1.3f, S0.X * 9.f, 1); P.S1 = FVector(S1.Z * 1.3f, S1.X * 9.f, 1);
			ToSprite(P, ETex::Spark, EFace::Axial, false, C, Glow);
		}
	}
	Pieces.Add(P);
	Keep.Add(M);
	Life = FMath::Max(Life, T1 + .1f);
}

void ADFSkillFX::ToSprite(FPiece& P, ETex Tex, EFace Face, bool bDust, FLinearColor C, float Glow)
{
	P.Face = Face;
	P.Mesh->SetStaticMesh(ShapeMesh(3));
	P.Mat = UMaterialInstanceDynamic::Create(SpriteMat(bDust), this);
	if (UTexture* T = SpriteTex((int32)Tex)) P.Mat->SetTextureParameterValue(TEXT("Tex"), T);
	P.Mat->SetVectorParameterValue(TEXT("Color"), C);
	P.Mat->SetScalarParameterValue(TEXT("Glow"), bDust ? Glow * .5f : Glow * .55f);
	P.Mat->SetScalarParameterValue(TEXT("Opacity"), 0.f);
	P.Mesh->SetMaterial(0, P.Mat);
}

void ADFSkillFX::Sprite(ETex Tex, EFace Face, FVector From, FVector To, float S0, float S1, float T0, float T1, FLinearColor C, float Glow, float Roll, float RollSpin, bool bDust, float Stretch)
{
	if (!SpriteMat(bDust)) { Piece(EShape::Sphere, From, To, FVector(S0 * .4f), FVector(S1 * .4f), T0, T1, C, Glow); return; }
	Piece(EShape::Cube, From, To, FVector(1), FVector(1), T0, T1, C, Glow);
	FPiece& P = Pieces.Last();
	ToSprite(P, Tex, Face, bDust, C, Glow);
	P.S0 = FVector(S0 * Stretch, S0, 1); P.S1 = FVector(S1 * Stretch, S1, 1);
	P.Roll = Roll; P.RollSpin = RollSpin;
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
	// flat expanding ring (texture ring sits at 78% of the sprite radius)
	const float K = 1.f / (50.f * .78f);
	Sprite(ETex::Ring, EFace::Flat, Center, Center, R0 * K, R1 * K, T0, T1, C, 6.f, FMath::FRandRange(0.f, 360.f));
	Sprite(ETex::Shock, EFace::Flat, Center + FVector(0, 0, 3), Center + FVector(0, 0, 3), R0 * K * .8f, R1 * K * 1.15f, T0 + .04f, T1 + .1f, Mix(C, White, .4f), 3.f, FMath::FRandRange(0.f, 360.f), 60.f);
	for (int32 I = 0; I < Count / 3; ++I)
	{
		const float A = 2.f * PI * I / FMath::Max(1, Count / 3) + FMath::FRandRange(-.2f, .2f);
		const FVector Dir(FMath::Cos(A), FMath::Sin(A), 0);
		Sprite(ETex::SoftGlow, EFace::Camera, Center + Dir * R0, Center + Dir * R1 + FVector(0, 0, FMath::FRandRange(0.f, 60.f)), Size * 1.2f, Size * .3f, T0, T1, C, 6.f);
	}
}

void ADFSkillFX::Sparks(FVector Center, int32 Count, float Dist, float T0, float Dur, FLinearColor C, float Size, bool bUp)
{
	Streaks(Center, Count * 2, Dist * 2.6f, T0, Dur + .2f, C, Size * .4f, bUp ? -1600.f : 0.f, bUp);
}

void ADFSkillFX::Streaks(FVector Center, int32 Count, float Speed, float T0, float Dur, FLinearColor C, float Width, float Grav, bool bUp)
{
	for (int32 I = 0; I < Count * 2; ++I)
	{
		FVector Dir = FMath::VRand();
		if (bUp) Dir.Z = FMath::Abs(Dir.Z) * 1.1f + .15f;
		FMote& M = Mote(ETex::Spark, Center, Dir.GetSafeNormal() * Speed * FMath::FRandRange(.35f, 1.f), T0 + FMath::FRandRange(0.f, .06f), Dur * FMath::FRandRange(.6f, 1.3f), Width * 2.f, Width * .4f, I % 3 ? C : Mix(C, White, .45f), 6.f);
		M.bStreak = true; M.Grav = Grav; M.Drag = 2.2f;
	}
}

void ADFSkillFX::Embers(FVector Center, int32 Count, float Spread, float T0, float Dur, FLinearColor C)
{
	for (int32 I = 0; I < Count * 2; ++I)
	{
		const FVector P0 = Center + FVector(FMath::FRandRange(-Spread, Spread), FMath::FRandRange(-Spread, Spread), FMath::FRandRange(0.f, 120.f));
		FMote& M = Mote(I % 3 ? ElemTex() : ETex::SoftGlow, P0, FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(120.f, 320.f)),
			T0 + FMath::FRandRange(0.f, .4f), Dur * FMath::FRandRange(.6f, 1.2f), FMath::FRandRange(.12f, .3f), .03f, Mix(C, White, .25f), 5.f);
		M.Grav = 40.f; M.Drag = .6f; M.Roll = FMath::FRandRange(0.f, 360.f); M.RollSpin = FMath::FRandRange(-120.f, 120.f);
	}
}

ADFSkillFX::FMote& ADFSkillFX::Mote(ETex Tex, FVector P, FVector V, float T0, float Dur, float S0, float S1, FLinearColor C, float Glow)
{
	const uint8 K = (uint8)Tex;
	UInstancedStaticMeshComponent* ISM = nullptr;
	if (TObjectPtr<UInstancedStaticMeshComponent>* Found = MoteISM.Find(K)) ISM = Found->Get();
	if (!ISM && ParticleMat())
	{
		ISM = NewObject<UInstancedStaticMeshComponent>(this);
		ISM->SetStaticMesh(ShapeMesh(3));
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCastShadow(false);
		ISM->NumCustomDataFloats = 4;
		ISM->SetupAttachment(RootComponent);
		ISM->SetUsingAbsoluteLocation(true); ISM->SetUsingAbsoluteRotation(true); ISM->SetUsingAbsoluteScale(true);
		ISM->SetWorldTransform(FTransform::Identity);
		ISM->RegisterComponent();
		UMaterialInstanceDynamic* MI = UMaterialInstanceDynamic::Create(ParticleMat(), this);
		if (UTexture* T = SpriteTex(K)) MI->SetTextureParameterValue(TEXT("Tex"), T);
		ISM->SetMaterial(0, MI);
		ISM->SetCullDistances(0, 0);
		MoteISM.Add(K, ISM);
	}
	FMote M;
	M.Tex = K; M.P = P; M.V = V; M.T0 = T0; M.T1 = T0 + Dur; M.S0 = S0; M.S1 = S1; M.C = C; M.Glow = Glow;
	M.Index = ISM ? ISM->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(.001f)), true) : -1;
	Life = FMath::Max(Life, M.T1 + .1f);
	return Motes.Add_GetRef(M);
}

void ADFSkillFX::TickMotes(float Dt)
{
	if (!Motes.Num()) return;
	TMap<uint8, TArray<FTransform>> Xf;
	for (TPair<uint8, TObjectPtr<UInstancedStaticMeshComponent>>& KV : MoteISM)
		if (KV.Value) Xf.Add(KV.Key).SetNum(KV.Value->GetInstanceCount());
	for (FMote& M : Motes)
	{
		TArray<FTransform>* Arr = Xf.Find(M.Tex);
		if (!Arr || !Arr->IsValidIndex(M.Index)) continue;
		const bool bOn = Age >= M.T0 && Age <= M.T1;
		if (!bOn) { (*Arr)[M.Index] = FTransform(FRotator::ZeroRotator, M.P, FVector(.001f)); continue; }
		const float A = FMath::Clamp((Age - M.T0) / FMath::Max(.01f, M.T1 - M.T0), 0.f, 1.f);
		M.V *= FMath::Max(0.f, 1.f - M.Drag * Dt);
		M.V.Z += M.Grav * Dt;
		M.P += M.V * Dt;
		if (M.Follow.IsValid()) M.LastFollow = M.Follow->GetActorLocation();
		const FVector Pos = M.P + M.LastFollow;
		FVector ToCam = (CamLoc - Pos).GetSafeNormal();
		if (ToCam.IsNearlyZero()) ToCam = FVector::UpVector;
		const float Sz = FMath::Lerp(M.S0, M.S1, A);
		FQuat Q; FVector S(Sz, Sz, 1.f);
		const float Speed = M.V.Size();
		if (M.bStreak && Speed > 1.f)
		{
			Q = FRotationMatrix::MakeFromXZ(M.V / Speed, ToCam).ToQuat();
			S.X = FMath::Clamp(Speed * .0016f, .12f, 2.4f);
		}
		else Q = FQuat(ToCam, FMath::DegreesToRadians(M.Roll + M.RollSpin * (Age - M.T0))) * FRotationMatrix::MakeFromZ(ToCam).ToQuat();
		(*Arr)[M.Index] = FTransform(Q, Pos, S);
		const float Op = FMath::Min(1.f, A / .08f) * (A > .55f ? (1.f - A) / .45f : 1.f);
		if (TObjectPtr<UInstancedStaticMeshComponent>* I = MoteISM.Find(M.Tex))
		{
			const float D[4] = { M.C.R, M.C.G, M.C.B, M.Glow * Op };
			(*I)->SetCustomData(M.Index, TArrayView<const float>(D, 4), false);
		}
	}
	for (TPair<uint8, TArray<FTransform>>& KV : Xf)
		if (TObjectPtr<UInstancedStaticMeshComponent>* I = MoteISM.Find(KV.Key))
			if (KV.Value.Num()) (*I)->BatchUpdateInstancesTransforms(0, KV.Value, true, true, true);
}

ADFSkillFX::ETex ADFSkillFX::ElemTex() const
{
	static const TMap<FName, ETex> Map = {
		{ "fire", ETex::Flame }, { "water", ETex::Drop }, { "earth", ETex::Shard }, { "wind", ETex::Leaf }, { "poison", ETex::Bubble },
		{ "spore", ETex::Spore }, { "light", ETex::Star }, { "dark", ETex::Flame }, { "fight", ETex::Spark }, { "psy", ETex::Swirl },
		{ "mech", ETex::Gear }, { "zombie", ETex::Bubble }, { "inf", ETex::Star } };
	const ETex* T = Map.Find(Element);
	return T ? *T : ETex::Star;
}

// a cloud of element-shaped particles (flames, shards, leaves, bubbles...)
void ADFSkillFX::ElementBurst(FVector Center, int32 Count, float Speed, float T0, float Dur, FLinearColor C, float Size, bool bUp)
{
	const ETex Tx = ElemTex();
	for (int32 I = 0; I < Count; ++I)
	{
		FVector Dir = FMath::VRand();
		if (bUp) Dir.Z = FMath::Abs(Dir.Z) * .9f + .2f;
		FMote& M = Mote(I % 4 ? Tx : ETex::SoftGlow, Center, Dir.GetSafeNormal() * Speed * FMath::FRandRange(.25f, 1.f), T0 + FMath::FRandRange(0.f, .08f),
			Dur * FMath::FRandRange(.6f, 1.25f), Size * FMath::FRandRange(.6f, 1.3f), Size * .15f, I % 5 ? C : Mix(C, White, .4f), 5.f);
		M.Grav = Tx == ETex::Flame || Tx == ETex::Bubble || Tx == ETex::Spore ? 150.f : -500.f;
		M.Drag = 2.f; M.Roll = FMath::FRandRange(0.f, 360.f); M.RollSpin = FMath::FRandRange(-260.f, 260.f);
	}
}

// spinning magic circles under the caster + element motes spiralling up
void ADFSkillFX::CastCircle(FVector Feet, float Size, float Dur, FLinearColor C, bool bUlt)
{
	const ETex Circ = (ETex)((int32)ETex::Circle1 + FMath::RandRange(0, 2));
	Sprite(Circ, EFace::Flat, Feet, Feet, Size * .3f, Size, 0.f, Dur, C, 7.f, FMath::FRandRange(0.f, 360.f), 110.f);
	Sprite(ETex::Ring, EFace::Flat, Feet + FVector(0, 0, 1), Feet + FVector(0, 0, 1), Size * .2f, Size * 1.25f, 0.f, Dur * .7f, Mix(C, White, .3f), 6.f);
	Sprite(ETex::SoftGlow, EFace::Flat, Feet, Feet, Size * .8f, Size * 1.8f, 0.f, Dur, C, 1.6f);
	if (bUlt)
	{
		Sprite((ETex)((int32)ETex::Circle1 + ((int32)Circ - (int32)ETex::Circle1 + 1) % 3), EFace::Flat, Feet + FVector(0, 0, 2), Feet + FVector(0, 0, 2), Size * .2f, Size * .6f, .1f, Dur, Mix(C, White, .4f), 8.f, 0.f, -160.f);
		Sprite(Circ, EFace::Camera, Feet + FVector(0, 0, 130), Feet + FVector(0, 0, 130), 1.f, 4.5f, .05f, Dur * .8f, C, 5.f, 0.f, 90.f);
	}
	const int32 N = bUlt ? 70 : 34;
	for (int32 I = 0; I < N; ++I)
	{
		const float A = 2.f * PI * I / N;
		const float R = Size * 45.f * FMath::FRandRange(.5f, 1.f);
		const FVector P0 = Feet + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 5.f);
		FMote& M = Mote(I % 3 ? ElemTex() : ETex::SoftGlow, P0, FVector(-FMath::Sin(A) * 120.f, FMath::Cos(A) * 120.f, FMath::FRandRange(180.f, 420.f)),
			FMath::FRandRange(0.f, Dur * .5f), FMath::FRandRange(.5f, .9f), FMath::FRandRange(.15f, .32f), .02f, I % 4 ? C : Mix(C, White, .4f), 5.f);
		M.Drag = .8f; M.Roll = FMath::FRandRange(0.f, 360.f); M.RollSpin = 200.f;
	}
}

void ADFSkillFX::Smoke(FVector Center, int32 Count, float Size, float T0, float Dur, FLinearColor C)
{
	for (int32 I = 0; I < Count; ++I)
	{
		const FVector D = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(.2f, .8f)) * Size * 60.f;
		const float T = T0 + FMath::FRandRange(0.f, .15f);
		const ETex Tx = (ETex)((int32)ETex::Smoke1 + I % 3);
		Sprite(Tx, EFace::Camera, Center + D * .3f, Center + D + FVector(0, 0, Size * 40.f), Size * 1.6f, Size * 4.f, T, T + Dur * FMath::FRandRange(.8f, 1.3f), Mix(C, FLinearColor(.16f, .14f, .13f), .8f), .9f, FMath::FRandRange(0.f, 360.f), FMath::FRandRange(-40.f, 40.f), true);
	}
}

// layered hit: white core, double shock ring, sparks with gravity, embers, smoke, scorch mark
void ADFSkillFX::Impact(FVector Center, float Scale, float T0, FLinearColor C, FLinearColor Hot)
{
	const FVector G = Ground(Center) + FVector(0, 0, 3);
	if (SfxPlayed < 8)
	{
		++SfxPlayed;
		FTimerHandle TH;
		TWeakObjectPtr<ADFSkillFX> W(this);
		const bool bBig = Scale >= .75f;
		const float Delay = FMath::Max(.01f, T0 - Age);
		GetWorldTimerManager().SetTimer(TH, [W, Center, bBig]() { if (W.IsValid()) DFAudio::Sfx(W.Get(), bBig ? TEXT("S_Boom") : TEXT("S_Hit"), Center, bBig ? .8f : .5f, FMath::FRandRange(.85f, 1.15f)); }, Delay, false);
	}
	Sprite(ETex::Star, EFace::Camera, Center, Center, 1.f * Scale, 7.f * Scale, T0, T0 + .16f, Mix(C, White, .35f), 12.f, FMath::FRandRange(0.f, 45.f));
	Sprite(ETex::SoftGlow, EFace::Camera, Center, Center, 1.5f * Scale, 6.5f * Scale, T0, T0 + .3f, C, 6.f);
	Sprite(ETex::Shock, EFace::Flat, G, G, 1.f * Scale, 13.f * Scale, T0, T0 + .45f, Mix(C, White, .5f), 5.f, FMath::FRandRange(0.f, 360.f));
	Sprite(ETex::Ring, EFace::Flat, G, G, 1.f * Scale, 9.f * Scale, T0 + .03f, T0 + .4f, C, 6.f);
	Sprite(ETex::Crack, EFace::Flat, G, G, 6.f * Scale, 6.5f * Scale, T0, T0 + 1.8f, Mix(C, Hot, .5f), 3.f, FMath::FRandRange(0.f, 360.f));
	Streaks(Center, FMath::RoundToInt(26 * Scale), 1500.f * Scale, T0, .7f, Hot);
	ElementBurst(Center, FMath::RoundToInt(36 * Scale), 900.f * Scale, T0, .9f, C, .3f + .1f * Scale);
	if (Scale >= .7f) Sprite((ETex)((int32)ETex::Circle1 + FMath::RandRange(0, 2)), EFace::Flat, G + FVector(0, 0, 2), G + FVector(0, 0, 2), 2.f * Scale, 6.f * Scale, T0, T0 + .7f, C, 5.f, FMath::FRandRange(0.f, 360.f), 140.f);
	Embers(G, FMath::RoundToInt(14 * Scale), 220.f * Scale, T0 + .1f, 1.4f, C);
	Smoke(G + FVector(0, 0, 40), FMath::RoundToInt(5 * Scale), .9f * Scale, T0 + .05f, 1.f, C);
}

ADFSkillFX::FHit& ADFSkillFX::Hit(float At, FVector Center, float Radius, float Damage, float Stun, float Pull, float HealFrac)
{
	FHit H;
	H.At = At; H.Center = Center; H.Radius = Radius; H.Damage = Damage; H.Stun = Stun; H.Pull = Pull; H.HealFrac = HealFrac;
	Life = FMath::Max(Life, At + .1f);
	return Hits.Add_GetRef(H);
}

void ADFSkillFX::Flash(FVector At, float Peak, float T0, float T1, FVector To)
{
	LightFrom = At;
	LightTo = To.IsZero() ? At : To;
	LightPeak = Peak * 4.f; LightT0 = T0; LightT1 = T1;
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

TArray<AActor*> ADFSkillFX::TargetActors(int32 Max, float MaxDist) const
{
	TArray<TPair<float, AActor*>> L;
	const FVector Me = Caster->GetActorLocation();
	const int32 Team = Combat.IsValid() ? Combat->Team : 0;
	for (UDFCombatComponent* O : UDFCombatComponent::Registry())
		if (O && O->Team != Team && !O->IsDead() && O->GetOwner())
		{
			const float D = FVector::Dist(O->GetOwner()->GetActorLocation(), Me);
			if (D < MaxDist) L.Add({ D, O->GetOwner() });
		}
	L.Sort([](const TPair<float, AActor*>& A, const TPair<float, AActor*>& B) { return A.Key < B.Key; });
	TArray<AActor*> Out;
	for (int32 I = 0; I < L.Num() && I < Max; ++I) Out.Add(L[I].Value);
	return Out;
}

// a held beam that follows the caster: layered streak sprites + flicker + ticking hits along its length
void ADFSkillFX::Beam(FVector Start, FVector Dir, float Len, float T0, float Dur, FLinearColor C, FLinearColor Hot, float Width, float Dmg)
{
	AActor* Ca = Caster.Get();
	const FVector Off0 = Start - Ca->GetActorLocation();
	const FVector Mid = Off0 + Dir * Len * .5f;
	const float L = Len / 100.f;
	struct FLayer { float W; FLinearColor Col; float Glow; };
	const FLayer BeamLayers[] = { { Width * 3.f, C, 4.f }, { Width * 1.6f, Hot, 7.f }, { Width * .6f, White, 12.f } };
	for (const FLayer& Ly : BeamLayers)
	{
		Sprite(ETex::Spark, EFace::Axial, Mid, Mid, Ly.W, Ly.W, T0, T0 + Dur, Ly.Col, Ly.Glow, 0.f, 0.f, false, 1.f);
		FPiece& P = Last();
		P.S0 = FVector(L * 1.15f, Ly.W, 1); P.S1 = FVector(L * 1.15f, Ly.W * .7f, 1);
		P.Axis = Dir; P.Follow = Caster; P.bFlicker = true;
	}
	// muzzle + end glow
	Sprite(ETex::Star, EFace::Camera, Off0, Off0, Width * 6.f, Width * 7.f, T0, T0 + Dur, Hot, 10.f, 0.f, 220.f);
	Last().Follow = Caster; Last().bFlicker = true;
	const FVector EndOff = Off0 + Dir * Len;
	Sprite(ETex::SoftGlow, EFace::Camera, EndOff, EndOff, Width * 8.f, Width * 9.f, T0, T0 + Dur, C, 8.f);
	Last().Follow = Caster; Last().bFlicker = true;
	const int32 Ticks = FMath::Max(1, FMath::RoundToInt(Dur / .15f));
	for (int32 K = 0; K < Ticks; ++K)
		for (int32 S = 1; S <= 5; ++S)
		{
			FHit& H = Hit(T0 + K * .15f, FVector::ZeroVector, 120.f + Width * 60.f, Dmg / (Ticks * 1.5f));
			H.bAtCaster = true; H.Offset = Off0 + Dir * Len * S / 5.f;
			if (S == 5 && K % 2 == 0) H.FxScale = .45f;
		}
}

// glowing crystal spike that bursts out of the ground (cone mesh)
void ADFSkillFX::Crystal(FVector Base, float Height, float T0, float Dur, FLinearColor C, float Tilt)
{
	const FRotator R(FMath::FRandRange(-Tilt, Tilt), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(-Tilt, Tilt));
	const float H = Height / 100.f, W = Height / 260.f;
	Piece(EShape::Cone, Base - FVector(0, 0, Height * .5f), Base + FVector(0, 0, Height * .45f), FVector(W, W, H * .3f), FVector(W, W, H), T0, T0 + Dur, C, 3.f, R);
	Piece(EShape::Cone, Base - FVector(0, 0, Height * .5f), Base + FVector(0, 0, Height * .45f), FVector(W * .45f, W * .45f, H * .3f), FVector(W * .45f, W * .45f, H * 1.05f), T0, T0 + Dur, Mix(C, White, .6f), 9.f, R);
}

// spectral copy of the caster that fights on our side for a while
void ADFSkillFX::SpawnAlly(FVector At, float AllyLife, float DmgMul)
{
	ACharacter* Me = Cast<ACharacter>(Caster.Get());
	if (!Me) return;
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ADFEnemyCharacter* A = GetWorld()->SpawnActor<ADFEnemyCharacter>(ADFEnemyCharacter::StaticClass(), At, Me->GetActorRotation(), SP);
	if (!A) return;
	if (Me->GetMesh() && Me->GetMesh()->GetSkeletalMeshAsset()) A->GetMesh()->SetSkeletalMesh(Me->GetMesh()->GetSkeletalMeshAsset());
	if (UMaterialInterface* G = GlowMat())
	{
		UMaterialInstanceDynamic* MI = UMaterialInstanceDynamic::Create(G, A);
		MI->SetVectorParameterValue(TEXT("Color"), Mix(Tint, Void, .5f));
		MI->SetScalarParameterValue(TEXT("Glow"), 2.5f);
		MI->SetScalarParameterValue(TEXT("Opacity"), .8f);
		A->GetMesh()->SetOverlayMaterial(MI);
	}
	if (UDFCombatComponent* AC = A->FindComponentByClass<UDFCombatComponent>())
	{
		AC->Team = Combat.IsValid() ? Combat->Team : 0;
		const float Base = Combat.IsValid() ? Combat->Damage : 12.f;
		AC->Damage = Base * DmgMul;
		AC->MaxHP = Combat.IsValid() ? Combat->MaxHP * .5f : 60.f;
		AC->HP = AC->MaxHP;
	}
	A->GetCharacterMovement()->MaxWalkSpeed = 620.f;
	A->SetLifeSpan(AllyLife);
	Smoke(At, 6, 1.f, 0.f, .9f, Void);
	Sprite(ETex::Star, EFace::Camera, At + FVector(0, 0, 60), At + FVector(0, 0, 60), 1.f, 7.f, 0.f, .3f, Mix(Tint, White, .5f), 10.f);
}

// ---------- archetypes ----------
void ADFSkillFX::Build(FName Skill, float Dmg)
{
	AActor* C = Caster.Get();
	const FVector Me = C->GetActorLocation();
	const FVector Feet = Ground(Me);
	FVector Fwd = TargetPoint(2500.f) - Me; Fwd.Z = 0; Fwd = Fwd.GetSafeNormal();
	if (Fwd.IsNearlyZero()) Fwd = C->GetActorForwardVector();
	const FLinearColor T = Tint, Hot = Mix(Tint, White, .22f);
	const bool bUlt = Skill.ToString().StartsWith(TEXT("ult_"));
	const float Big = bUlt ? 1.8f : 1.f;
	ShakeOnHit = bUlt ? 28.f : 10.f;
	StopOnHit = bUlt ? .14f : .05f;

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
		Impact(Feet + FVector(0, 0, 40), 1.4f * Big, 0.f, T, Hot);
		Flash(Feet + FVector(0, 0, 100), 60.f, 0.f, .45f);
		Hit(.05f, Feet, 520.f * Big, Dmg * 2.2f, .4f);
		break;
	}
	case EArch::Burst: // nova around self
	{
		Piece(EShape::Sphere, Me, Me, FVector(.3f), FVector(7.f) * Big, 0.f, .4f, Hot, 8.f);
		Ring(Me, 50, 600 * Big, 0.f, .45f, T, 28, .35f);
		Impact(Me, 1.2f * Big, 0.f, T, Hot);
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
			Impact(To, bRain ? .35f : .5f, T0 + Fly, T, Hot);
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
			const float T0 = .1f * W;
			// painted crescent flying forward (texture arc opens toward -Y, so roll it to face Dir)
			const float Yaw = Dir.Rotation().Yaw + 90.f;
			for (int32 L = 0; L < 3; ++L)
			{
				const FVector P0 = Me + FVector(0, 0, 50 + L * 8) - Dir * 40.f * L;
				Sprite(ETex::Slash, EFace::Flat, P0, P0 + Dir * 1300, 5.f * Big, 7.f * Big, T0 + .03f * L, T0 + .55f + .03f * L, L == 0 ? White : L == 1 ? Hot : T, L == 0 ? 8.f : 5.f, Yaw);
			}
			for (int32 K = 1; K <= 6; ++K) Hit(T0 + .55f * K / 6.f, Me + Dir * 1300 * K / 6.f, 260.f, Dmg * .5f);
			// trailing sparks along the slash path + burst where it ends
			for (int32 K = 1; K <= 5; ++K) Streaks(Me + Dir * 260.f * K + FVector(0, 0, 40), 6, 700.f, T0 + .11f * K, .4f, T, .04f);
			Impact(Me + Dir * 1300 + FVector(0, 0, 40), .7f * Big, T0 + .55f, T, Hot);
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
		Impact(End, .8f, .2f, T, Hot);
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
			Impact(Land + FVector(0, 0, 60), Sz / 1.4f, Ti, Fire, Gold);
			for (int32 K = 0; K < 10; ++K) Streaks(FMath::Lerp(Sky, Land, K / 10.f), 3, 300.f, T0 + Fall * K / 10.f, .5f, Fire, .05f, 200.f, false);
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
			Impact(G + FVector(0, 0, 30), .8f * Big, T0, Electric, White);
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
	// ===================== extended patterns =====================
	case EArch::Homing: // orbs that curve out and chase their targets
	{
		TArray<AActor*> Ts = TargetActors(bUlt ? 10 : 5, 2600.f);
		const int32 N = bUlt ? 10 : 5;
		for (int32 I = 0; I < N; ++I)
		{
			AActor* Tg = Ts.Num() ? Ts[I % Ts.Num()] : nullptr;
			const FVector Side = FVector::CrossProduct(Fwd, FVector::UpVector) * (I % 2 ? 1.f : -1.f);
			const FVector From = Me + FVector(0, 0, 60);
			const FVector To = Tg ? Tg->GetActorLocation() : Me + Fwd * 1400.f;
			const float T0 = .07f * I, Fly = .75f;
			for (int32 L = 0; L < 2; ++L)
			{
				Sprite(L ? ETex::Star : ETex::SoftGlow, EFace::Camera, From, To, L ? .9f : 1.6f, L ? .7f : 1.2f, T0, T0 + Fly, L ? White : T, L ? 10.f : 6.f, 0.f, 400.f);
				FPiece& P = Last();
				P.bCurve = true; P.Ctrl = Me + Side * FMath::FRandRange(350.f, 650.f) + FVector(0, 0, FMath::FRandRange(250.f, 600.f)) - Fwd * 150.f;
				P.Chase = Tg;
			}
			// trail
			for (int32 K = 1; K <= 4; ++K)
			{
				Sprite(ETex::SoftGlow, EFace::Camera, From, To, 1.1f - K * .2f, .2f, T0 + K * .03f, T0 + Fly + K * .03f, T, 4.f);
				FPiece& P = Last(); P.bCurve = true; P.Ctrl = Pieces[Pieces.Num() - 1 - K].Ctrl; P.Chase = Tg;
			}
			if (Tg) { FHit& H = Hit(T0 + Fly, FVector::ZeroVector, 160.f, Dmg * (bUlt ? .9f : 1.f)); H.Target = Tg; H.FxScale = .45f; }
			else Hit(T0 + Fly, To, 160.f, Dmg).FxScale = .45f;
		}
		Flash(Me, 40.f, 0.f, .9f);
		break;
	}
	case EArch::Boomerang: // spinning crescent that flies out and comes back
	{
		const FVector Side = FVector::CrossProduct(Fwd, FVector::UpVector);
		const FVector A = Me + FVector(0, 0, 60), Far = Me + Fwd * 1150.f + FVector(0, 0, 60);
		for (int32 W = 0; W < (bUlt ? 3 : 1); ++W)
		{
			const FVector Turn = Side * (W - (bUlt ? 1 : 0)) * 300.f;
			for (int32 Leg = 0; Leg < 2; ++Leg)
			{
				const float T0 = .1f * W + Leg * .55f;
				for (int32 L = 0; L < 2; ++L)
				{
					Sprite(ETex::Slash, EFace::Camera, Leg ? Far + Turn : A, Leg ? A : Far + Turn, L ? 2.4f : 3.2f, L ? 2.4f : 3.2f, T0, T0 + .55f, L ? White : T, L ? 9.f : 6.f, 0.f, 1100.f);
					FPiece& P = Last(); P.bCurve = true; P.Ctrl = Me + Fwd * 650.f + Side * (Leg ? -380.f : 380.f) + FVector(0, 0, 60);
					if (Leg) P.Chase = Caster;
				}
				for (int32 K = 1; K <= 4; ++K)
				{
					const float U = K / 4.f;
					const FVector S0 = Leg ? Far + Turn : A, S1 = Leg ? A : Far + Turn;
					const FVector Ct = Me + Fwd * 650.f + Side * (Leg ? -380.f : 380.f);
					const FVector Pt = FMath::Lerp(FMath::Lerp(S0, Ct, U), FMath::Lerp(Ct, S1, U), U);
					Hit(T0 + .55f * U, Pt, 190.f, Dmg * .45f);
					Streaks(Pt, 3, 500.f, T0 + .55f * U, .3f, T, .04f);
				}
			}
		}
		break;
	}
	case EArch::Ricochet: // one bullet bouncing from enemy to enemy
	{
		TArray<AActor*> Ts = TargetActors(bUlt ? 8 : 5, 2400.f);
		FVector Prev = Me + FVector(0, 0, 60);
		float T0 = 0.f;
		if (!Ts.Num()) { Hit(.3f, Me + Fwd * 900.f, 150.f, Dmg).FxScale = .5f; Sprite(ETex::Star, EFace::Camera, Prev, Me + Fwd * 900.f, 1.f, 1.f, 0.f, .3f, Hot, 10.f, 0.f, 600.f); }
		for (int32 I = 0; I < Ts.Num(); ++I)
		{
			const FVector P1 = Ts[I]->GetActorLocation();
			const float Fly = FMath::Clamp(FVector::Dist(Prev, P1) / 3200.f, .08f, .35f);
			Sprite(ETex::Star, EFace::Camera, Prev, P1, 1.1f, 1.1f, T0, T0 + Fly, White, 11.f, 0.f, 700.f);
			Sprite(ETex::Spark, EFace::Axial, Prev, P1, 2.f, 1.f, T0, T0 + Fly + .08f, T, 7.f);
			Last().Axis = (P1 - Prev).GetSafeNormal(); Last().S0 = FVector(3.f, .8f, 1); Last().S1 = FVector(3.f, .5f, 1);
			FHit& H = Hit(T0 + Fly, P1, 150.f, Dmg * (1.f + .15f * I)); H.Target = Ts[I]; H.FxScale = .5f;
			Prev = P1; T0 += Fly + .03f;
		}
		break;
	}
	case EArch::SpikeLine: // crystals erupt in a line, launching enemies
	{
		const int32 N = bUlt ? 16 : 10;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector P0 = Ground(Me + Fwd * (150.f + I * 110.f) + FVector(FMath::FRandRange(-60.f, 60.f), FMath::FRandRange(-60.f, 60.f), 0));
			const float T0 = .045f * I;
			for (int32 K = 0; K < 3; ++K)
				Crystal(P0 + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 0), FMath::FRandRange(140.f, 260.f) * Big * (K ? .6f : 1.f), T0, .9f, Mix(T, Ice, .3f), 18.f);
			Smoke(P0, 2, .8f, T0, .7f, T);
			Streaks(P0, 5, 700.f, T0, .4f, Hot, .04f);
			FHit& H = Hit(T0 + .02f, P0, 150.f, Dmg * .35f); H.Lift = 750.f;
		}
		Sprite(ETex::Crack, EFace::Flat, Ground(Me + Fwd * 650.f), Ground(Me + Fwd * 650.f), 7.f, 9.f, 0.f, 1.4f, T, 3.f, Fwd.Rotation().Yaw, 0.f, false, 1.8f);
		Flash(Me + Fwd * 500.f, 40.f, 0.f, .7f, Me + Fwd * 1200.f);
		break;
	}
	case EArch::Mines: // drop blinking mines that blow up in sequence
	{
		const FVector Tgt = TargetPoint(1600.f);
		const int32 N = bUlt ? 7 : 4;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector P0 = Ground(Tgt + FVector(FMath::FRandRange(-320.f, 320.f), FMath::FRandRange(-320.f, 320.f), 0)) + FVector(0, 0, 8);
			const float Arm = .9f + .18f * I;
			Sprite(ETex::SoftGlow, EFace::Camera, Me + FVector(0, 0, 60), P0, .8f, .8f, 0.f, .35f, Hot, 8.f);
			Last().bCurve = true; Last().Ctrl = (Me + P0) * .5f + FVector(0, 0, 400);
			Sprite(ETex::Ring, EFace::Flat, P0, P0, 1.6f, 1.2f, .35f, Arm, FLinearColor(1.f, .15f, .1f), 6.f, 0.f, 180.f);
			Last().bFlicker = true;
			Sprite(ETex::Star, EFace::Camera, P0 + FVector(0, 0, 20), P0 + FVector(0, 0, 20), .8f, .8f, .35f, Arm, FLinearColor(1.f, .2f, .1f), 9.f);
			Last().bFlicker = true;
			FHit& H = Hit(Arm, P0, 260.f, Dmg * 1.1f); H.FxScale = .8f; H.Lift = 350.f;
		}
		break;
	}
	case EArch::Laser:
	{
		Sprite(ETex::SoftGlow, EFace::Camera, FVector(0, 0, 60), FVector(0, 0, 60), 1.f, 4.f, 0.f, .35f, Hot, 8.f);
		Last().Follow = Caster;
		Beam(Me + Fwd * 60.f + FVector(0, 0, 40), Fwd, bUlt ? 2600.f : 1800.f, .3f, bUlt ? 2.4f : 1.6f, T, Hot, bUlt ? 1.6f : 1.f, Dmg * 4.f);
		if (Combat.IsValid()) Combat->Stun(.3f + (bUlt ? 2.4f : 1.6f) * .6f); // rooted while firing
		Flash(Me + Fwd * 400.f, 80.f, .3f, .3f + (bUlt ? 2.4f : 1.6f));
		break;
	}
	case EArch::Breath: // cone of fire / frost streaming forward
	{
		const float Dur = bUlt ? 2.2f : 1.4f, Len = bUlt ? 1100.f : 800.f;
		const FLinearColor Col = Mix(T, Fire, .4f);
		for (float t = 0.f; t < Dur; t += .045f)
		{
			const FVector D = Fwd.RotateAngleAxis(FMath::FRandRange(-22.f, 22.f), FVector::UpVector);
			const FVector O = Fwd * 80.f + FVector(0, 0, 50);
			Sprite(ETex::SoftGlow, EFace::Camera, O, O + D * Len + FVector(0, 0, FMath::FRandRange(-40.f, 80.f)), .8f, 5.5f * Big, t, t + .5f, FMath::RandBool() ? Col : Hot, 5.f, FMath::FRandRange(0.f, 360.f));
			Last().Follow = Caster;
			if (FMath::RandBool())
			{
				Sprite((ETex)((int32)ETex::Smoke1 + FMath::RandRange(0, 2)), EFace::Camera, O, O + D * Len * 1.1f + FVector(0, 0, 120), 1.f, 6.f * Big, t + .1f, t + .8f, Mix(Col, FLinearColor(.4f, .35f, .3f), .5f), .8f, FMath::FRandRange(0.f, 360.f), 90.f, true);
				Last().Follow = Caster;
			}
		}
		for (float t = .1f; t < Dur; t += .2f)
			for (int32 S = 1; S <= 3; ++S) { FHit& H = Hit(t, FVector::ZeroVector, 110.f + S * 70.f, Dmg * .22f); H.bAtCaster = true; H.Offset = Fwd * Len * S / 3.5f; }
		if (Combat.IsValid()) Combat->AddBuff(EDFBuff::Speed, .45f, Dur);
		Flash(Me + Fwd * 300.f, 60.f, 0.f, Dur);
		break;
	}
	case EArch::Blink: // vanish and reappear behind the target with a cut
	{
		AActor* Tg = TargetActors(1, 1800.f).Num() ? TargetActors(1, 1800.f)[0] : nullptr;
		const FVector Dest = Tg ? Tg->GetActorLocation() - (Tg->GetActorLocation() - Me).GetSafeNormal2D() * -180.f : Me + Fwd * 700.f;
		Smoke(Me, 6, 1.f, 0.f, .8f, Void);
		Sprite(ETex::Star, EFace::Camera, Me, Me, 1.f, 6.f, 0.f, .25f, Mix(T, Void, .4f), 10.f);
		Sprite(ETex::Spark, EFace::Axial, (Me + Dest) * .5f, (Me + Dest) * .5f, 1.f, .4f, 0.f, .25f, Hot, 10.f);
		Last().Axis = (Dest - Me).GetSafeNormal(); Last().S0 = FVector(FVector::Dist(Me, Dest) / 100.f, 1.f, 1); Last().S1 = FVector(FVector::Dist(Me, Dest) / 100.f, .2f, 1);
		C->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
		if (Tg) C->SetActorRotation((Tg->GetActorLocation() - Dest).GetSafeNormal2D().Rotation());
		if (Combat.IsValid()) Combat->SetInvulnerable(.4f);
		const float Yaw = (Tg ? (Tg->GetActorLocation() - Dest) : Fwd).Rotation().Yaw + 90.f;
		Sprite(ETex::Slash, EFace::Flat, Dest + FVector(0, 0, 70), Dest + FVector(0, 0, 70), 4.f, 6.f, .08f, .35f, White, 9.f, Yaw);
		Sprite(ETex::Slash, EFace::Flat, Dest + FVector(0, 0, 60), Dest + FVector(0, 0, 60), 5.f, 7.f, .1f, .4f, T, 6.f, Yaw + 25.f);
		FHit& H = Hit(.12f, Dest, 260.f, Dmg * 2.2f, .5f); H.FxScale = .7f;
		break;
	}
	case EArch::LeapSlam: // jump in an arc and smash down where we land
	{
		const FVector Tgt = TargetPoint(1500.f);
		FVector V = Tgt - Me; V.Z = 0;
		const float Air = .7f;
		if (ACharacter* Ch = Cast<ACharacter>(C)) Ch->LaunchCharacter(V / Air + FVector(0, 0, 980.f * Air * .5f + 300.f), true, true);
		Streaks(Me, 10, 500.f, 0.f, .4f, T, .04f);
		Sprite(ETex::Ring, EFace::Flat, Ground(Tgt), Ground(Tgt), 6.f * Big, 5.f * Big, 0.f, Air, FLinearColor(1, .2f, .1f), 4.f, 0.f, 120.f);
		FHit& H = Hit(Air, FVector::ZeroVector, 480.f * Big, Dmg * 2.2f, .5f); H.bAtCaster = true; H.Offset = FVector(0, 0, -60); H.FxScale = 1.3f * Big; H.Lift = 600.f;
		FHit& R = Hit(Air + .05f, FVector::ZeroVector, 600.f * Big, 0.f); R.bAtCaster = true; R.Push = 700.f;
		Flash(Tgt + FVector(0, 0, 150), 70.f, Air - .05f, Air + .4f);
		break;
	}
	case EArch::Hook: // throw a chain, yank the target in
	{
		AActor* Tg = TargetActors(1, 1600.f).Num() ? TargetActors(1, 1600.f)[0] : nullptr;
		const FVector End = Tg ? Tg->GetActorLocation() : Me + Fwd * 1200.f;
		for (int32 K = 0; K < 14; ++K)
		{
			const float U = (K + 1) / 14.f;
			const FVector P0 = FMath::Lerp(Me, End, U) + FVector(0, 0, 50);
			Sprite(K == 13 ? ETex::Star : ETex::SoftGlow, EFace::Camera, Me + FVector(0, 0, 50), P0, .3f, K == 13 ? 1.5f : .5f, 0.f, .25f + .3f, K == 13 ? White : T, 8.f);
		}
		if (Tg)
		{
			FHit& H = Hit(.25f, FVector::ZeroVector, 120.f, Dmg * 1.2f, .8f, 1700.f); H.Target = Tg; H.FxScale = .5f; H.bPullToCaster = true;
		}
		break;
	}
	case EArch::Juggle: // rising slash launches enemies, then air hits
	{
		const FVector Front = Me + Fwd * 180.f;
		Sprite(ETex::Slash, EFace::Camera, Front, Front + FVector(0, 0, 250), 4.f * Big, 6.f * Big, 0.f, .3f, White, 9.f, 180.f);
		Sprite(ETex::Slash, EFace::Camera, Front, Front + FVector(0, 0, 300), 5.f * Big, 7.f * Big, .02f, .35f, T, 6.f, 180.f);
		Streaks(Front, 14, 900.f, 0.f, .4f, Hot, .04f, 0.f, true);
		FHit& H = Hit(.05f, Front, 260.f * Big, Dmg * .9f, .9f); H.Lift = 1100.f;
		for (AActor* Tg : TargetActors(bUlt ? 6 : 3, 420.f * Big))
			for (int32 K = 0; K < 3; ++K)
			{
				FHit& A = Hit(.45f + .16f * K, FVector::ZeroVector, 100.f, Dmg * .4f, .4f); A.Target = Tg; A.FxScale = .35f; A.Lift = 250.f;
			}
		if (ACharacter* Ch = Cast<ACharacter>(C)) Ch->LaunchCharacter(FVector(0, 0, 700.f), false, true);
		break;
	}
	case EArch::Freeze: // ice nova: freezes enemies inside crystals
	{
		Ring(Feet + FVector(0, 0, 10), 60.f, 700.f * Big, 0.f, .5f, Ice, 18, .3f);
		Sprite(ETex::SoftGlow, EFace::Camera, Me, Me, 2.f, 14.f * Big, 0.f, .4f, Mix(Ice, White, .5f), 6.f);
		Streaks(Me, 24, 1300.f, 0.f, .6f, Ice, .05f, -900.f);
		for (AActor* Tg : TargetActors(12, 720.f * Big))
		{
			const FVector G = Ground(Tg->GetActorLocation());
			for (int32 K = 0; K < 5; ++K) Crystal(G + FVector(FMath::FRandRange(-50.f, 50.f), FMath::FRandRange(-50.f, 50.f), 0), FMath::FRandRange(120.f, 220.f), .1f, bUlt ? 3.f : 2.2f, Ice, 25.f);
			FHit& H = Hit(.12f, FVector::ZeroVector, 80.f, Dmg * 1.2f, bUlt ? 3.f : 2.2f); H.Target = Tg; H.SlowMul = .5f; H.SlowDur = 4.f;
		}
		Flash(Me, 80.f, 0.f, .5f);
		break;
	}
	case EArch::Wall: // stone wall rises across the path, blocks and shoves
	{
		const FVector Side = FVector::CrossProduct(Fwd, FVector::UpVector);
		const FVector Mid = Ground(Me + Fwd * 380.f);
		const int32 N = bUlt ? 9 : 6;
		const float Dur = bUlt ? 6.f : 4.5f;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector P0 = Mid + Side * (I - (N - 1) * .5f) * 110.f;
			const float H = FMath::FRandRange(2.2f, 3.2f);
			Piece(EShape::Cube, P0 - FVector(0, 0, 160), P0 + FVector(0, 0, H * 50.f - 10.f), FVector(1.f, 1.1f, H), FVector(1.f, 1.1f, H), .03f * I, Dur, Mix(T, Gold, .3f), 1.6f, FRotator(FMath::FRandRange(-6.f, 6.f), Fwd.Rotation().Yaw, FMath::FRandRange(-6.f, 6.f)));
			Last().bEaseOut = false;
			Last().Mesh->SetCollisionProfileName(TEXT("BlockAll"));
			Last().Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Smoke(P0, 2, 1.f, .03f * I, .9f, T);
		}
		Sprite(ETex::Crack, EFace::Flat, Mid, Mid, 6.f, 8.f, 0.f, Dur, T, 2.f, Fwd.Rotation().Yaw + 90.f, 0.f, false, 2.f);
		FHit& H = Hit(.1f, Mid, 380.f, Dmg * 1.4f, .6f); H.Push = 900.f; H.Lift = 300.f;
		Flash(Mid + FVector(0, 0, 100), 50.f, 0.f, .5f);
		break;
	}
	case EArch::Chill: // frost field: slows and nibbles
	{
		const FVector Tgt = Ground(TargetPoint(1400.f)) + FVector(0, 0, 4);
		const float Dur = bUlt ? 6.f : 4.5f, R = 420.f * Big;
		Sprite(ETex::Shock, EFace::Flat, Tgt, Tgt, R / 40.f, R / 38.f, 0.f, Dur, Ice, 3.f, 0.f, 25.f);
		Sprite(ETex::SoftGlow, EFace::Flat, Tgt, Tgt, R / 30.f, R / 28.f, 0.f, Dur, Mix(Ice, T, .3f), 1.5f);
		Sprite(ETex::Crack, EFace::Flat, Tgt, Tgt, R / 45.f, R / 45.f, 0.f, Dur, Ice, 2.f, FMath::FRandRange(0.f, 360.f));
		for (int32 I = 0; I < 30; ++I)
		{
			const FVector P0 = Tgt + FVector(FMath::FRandRange(-R, R), FMath::FRandRange(-R, R), FMath::FRandRange(20.f, 200.f)) * .8f;
			const float T0 = FMath::FRandRange(0.f, Dur - .8f);
			Sprite(ETex::Star, EFace::Camera, P0, P0 - FVector(0, 0, 60), .5f, .2f, T0, T0 + .8f, Mix(Ice, White, .5f), 8.f, 0.f, 90.f);
		}
		for (float t = .2f; t < Dur; t += .5f) { FHit& H = Hit(t, Tgt, R, Dmg * .18f); H.SlowMul = .4f; H.SlowDur = .7f; }
		break;
	}
	case EArch::Drain: // blood streams from enemies into us
	{
		const TArray<AActor*> Ts = TargetActors(bUlt ? 10 : 6, 900.f * Big);
		Sprite(ETex::Ring, EFace::Flat, Feet + FVector(0, 0, 5), Feet + FVector(0, 0, 5), 14.f * Big, 3.f, 0.f, .9f, Blood, 5.f, 0.f, -90.f);
		for (int32 I = 0; I < Ts.Num(); ++I)
		{
			FHit& H = Hit(.15f, FVector::ZeroVector, 90.f, Dmg * 1.3f); H.Target = Ts[I]; H.Drain = .6f; H.FxScale = .3f;
			const FVector From = Ts[I]->GetActorLocation();
			for (int32 K = 0; K < 7; ++K)
			{
				const float T0 = .15f + K * .06f;
				Sprite(ETex::SoftGlow, EFace::Camera, From, FVector(0, 0, 50), .9f, .4f, T0, T0 + .5f, K % 2 ? Blood : Mix(Blood, White, .3f), 7.f);
				FPiece& P = Last(); P.bCurve = true; P.Ctrl = (From + Me) * .5f + FVector(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f), 250.f); P.Chase = Caster;
			}
		}
		Sprite(ETex::SoftGlow, EFace::Camera, FVector(0, 0, 50), FVector(0, 0, 50), 2.f, 5.f, .5f, 1.1f, Blood, 6.f);
		Last().Follow = Caster;
		break;
	}
	case EArch::Vanish: // smoke bomb: disappear, become untouchable, next hits stronger
	{
		const float Dur = bUlt ? 4.f : 2.8f;
		Smoke(Me, 12, 1.4f, 0.f, 1.2f, FLinearColor(.4f, .4f, .45f));
		Sprite(ETex::Star, EFace::Camera, Me, Me, 1.f, 6.f, 0.f, .2f, White, 10.f);
		Streaks(Me, 12, 700.f, 0.f, .4f, Mix(T, Void, .5f), .04f);
		C->SetActorHiddenInGame(true);
		bRestoreHidden = true; RestoreAt = Dur;
		if (Combat.IsValid()) { Combat->SetInvulnerable(Dur * .6f); Combat->AddBuff(EDFBuff::Damage, 1.7f, Dur + 2.f); Combat->AddBuff(EDFBuff::Speed, 1.4f, Dur); }
		Sprite(ETex::SoftGlow, EFace::Flat, FVector(0, 0, -85), FVector(0, 0, -85), 1.4f, 1.4f, .2f, Dur, Mix(T, Void, .5f), 1.2f);
		Last().Follow = Caster; Last().bFlicker = true;
		Life = FMath::Max(Life, Dur + .2f);
		break;
	}
	case EArch::Warcry: // battle aura: buffs + burning ring around us
	{
		const float Dur = bUlt ? 9.f : 6.f;
		if (Combat.IsValid()) { Combat->AddBuff(EDFBuff::Damage, bUlt ? 1.7f : 1.4f, Dur); Combat->AddBuff(EDFBuff::Speed, 1.15f, Dur); }
		Ring(Feet + FVector(0, 0, 10), 40.f, 650.f, 0.f, .5f, Hot, 12, .3f);
		Sprite(ETex::Star, EFace::Camera, Me, Me, 2.f, 10.f, 0.f, .3f, White, 10.f);
		Sprite(ETex::Ring, EFace::Flat, FVector(0, 0, -80), FVector(0, 0, -80), 3.2f, 3.4f, 0.f, Dur, T, 4.f, 0.f, 120.f);
		Last().Follow = Caster;
		Sprite(ETex::Shock, EFace::Flat, FVector(0, 0, -78), FVector(0, 0, -78), 3.6f, 3.8f, 0.f, Dur, Hot, 2.f, 0.f, -80.f);
		Last().Follow = Caster;
		for (float t = 0.f; t < Dur; t += .12f)
		{
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			const FVector O(FMath::Cos(A) * 110.f, FMath::Sin(A) * 110.f, -70.f);
			Sprite(ETex::SoftGlow, EFace::Camera, O, O + FVector(0, 0, FMath::FRandRange(120.f, 240.f)), .5f, .1f, t, t + .7f, FMath::RandBool() ? T : Hot, 7.f);
			Last().Follow = Caster;
		}
		for (float t = .5f; t < Dur; t += .5f) { FHit& H = Hit(t, FVector::ZeroVector, 230.f, Dmg * .15f); H.bAtCaster = true; }
		break;
	}
	case EArch::Beast: // spirit wolves pounce from enemy to enemy
	{
		const int32 Packs = bUlt ? 3 : 1;
		const TArray<AActor*> Ts = TargetActors(6, 2400.f);
		for (int32 W = 0; W < Packs; ++W)
		{
			FVector Prev = Me + FVector::CrossProduct(Fwd, FVector::UpVector) * (W - (Packs - 1) * .5f) * 200.f;
			float T0 = .1f * W;
			const FLinearColor Col = Mix(T, Electric, .35f);
			const int32 Jumps = FMath::Max(3, Ts.Num());
			for (int32 J = 0; J < Jumps; ++J)
			{
				AActor* Tg = Ts.Num() ? Ts[(J + W) % Ts.Num()] : nullptr;
				const FVector P1 = Tg ? Tg->GetActorLocation() : Me + Fwd.RotateAngleAxis(J * 40.f - 40.f, FVector::UpVector) * 800.f;
				const float Fly = .32f;
				// body, head, eyes, tail
				for (int32 L = 0; L < 4; ++L)
				{
					const float Sz = L == 0 ? 3.2f : L == 1 ? 2.f : L == 2 ? .8f : 1.5f;
					Sprite(L == 2 ? ETex::Star : ETex::SoftGlow, EFace::Camera, Prev, P1, Sz, Sz, T0 + L * .03f, T0 + Fly + L * .03f, L == 2 ? White : Col, L == 2 ? 12.f : 5.f);
					FPiece& P = Last(); P.bCurve = true; P.Ctrl = (Prev + P1) * .5f + FVector(0, 0, 260.f); P.Chase = Tg;
				}
				Smoke(Prev, 1, .7f, T0, .6f, Col);
				if (Tg) { FHit& H = Hit(T0 + Fly, FVector::ZeroVector, 150.f, Dmg * .9f, .3f); H.Target = Tg; H.FxScale = .5f; }
				Prev = P1; T0 += Fly + .05f;
			}
		}
		break;
	}
	case EArch::Clone: // shadow copies of us join the fight
	{
		const int32 N = bUlt ? 3 : 1;
		for (int32 I = 0; I < N; ++I)
		{
			const float Ang = N > 1 ? -60.f + 120.f * I / (N - 1) : 0.f;
			const FVector At = Me + Fwd.RotateAngleAxis(Ang, FVector::UpVector) * 220.f;
			SpawnAlly(At, bUlt ? 12.f : 8.f, bUlt ? .9f : .7f);
		}
		Ring(Feet + FVector(0, 0, 10), 50.f, 500.f, 0.f, .5f, Mix(T, Void, .5f), 12, .3f);
		Flash(Me, 50.f, 0.f, .5f);
		break;
	}
	case EArch::Orbit: // blades circle us and shred whatever they touch
	{
		const int32 N = bUlt ? 8 : 5;
		const float Dur = bUlt ? 8.f : 6.f, R = 230.f;
		for (int32 I = 0; I < N; ++I)
		{
			for (int32 L = 0; L < 2; ++L)
			{
				Sprite(L ? ETex::Spark : ETex::SoftGlow, L ? EFace::Axial : EFace::Camera, FVector(0, 0, 10), FVector(0, 0, 10), L ? 2.2f : 1.2f, L ? 2.2f : 1.2f, 0.f, Dur, L ? White : T, L ? 10.f : 6.f);
				FPiece& P = Last(); P.Follow = Caster; P.OrbitR = R; P.OrbitSpeed = 280.f; P.OrbitPhase = 360.f * I / N;
				if (L) { P.S0 = FVector(1.8f, .9f, 1); P.S1 = P.S0; }
			}
		}
		for (float t = .2f; t < Dur; t += .25f) { FHit& H = Hit(t, FVector::ZeroVector, R + 60.f, Dmg * .2f); H.bAtCaster = true; }
		Sprite(ETex::Ring, EFace::Flat, FVector(0, 0, -80), FVector(0, 0, -80), R / 39.f, R / 39.f, 0.f, Dur, T, 2.5f, 0.f, 280.f);
		Last().Follow = Caster;
		break;
	}
	case EArch::Turret: // a crystal tower that shoots the nearest enemy
	{
		const FVector P0 = Ground(Me + Fwd * 250.f);
		const float Dur = bUlt ? 9.f : 6.f;
		Piece(EShape::Cone, P0 - FVector(0, 0, 80), P0 + FVector(0, 0, 80), FVector(.6f, .6f, .5f), FVector(.6f, .6f, 1.8f), 0.f, Dur, Mix(T, Ice, .3f), 2.5f);
		Last().bEaseOut = false;
		Piece(EShape::Cone, P0 + FVector(0, 0, 210), P0 + FVector(0, 0, 210), FVector(.4f, .4f, .6f), FVector(.4f, .4f, .6f), .2f, Dur, Mix(T, White, .4f), 6.f, FRotator(180.f, 0.f, 0.f), 120.f);
		Sprite(ETex::SoftGlow, EFace::Camera, P0 + FVector(0, 0, 230), P0 + FVector(0, 0, 230), 2.f, 2.5f, .2f, Dur, Hot, 6.f);
		Last().bFlicker = true;
		Sprite(ETex::Ring, EFace::Flat, P0 + FVector(0, 0, 4), P0 + FVector(0, 0, 4), 3.f, 3.f, 0.f, Dur, T, 3.f, 0.f, 90.f);
		Smoke(P0, 4, 1.f, 0.f, .8f, T);
		for (float t = .5f; t < Dur; t += .4f) { FHit& H = Hit(t, P0 + FVector(0, 0, 230), 1500.f, Dmg * .55f); H.bShot = true; }
		break;
	}
	case EArch::Transform: // power form: bigger, faster, harder; burning aura
	{
		const float Dur = 8.f;
		const bool bDragon = Skill == FName(TEXT("ult_dragonform"));
		const FLinearColor Col = bDragon ? Mix(Fire, T, .3f) : Mix(Void, T, .3f);
		RestoreScale = C->GetActorScale3D();
		C->SetActorScale3D(RestoreScale * 1.35f);
		RestoreAt = Dur; Life = FMath::Max(Life, Dur + .2f);
		if (Combat.IsValid()) { Combat->AddBuff(EDFBuff::Damage, 1.8f, Dur); Combat->AddBuff(EDFBuff::Speed, 1.3f, Dur); Combat->AddBuff(EDFBuff::Guard, .6f, Dur); Combat->Heal(Combat->MaxHP * .2f); }
		// eruption
		Impact(Me, 1.8f, 0.f, Col, Mix(Col, White, .5f));
		Sprite(ETex::SoftGlow, EFace::Camera, Me, Me + FVector(0, 0, 600), 3.f, 2.f, 0.f, .6f, Col, 8.f, 0.f, 0.f, false, .4f);
		Ring(Feet + FVector(0, 0, 10), 50.f, 900.f, .05f, .6f, Col, 12, .3f);
		Hit(.1f, Me, 600.f, Dmg * 2.f, .6f).Push = 900.f;
		// lingering aura
		for (float t = 0.f; t < Dur; t += .07f)
		{
			const float A = FMath::FRandRange(0.f, 2.f * PI);
			const FVector O(FMath::Cos(A) * 70.f, FMath::Sin(A) * 70.f, FMath::FRandRange(-90.f, 40.f));
			Sprite(FMath::RandBool() ? ETex::SoftGlow : ETex::Smoke1, EFace::Camera, O, O + FVector(0, 0, FMath::FRandRange(150.f, 300.f)), 1.2f, .2f, t, t + .6f, FMath::RandBool() ? Col : Mix(Col, White, .4f), 5.f, FMath::FRandRange(0.f, 360.f));
			Last().Follow = Caster;
		}
		Sprite(ETex::Ring, EFace::Flat, FVector(0, 0, -85), FVector(0, 0, -85), 4.f, 4.f, 0.f, Dur, Col, 4.f, 0.f, 150.f);
		Last().Follow = Caster;
		for (float t = .6f; t < Dur; t += .6f) { FHit& H = Hit(t, FVector::ZeroVector, 260.f, Dmg * .3f); H.bAtCaster = true; }
		Flash(Me, 120.f, 0.f, .8f);
		break;
	}
	case EArch::Charge: // gather power, then release a huge piercing blast
	{
		const float Gather = .9f;
		if (Combat.IsValid()) Combat->Stun(Gather);
		for (int32 I = 0; I < 40; ++I)
		{
			const FVector From = FMath::VRand() * FMath::FRandRange(250.f, 500.f);
			const float T0 = FMath::FRandRange(0.f, Gather - .3f);
			Sprite(ETex::Spark, EFace::Axial, From + FVector(0, 0, 60), FVector(0, 0, 60), 1.2f, .4f, T0, T0 + .3f, I % 3 ? T : White, 9.f);
			FPiece& P = Last(); P.Follow = Caster; P.Axis = (-From).GetSafeNormal(); P.S0 = FVector(1.5f, .5f, 1); P.S1 = FVector(.3f, .3f, 1);
		}
		Sprite(ETex::SoftGlow, EFace::Camera, FVector(0, 0, 60), FVector(0, 0, 60), .5f, 5.f, 0.f, Gather, Hot, 8.f);
		Last().Follow = Caster; Last().bFlicker = true;
		Sprite(ETex::Ring, EFace::Camera, FVector(0, 0, 60), FVector(0, 0, 60), 7.f, .5f, 0.f, Gather, T, 5.f, 0.f, 200.f);
		Last().Follow = Caster;
		// release: giant orb + beam
		const FVector Start = Me + Fwd * 80.f + FVector(0, 0, 50);
		const FVector End = Start + Fwd * 2200.f;
		for (int32 L = 0; L < 3; ++L)
			Sprite(L == 2 ? ETex::Star : ETex::SoftGlow, EFace::Camera, Start, End, L ? 3.f : 7.f, L ? 3.f : 7.f, Gather, Gather + .5f, L == 0 ? T : L == 1 ? Hot : White, L == 2 ? 12.f : 7.f, 0.f, 500.f);
		Beam(Start, Fwd, 2200.f, Gather, .35f, T, Hot, 2.2f, Dmg * 2.5f);
		for (int32 K = 1; K <= 8; ++K) Hit(Gather + .5f * K / 8.f, FMath::Lerp(Start, End, K / 8.f), 230.f, Dmg * .6f, .3f);
		Hit(Gather + .5f, End, 450.f, Dmg * 1.5f).FxScale = 1.3f;
		Flash(Start, 120.f, Gather, Gather + .6f, End);
		break;
	}
	case EArch::Mark: // brand enemies, then detonate the brands
	{
		const TArray<AActor*> Ts = TargetActors(bUlt ? 10 : 5, 1800.f);
		const float Boom = 1.6f;
		for (AActor* Tg : Ts)
		{
			Sprite(ETex::Ring, EFace::Camera, FVector(0, 0, 160), FVector(0, 0, 160), 3.f, 1.4f, 0.f, Boom, Mix(T, Blood, .4f), 6.f, 0.f, 240.f);
			Last().Follow = Tg;
			Sprite(ETex::Star, EFace::Camera, FVector(0, 0, 160), FVector(0, 0, 160), 1.6f, 1.2f, 0.f, Boom, White, 9.f, 0.f, -200.f);
			Last().Follow = Tg; Last().bFlicker = true;
			FHit& H = Hit(Boom, FVector::ZeroVector, 200.f, Dmg * 2.2f, .4f); H.Target = Tg; H.FxScale = .8f;
		}
		Sprite(ETex::Star, EFace::Camera, Me + FVector(0, 0, 80), Me + FVector(0, 0, 80), 1.f, 5.f, 0.f, .25f, Hot, 10.f);
		break;
	}
	case EArch::Parry: // guard stance; on release a counter burst
	{
		const float Stance = 1.f;
		if (Combat.IsValid()) { Combat->AddBuff(EDFBuff::Guard, .05f, Stance); Combat->SetInvulnerable(.5f); }
		Sprite(ETex::Ring, EFace::Camera, FVector(0, 0, 40), FVector(0, 0, 40), 4.5f, 4.f, 0.f, Stance, Mix(T, Gold, .5f), 5.f, 0.f, 200.f);
		Last().Follow = Caster;
		Sprite(ETex::SoftGlow, EFace::Camera, FVector(0, 0, 40), FVector(0, 0, 40), 5.f, 5.5f, 0.f, Stance, Mix(T, Gold, .5f), 2.f);
		Last().Follow = Caster; Last().bFlicker = true;
		for (int32 K = 0; K < 3; ++K)
		{
			const float Yaw = FMath::FRandRange(0.f, 360.f);
			Sprite(ETex::Slash, EFace::Flat, FVector(0, 0, 40 + K * 20), FVector(0, 0, 40 + K * 20), 6.f, 8.f, Stance + K * .05f, Stance + .3f + K * .05f, K ? T : White, 9.f, Yaw);
			Last().Follow = Caster;
		}
		FHit& H = Hit(Stance + .02f, FVector::ZeroVector, 420.f, Dmg * 2.5f, .8f); H.bAtCaster = true; H.FxScale = 1.1f; H.Push = 700.f;
		break;
	}
	case EArch::Combo: // four quick slashes, last one launches
	{
		const float Times[4] = { 0.f, .18f, .36f, .6f };
		const float Yaws[4] = { 60.f, -60.f, 0.f, 180.f };
		for (int32 S = 0; S < 4; ++S)
		{
			const float T0 = Times[S];
			const FVector Off = Fwd * (150.f + 30.f * S) + FVector(0, 0, 50);
			const float Yaw = Fwd.Rotation().Yaw + 90.f + Yaws[S];
			for (int32 L = 0; L < 2; ++L)
			{
				Sprite(ETex::Slash, S == 3 ? EFace::Camera : EFace::Flat, Off, Off + Fwd * 60.f, (L ? 5.f : 4.f) * Big, (L ? 7.f : 5.5f) * Big, T0 + L * .02f, T0 + .25f + L * .02f, L ? T : White, L ? 6.f : 9.f, S == 3 ? 180.f : Yaw);
				Last().Follow = Caster;
			}
			FHit& H = Hit(T0 + .05f, FVector::ZeroVector, 230.f * Big, Dmg * (S == 3 ? 1.2f : .6f), .35f); H.bAtCaster = true; H.Offset = Fwd * 170.f;
			if (S == 3) { H.Lift = 900.f; H.FxScale = .9f; }
			Streaks(Me + Off, 6, 700.f, T0 + .05f, .35f, Hot, .04f);
		}
		if (ACharacter* Ch = Cast<ACharacter>(C)) Ch->LaunchCharacter(Fwd * 900.f, true, false);
		break;
	}
	}
}


// first landed hit: camera shake + brief hit-stop (real-time length)
void ADFSkillFX::Feel()
{
	bImpactFelt = true;
	if (ADFPlayerCharacter* PC = Cast<ADFPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) PC->AddShake(ShakeOnHit);
	if (StopOnHit <= 0.f) return;
	UWorld* W = GetWorld();
	const float Slow = .06f;
	UGameplayStatics::SetGlobalTimeDilation(W, Slow);
	FTimerHandle TH;
	TWeakObjectPtr<UWorld> WW = W;
	W->GetTimerManager().SetTimer(TH, FTimerDelegate::CreateLambda([WW]() { if (WW.IsValid()) UGameplayStatics::SetGlobalTimeDilation(WW.Get(), 1.f); }), StopOnHit * Slow, false);
}

// ---------- update ----------
void ADFSkillFX::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;
	if (APlayerController* PCtl = GetWorld()->GetFirstPlayerController())
		if (PCtl->PlayerCameraManager) CamLoc = PCtl->PlayerCameraManager->GetCameraLocation();

	for (FPiece& P : Pieces)
	{
		if (!P.Mesh) continue;
		const bool bOn = Age >= P.T0 && Age <= P.T1;
		P.Mesh->SetVisibility(bOn);
		if (!bOn) continue;
		const float A = FMath::Clamp((Age - P.T0) / FMath::Max(.01f, P.T1 - P.T0), 0.f, 1.f);
		const float E = 1.f - FMath::Square(1.f - A); // ease out
		if (P.bPhys)
		{
			// integrate: drag slows, gravity pulls; streaks stretch along their velocity
			P.Vel *= FMath::Max(0.f, 1.f - P.Drag * Dt);
			P.Vel.Z += P.Grav * Dt;
			P.From += P.Vel * Dt;
			P.Mesh->SetWorldLocation(P.From);
			FVector S = FMath::Lerp(P.S0, P.S1, E);
			if (P.bStreak)
			{
				const float Speed = P.Vel.Size();
				if (P.Face == EFace::Axial) { if (Speed > 1.f) P.Axis = P.Vel / Speed; S.X = FMath::Clamp(Speed * .0016f, .15f, 2.4f); }
				else { S.Z = FMath::Clamp(Speed * .0009f, .05f, 1.4f); if (Speed > 1.f) P.Mesh->SetWorldRotation(FRotationMatrix::MakeFromZ(P.Vel / Speed).Rotator()); }
			}
			P.Mesh->SetWorldScale3D(S);
		}
		else
		{
			const FVector Prev = P.Mesh->GetComponentLocation();
			FVector To = P.To;
			if (P.Chase.IsValid()) To = P.Chase->GetActorLocation() + (P.Chase == Caster ? FVector(0, 0, 50) : FVector::ZeroVector);
			FVector Pos = P.bCurve ? FMath::Lerp(FMath::Lerp(P.From, P.Ctrl, A), FMath::Lerp(P.Ctrl, To, A), A)
				: FMath::Lerp(P.From, To, P.From == To ? 0.f : A);
			if (P.OrbitR > 0.f)
			{
				const float Ang = FMath::DegreesToRadians(P.OrbitPhase + P.OrbitSpeed * (Age - P.T0));
				Pos += FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0) * P.OrbitR;
				if (P.Face == EFace::Axial) P.Axis = FVector(-FMath::Sin(Ang), FMath::Cos(Ang), 0);
			}
			if (P.Follow.IsValid()) P.LastFollow = P.Follow->GetActorLocation();
			if (!P.Follow.IsExplicitlyNull()) Pos += P.LastFollow;
			P.Mesh->SetWorldLocation(Pos);
			if (P.Face == EFace::Axial && P.OrbitR <= 0.f && (P.bCurve || P.From != P.To) && FVector::DistSquared(Pos, Prev) > 1.f && Age > P.T0 + Dt)
				P.Axis = (Pos - Prev).GetSafeNormal();
			P.Mesh->SetWorldScale3D(FMath::Lerp(P.S0, P.S1, P.bEaseOut ? E : A));
			if (P.Spin != 0.f) P.Mesh->SetWorldRotation(P.Rot + FRotator(0, P.Spin * Age, 0));
			else P.Mesh->SetWorldRotation(P.Rot);
		}
		if (P.Face != EFace::None)
		{
			// sprites: face the camera / lie flat / stretch along an axis
			const FVector Loc = P.Mesh->GetComponentLocation();
			FVector ToCam = (CamLoc - Loc).GetSafeNormal();
			if (ToCam.IsNearlyZero()) ToCam = FVector::UpVector;
			const float RollNow = P.Roll + P.RollSpin * (Age - P.T0);
			FQuat Q;
			if (P.Face == EFace::Camera) Q = FQuat(ToCam, FMath::DegreesToRadians(RollNow)) * FRotationMatrix::MakeFromZ(ToCam).ToQuat();
			else if (P.Face == EFace::Flat) Q = FRotator(0, RollNow, 0).Quaternion();
			else Q = FRotationMatrix::MakeFromXZ(P.Axis.IsNearlyZero() ? FVector::ForwardVector : P.Axis, ToCam).ToQuat();
			P.Mesh->SetWorldRotation(Q);
		}
		float Op = FMath::Min(1.f, A / .1f) * (A > .6f ? (1.f - A) / .4f : 1.f);
		if (P.bFlicker) Op *= FMath::FRandRange(.55f, 1.f);
		if (P.Mat) P.Mat->SetScalarParameterValue(TEXT("Opacity"), Op);
	}

	TickMotes(Dt);

	if (LightPeak > 0.f)
	{
		const float A = FMath::Clamp((Age - LightT0) / FMath::Max(.01f, LightT1 - LightT0), 0.f, 1.f);
		Light->SetWorldLocation(FMath::Lerp(LightFrom, LightTo, A));
		Light->SetIntensity(Age < LightT0 || Age > LightT1 ? 0.f : LightPeak * FMath::Sin(A * PI));
	}

	const int32 Team = Combat.IsValid() ? Combat->Team : 0;
	const FLinearColor Hot = Mix(Tint, White, .22f);
	for (int32 HI = 0; HI < Hits.Num(); ++HI)
	{
		FHit& H = Hits[HI];
		if (H.bDone || Age < H.At) continue;
		H.bDone = true;
		// where does it land?
		if (H.bAtCaster) { if (!Caster.IsValid()) continue; H.Center = Caster->GetActorLocation() + H.Offset; }
		if (!H.Target.IsExplicitlyNull())
		{
			UDFCombatComponent* TC = H.Target.IsValid() ? H.Target->FindComponentByClass<UDFCombatComponent>() : nullptr;
			if (!TC || TC->IsDead()) continue;
			H.Center = H.Target->GetActorLocation();
		}
		if (H.HealFrac > 0.f && Combat.IsValid()) Combat->Heal(Combat->MaxHP * H.HealFrac);
		if (H.FxScale > 0.f) Impact(H.Center, H.FxScale, Age, Tint, Hot);
		if (H.bShot)
		{
			// turret: bolt to the nearest enemy in range
			UDFCombatComponent* Best = nullptr; float BestD = H.Radius;
			for (UDFCombatComponent* O : UDFCombatComponent::Registry())
				if (O && O->Team != Team && !O->IsDead() && O->GetOwner())
				{
					const float D = FVector::Dist(O->GetOwner()->GetActorLocation(), H.Center);
					if (D < BestD) { BestD = D; Best = O; }
				}
			if (!Best) continue;
			const FVector E = Best->GetOwner()->GetActorLocation();
			Sprite(ETex::Spark, EFace::Axial, H.Center, E, 1.f, .8f, Age, Age + .12f, Hot, 10.f);
			Last().S0 = FVector(2.5f, .9f, 1); Last().S1 = FVector(2.5f, .6f, 1); Last().Axis = (E - H.Center).GetSafeNormal();
			Sprite(ETex::Star, EFace::Camera, H.Center, H.Center, 1.5f, .5f, Age, Age + .12f, White, 10.f);
			Impact(E, .35f, Age + .12f, Tint, Hot);
			Best->ApplyDamage(H.Damage, Caster.Get());
			if (!bImpactFelt) Feel();
			continue;
		}
		if (H.Damage <= 0.f && H.Stun <= 0.f && H.Pull <= 0.f && H.Lift <= 0.f && H.Push == 0.f && H.SlowMul <= 0.f) continue;
		for (UDFCombatComponent* O : UDFCombatComponent::Registry())
		{
			if (!O || O->Team == Team || O->IsDead() || !O->GetOwner()) continue;
			const FVector P = O->GetOwner()->GetActorLocation();
			if (FVector::Dist2D(P, H.Center) > H.Radius + O->Radius) continue;
			float Dealt = 0.f;
			if (H.Damage > 0.f) Dealt = O->ApplyDamage(H.Damage, Caster.Get());
			if (H.Damage > 0.f && !bImpactFelt) Feel();
			if (H.Drain > 0.f && Dealt > 0.f && Combat.IsValid()) Combat->Heal(Dealt * H.Drain);
			if (H.Stun > 0.f) O->Stun(H.Stun);
			if (H.SlowMul > 0.f) O->AddBuff(EDFBuff::Speed, H.SlowMul, H.SlowDur);
			ACharacter* Ch = Cast<ACharacter>(O->GetOwner());
			if (!Ch) continue;
			FVector Launch = FVector::ZeroVector;
			if (H.Pull > 0.f)
			{
				const FVector Dest = H.bPullToCaster && Caster.IsValid() ? Caster->GetActorLocation() : H.Center;
				FVector To = Dest - P; To.Z = 0;
				if (To.Size() > 60.f) Launch += To.GetSafeNormal() * FMath::Min(H.Pull, To.Size() * 2.5f);
			}
			if (H.Push != 0.f) { FVector Out = P - H.Center; Out.Z = 0; Launch += Out.GetSafeNormal() * H.Push + FVector(0, 0, FMath::Abs(H.Push) * .25f); }
			if (H.Lift > 0.f) Launch.Z += H.Lift;
			if (!Launch.IsNearlyZero()) Ch->LaunchCharacter(Launch, true, H.Lift > 0.f || H.Push != 0.f);
		}
	}

	// transform / vanish wear off
	if (RestoreAt > 0.f && !bRestored && Age >= RestoreAt)
	{
		bRestored = true;
		if (AActor* Ca = Caster.Get())
		{
			if (bRestoreHidden) Ca->SetActorHiddenInGame(false);
			else Ca->SetActorScale3D(RestoreScale);
		}
	}

	if (Age > Life) Destroy();
}
