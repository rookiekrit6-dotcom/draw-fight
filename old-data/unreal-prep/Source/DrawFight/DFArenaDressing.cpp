#include "DFArenaDressing.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UStaticMesh* Shape(int32 I)
	{
		static UStaticMesh* M[5] = {};
		static const TCHAR* P[5] = { TEXT("/Engine/BasicShapes/Cube.Cube"), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), TEXT("/Engine/BasicShapes/Sphere.Sphere"), TEXT("/Engine/BasicShapes/Cone.Cone"), TEXT("/Engine/BasicShapes/Plane.Plane") };
		if (!M[I]) M[I] = LoadObject<UStaticMesh>(nullptr, P[I]);
		return M[I];
	}
	enum { Cube = 0, Cyl = 1, Ball = 2, Cone = 3, Plane = 4 };
}

ADFArenaDressing::ADFArenaDressing()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

TArray<FName> ADFArenaDressing::Themes() { return { TEXT("lab"), TEXT("white"), TEXT("sky"), TEXT("volcano"), TEXT("ring") }; }

FString ADFArenaDressing::ThemeNameTH(FName T)
{
	if (T == TEXT("white")) return TEXT("ห้องสีขาว");
	if (T == TEXT("sky")) return TEXT("ลานกลางเวหา");
	if (T == TEXT("volcano")) return TEXT("ลานภูเขาไฟ");
	if (T == TEXT("ring")) return TEXT("ลานมวย");
	return TEXT("ห้องวิจัย");
}

FLinearColor ADFArenaDressing::ThemeColor(FName T)
{
	if (T == TEXT("white")) return FLinearColor(.75f, .78f, .85f);
	if (T == TEXT("sky")) return FLinearColor(.35f, .65f, 1.f);
	if (T == TEXT("volcano")) return FLinearColor(1.f, .35f, .08f);
	if (T == TEXT("ring")) return FLinearColor(.9f, .2f, .3f);
	return FLinearColor(.2f, .85f, 1.f);
}

ADFArenaDressing* ADFArenaDressing::Build(UWorld* World, FName InTheme, float Radius)
{
	if (!World) return nullptr;
	// floor height: lowest hit of a few traces around the middle
	float FloorZ = 1e9f;
	for (int32 I = 0; I < 6; ++I)
	{
		const FVector P(FMath::Cos(I * 1.05f) * 900.f, FMath::Sin(I * 1.05f) * 900.f, 0.f);
		FHitResult H;
		if (World->LineTraceSingleByChannel(H, P + FVector(0, 0, 3000), P - FVector(0, 0, 5000), ECC_WorldStatic)) FloorZ = FMath::Min(FloorZ, H.ImpactPoint.Z);
	}
	if (FloorZ > 1e8f) FloorZ = 0.f;
	// hide the level's own blockout
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		It->SetActorHiddenInGame(true);
		It->SetActorEnableCollision(false);
	}
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADFArenaDressing* D = World->SpawnActor<ADFArenaDressing>(ADFArenaDressing::StaticClass(), FVector(0, 0, FloorZ), FRotator::ZeroRotator, SP);
	if (!D) return nullptr;
	D->Theme = Themes().Contains(InTheme) ? InTheme : FName(TEXT("lab"));
	D->Make(Radius, FloorZ);
	return D;
}

UStaticMeshComponent* ADFArenaDressing::Solid(int32 S, FVector Loc, FRotator Rot, FVector Scale, FLinearColor Color, bool bCollide)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Shape(S));
	M->SetupAttachment(RootComponent);
	M->SetRelativeLocation(Loc); M->SetRelativeRotation(Rot); M->SetRelativeScale3D(Scale);
	if (bCollide) { M->SetCollisionProfileName(TEXT("BlockAll")); M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }
	else M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->RegisterComponent();
	if (UMaterialInterface* B = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* MI = UMaterialInstanceDynamic::Create(B, this);
		MI->SetVectorParameterValue(TEXT("Color"), Color);
		M->SetMaterial(0, MI);
	}
	Keep.Add(M);
	return M;
}

UStaticMeshComponent* ADFArenaDressing::Glow(const TCHAR* Tex, FVector Loc, FRotator Rot, FVector Scale, FLinearColor Color, float G, bool bDust)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(Shape(Plane));
	M->SetupAttachment(RootComponent);
	M->SetRelativeLocation(Loc); M->SetRelativeRotation(Rot); M->SetRelativeScale3D(Scale);
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCastShadow(false);
	M->RegisterComponent();
	if (UMaterialInterface* B = LoadObject<UMaterialInterface>(nullptr, bDust ? TEXT("/Game/DrawFight/FX/M_Dust.M_Dust") : TEXT("/Game/DrawFight/FX/M_Sprite.M_Sprite")))
	{
		UMaterialInstanceDynamic* MI = UMaterialInstanceDynamic::Create(B, this);
		if (UTexture* T = LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/DrawFight/FX/Tex/%s.%s"), Tex, Tex)))
			MI->SetTextureParameterValue(TEXT("Tex"), T);
		MI->SetVectorParameterValue(TEXT("Color"), Color);
		MI->SetScalarParameterValue(TEXT("Glow"), G);
		MI->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		M->SetMaterial(0, MI);
	}
	Keep.Add(M);
	return M;
}

void ADFArenaDressing::Mood(FLinearColor SunColor, float SunIntensity, float SunPitch, float SkyIntensity, FLinearColor FogColor, float FogDensity, float FogFalloff)
{
	UWorld* W = GetWorld();
	for (TActorIterator<ADirectionalLight> It(W); It; ++It)
	{
		UDirectionalLightComponent* L = Cast<UDirectionalLightComponent>(It->GetLightComponent());
		if (!L) continue;
		L->SetLightColor(SunColor); L->SetIntensity(SunIntensity);
		It->SetActorRotation(FRotator(SunPitch, 130.f, 0.f));
	}
	for (TActorIterator<ASkyLight> It(W); It; ++It)
		if (USkyLightComponent* S = It->GetLightComponent())
		{
			S->SetIntensity(SkyIntensity);
			S->RecaptureSky();
		}
	bool bFog = false;
	for (TActorIterator<AExponentialHeightFog> It(W); It; ++It)
	{
		UExponentialHeightFogComponent* F = It->GetComponent();
		if (!F) continue;
		F->SetFogDensity(FogDensity); F->SetFogInscatteringColor(FogColor); F->SetFogHeightFalloff(FogFalloff);
		bFog = true;
	}
	if (!bFog)
	{
		UExponentialHeightFogComponent* F = NewObject<UExponentialHeightFogComponent>(this);
		F->SetupAttachment(RootComponent);
		F->RegisterComponent();
		F->SetFogDensity(FogDensity); F->SetFogInscatteringColor(FogColor); F->SetFogHeightFalloff(FogFalloff);
	}
}

void ADFArenaDressing::Embers(int32 Count, float Radius, float FloorZ, FLinearColor C, const TCHAR* Tex)
{
	for (int32 I = 0; I < Count; ++I)
	{
		const float A = FMath::FRandRange(0.f, 2.f * PI), R = FMath::FRandRange(200.f, Radius);
		const FVector P(FMath::Cos(A) * R, FMath::Sin(A) * R, FMath::FRandRange(0.f, 600.f));
		UStaticMeshComponent* M = Glow(Tex, P, FRotator::ZeroRotator, FVector(FMath::FRandRange(.1f, .25f)), C, 4.f);
		FAnim An{ M, P, FMath::FRandRange(0.f, 20.f), 0.f, 0.f, FMath::FRandRange(25.f, 70.f), true, nullptr };
		Anims.Add(An);
	}
}

void ADFArenaDressing::Make(float Radius, float FloorZ)
{
	const float R = Radius + 250.f;
	const float FR = R / 50.f; // cylinder scale for radius R
	if (Theme == TEXT("lab"))
	{
		Mood(FLinearColor(.6f, .75f, 1.f), 4.f, -50.f, 1.1f, FLinearColor(.02f, .06f, .12f), .03f, .3f);
		Solid(Cyl, FVector(0, 0, -50), FRotator::ZeroRotator, FVector(FR, FR, 1.f), FLinearColor(.035f, .045f, .06f), true);
		Glow(TEXT("T_Circle3"), FVector(0, 0, 2), FRotator::ZeroRotator, FVector(R / 70.f), FLinearColor(.1f, .7f, 1.f), .35f);
		Glow(TEXT("T_Ring"), FVector(0, 0, 3), FRotator::ZeroRotator, FVector(R / 39.f), FLinearColor(.1f, .8f, 1.f), 1.2f);
		for (int32 I = 0; I < 24; ++I) // radial light strips
		{
			const float Yaw = I * 15.f;
			const FVector Dir = FRotator(0, Yaw, 0).Vector();
			Glow(TEXT("T_Spark"), Dir * R * .55f + FVector(0, 0, 2), FRotator(0, Yaw, 0), FVector(R / 110.f, .25f, 1.f), FLinearColor(.1f, .6f, 1.f), .5f);
		}
		for (int32 I = 0; I < 16; ++I) // wall panels with glowing seams
		{
			const float Yaw = I * 22.5f;
			const FVector Dir = FRotator(0, Yaw, 0).Vector();
			Solid(Cube, Dir * (R + 60.f) + FVector(0, 0, 250), FRotator(0, Yaw, 0), FVector(.6f, R * 2.f * PI / 16.f / 100.f, 5.f), FLinearColor(.05f, .06f, .08f), true);
			Glow(TEXT("T_Spark"), Dir * (R + 25.f) + FVector(0, 0, 250), FRotator(90.f, Yaw, 0), FVector(4.5f, .35f, 1.f), FLinearColor(.1f, .8f, 1.f), 3.f);
		}
		for (int32 I = 0; I < 6; ++I) // pillars with holo rings
		{
			const FVector P = FRotator(0, I * 60.f + 30.f, 0).Vector() * R * .72f;
			Solid(Cyl, P + FVector(0, 0, 200), FRotator::ZeroRotator, FVector(.9f, .9f, 4.f), FLinearColor(.06f, .07f, .09f), true);
			UStaticMeshComponent* H = Glow(TEXT("T_Circle2"), P + FVector(0, 0, 140), FRotator::ZeroRotator, FVector(2.4f), FLinearColor(.2f, .9f, 1.f), 2.f);
			Anims.Add({ H, P + FVector(0, 0, 140), I * 1.f, 15.f, 40.f, 0.f, false, nullptr });
		}
		Embers(40, R, FloorZ, FLinearColor(.2f, .8f, 1.f), TEXT("T_SoftGlow"));
	}
	else if (Theme == TEXT("white"))
	{
		Mood(FLinearColor(1.f, 1.f, 1.f), 6.f, -60.f, 2.5f, FLinearColor(.85f, .87f, .9f), .02f, .2f);
		Solid(Cyl, FVector(0, 0, -50), FRotator::ZeroRotator, FVector(FR, FR, 1.f), FLinearColor(.85f, .86f, .88f), true);
		for (int32 I = -12; I <= 12; ++I) // ink grid like the 2D version
		{
			Solid(Cube, FVector(I * 200.f, 0, 1), FRotator::ZeroRotator, FVector(.04f, R / 50.f, .02f), FLinearColor(.25f, .27f, .32f), false);
			Solid(Cube, FVector(0, I * 200.f, 1), FRotator::ZeroRotator, FVector(R / 50.f, .04f, .02f), FLinearColor(.25f, .27f, .32f), false);
		}
		for (int32 I = 0; I < 20; ++I)
		{
			const float Yaw = I * 18.f;
			Solid(Cube, FRotator(0, Yaw, 0).Vector() * (R + 60.f) + FVector(0, 0, 300), FRotator(0, Yaw, 0), FVector(.5f, R * 2.f * PI / 20.f / 100.f, 6.f), FLinearColor(.92f, .92f, .94f), true);
		}
	}
	else if (Theme == TEXT("sky"))
	{
		Mood(FLinearColor(1.f, .95f, .85f), 7.f, -35.f, 1.6f, FLinearColor(.45f, .65f, .95f), .015f, .05f);
		Solid(Cyl, FVector(0, 0, -60), FRotator::ZeroRotator, FVector(FR, FR, 1.2f), FLinearColor(.32f, .3f, .28f), true);
		Solid(Cone, FVector(0, 0, -520), FRotator(180.f, 0, 0), FVector(FR * .95f, FR * .95f, 8.f), FLinearColor(.22f, .2f, .19f), false);
		Glow(TEXT("T_Circle1"), FVector(0, 0, 2), FRotator::ZeroRotator, FVector(R / 75.f), FLinearColor(1.f, .85f, .5f), .8f);
		for (int32 I = 0; I < 30; ++I) // clouds below and around
		{
			const float A = I * 12.f + FMath::FRandRange(0.f, 8.f);
			const float D = R + FMath::FRandRange(400.f, 2500.f);
			const FVector P = FRotator(0, A, 0).Vector() * D + FVector(0, 0, FMath::FRandRange(-900.f, -150.f));
			UStaticMeshComponent* Cl = Glow(TEXT("T_Smoke1"), P, FRotator::ZeroRotator, FVector(FMath::FRandRange(12.f, 26.f)), FLinearColor(1.f, 1.f, 1.f), 1.6f, true);
			Anims.Add({ Cl, P, FMath::FRandRange(0.f, 10.f), 30.f, 0.f, 0.f, true, nullptr });
		}
		for (int32 I = 0; I < 10; ++I) // floating rocks
		{
			const FVector P = FRotator(0, I * 36.f, 0).Vector() * (R + FMath::FRandRange(300.f, 900.f)) + FVector(0, 0, FMath::FRandRange(-100.f, 500.f));
			UStaticMeshComponent* Rk = Solid(Cube, P, FRotator(FMath::FRandRange(0.f, 40.f), FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 40.f)), FVector(FMath::FRandRange(1.f, 3.f)), FLinearColor(.28f, .26f, .24f), false);
			Anims.Add({ Rk, P, I * 1.3f, 40.f, 6.f, 0.f, false, nullptr });
		}
	}
	else if (Theme == TEXT("volcano"))
	{
		Mood(FLinearColor(1.f, .5f, .3f), 3.5f, -25.f, .8f, FLinearColor(.25f, .05f, .02f), .045f, .25f);
		Solid(Cyl, FVector(0, 0, -50), FRotator::ZeroRotator, FVector(FR, FR, 1.f), FLinearColor(.05f, .035f, .03f), true);
		for (int32 I = 0; I < 18; ++I) // glowing lava cracks
		{
			const FVector P(FMath::FRandRange(-R, R) * .8f, FMath::FRandRange(-R, R) * .8f, 2.f);
			UStaticMeshComponent* Cr = Glow(TEXT("T_Crack"), P, FRotator(0, FMath::FRandRange(0.f, 360.f), 0), FVector(FMath::FRandRange(3.f, 7.f)), FLinearColor(1.f, .3f, .05f), 3.f);
			Anims.Add({ Cr, P, FMath::FRandRange(0.f, 6.f), 0.f, 0.f, 0.f, false, Cast<UMaterialInstanceDynamic>(Cr->GetMaterial(0)) });
		}
		for (int32 I = 0; I < 5; ++I) // lava pools
		{
			const FVector P = FRotator(0, I * 72.f + 20.f, 0).Vector() * R * .62f + FVector(0, 0, 3);
			Glow(TEXT("T_SoftGlow"), P, FRotator::ZeroRotator, FVector(6.f), FLinearColor(1.f, .25f, .02f), 2.5f);
			UPointLightComponent* PL = NewObject<UPointLightComponent>(this);
			PL->SetupAttachment(RootComponent); PL->RegisterComponent();
			PL->SetRelativeLocation(P + FVector(0, 0, 80)); PL->SetLightColor(FLinearColor(1.f, .35f, .1f)); PL->SetIntensity(30000.f); PL->SetAttenuationRadius(900.f); PL->SetCastShadows(false);
		}
		for (int32 I = 0; I < 22; ++I) // rock spires around the rim
		{
			const float Yaw = I * (360.f / 22.f) + FMath::FRandRange(-5.f, 5.f);
			const float H = FMath::FRandRange(4.f, 10.f);
			Solid(Cone, FRotator(0, Yaw, 0).Vector() * (R + FMath::FRandRange(80.f, 400.f)) + FVector(0, 0, H * 50.f - 60.f), FRotator(FMath::FRandRange(-8.f, 8.f), 0, FMath::FRandRange(-8.f, 8.f)), FVector(FMath::FRandRange(3.f, 6.f), FMath::FRandRange(3.f, 6.f), H), FLinearColor(.06f, .04f, .035f), true);
		}
		Embers(90, R, FloorZ, FLinearColor(1.f, .45f, .1f), TEXT("T_Flame"));
	}
	else // ring
	{
		Mood(FLinearColor(.8f, .85f, 1.f), .6f, -70.f, .25f, FLinearColor(.03f, .02f, .06f), .02f, .2f);
		const float H = Radius * .74f; // half size of the square
		Solid(Cyl, FVector(0, 0, -60), FRotator::ZeroRotator, FVector(FR * 1.4f, FR * 1.4f, 1.f), FLinearColor(.02f, .02f, .03f), true);
		Solid(Cube, FVector(0, 0, -45), FRotator::ZeroRotator, FVector(H / 50.f, H / 50.f, 1.f), FLinearColor(.06f, .12f, .32f), true);
		Glow(TEXT("T_Circle1"), FVector(0, 0, 8), FRotator::ZeroRotator, FVector(H / 90.f), FLinearColor(1.f, .8f, .3f), .6f);
		const FLinearColor Corner[4] = { FLinearColor(.8f, .08f, .08f), FLinearColor(.08f, .2f, .8f), FLinearColor(.8f, .08f, .08f), FLinearColor(.08f, .2f, .8f) };
		for (int32 I = 0; I < 4; ++I)
		{
			const FVector C(((I == 0 || I == 3) ? 1.f : -1.f) * H, ((I < 2) ? 1.f : -1.f) * H, 0.f);
			Solid(Cyl, C + FVector(0, 0, 90), FRotator::ZeroRotator, FVector(.35f, .35f, 2.2f), Corner[I], true);
		}
		for (int32 S = 0; S < 4; ++S) // ropes
			for (int32 L = 0; L < 3; ++L)
			{
				const float Z = 60.f + L * 55.f;
				const bool bX = S % 2 == 0;
				const float Off = (S < 2 ? 1.f : -1.f) * H;
				const FVector P = bX ? FVector(0, Off, Z) : FVector(Off, 0, Z);
				Solid(Cyl, P, bX ? FRotator(90.f, 0, 0) : FRotator(0, 0, 90.f), FVector(.06f, .06f, H / 50.f), L == 1 ? FLinearColor(.9f, .9f, .9f) : FLinearColor(.75f, .05f, .08f), true);
			}
		for (int32 I = 0; I < 4; ++I) // stadium spotlights
		{
			USpotLightComponent* SL = NewObject<USpotLightComponent>(this);
			SL->SetupAttachment(RootComponent); SL->RegisterComponent();
			const FVector P = FRotator(0, I * 90.f + 45.f, 0).Vector() * H * 1.3f + FVector(0, 0, 1400);
			SL->SetRelativeLocation(P);
			SL->SetRelativeRotation((FVector(0, 0, 0) - P).Rotation());
			SL->SetIntensity(400000.f); SL->SetAttenuationRadius(4000.f); SL->SetOuterConeAngle(32.f); SL->SetInnerConeAngle(18.f);
			SL->SetLightColor(FLinearColor(1.f, .95f, .85f)); SL->SetCastShadows(true);
		}
		for (int32 I = 0; I < 120; ++I) // crowd silhouettes in the dark + phone lights
		{
			const float Yaw = FMath::FRandRange(0.f, 360.f);
			const FVector P = FRotator(0, Yaw, 0).Vector() * FMath::FRandRange(H * 1.55f, H * 2.1f);
			Solid(Ball, P + FVector(0, 0, 40), FRotator::ZeroRotator, FVector(.5f, .5f, 1.2f), FLinearColor(.015f, .015f, .02f), false);
			if (I % 4 == 0) Glow(TEXT("T_SoftGlow"), P + FVector(0, 0, 110), FRotator::ZeroRotator, FVector(.25f), FLinearColor(.8f, .85f, 1.f), 3.f);
		}
	}
}

void ADFArenaDressing::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;
	FVector Cam = FVector::ZeroVector;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		if (PC->PlayerCameraManager) Cam = PC->PlayerCameraManager->GetCameraLocation();
	for (FAnim& A : Anims)
	{
		if (!A.Mesh) continue;
		FVector P = A.Base;
		if (A.Rise > 0.f) P.Z = FMath::Fmod(A.Base.Z + (Age + A.Phase) * A.Rise, 650.f);
		P.Z += FMath::Sin(Age * .8f + A.Phase) * A.Bob;
		A.Mesh->SetRelativeLocation(P);
		if (A.bBill)
		{
			const FVector W = GetActorLocation() + P;
			A.Mesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Cam - W).Rotator());
		}
		else if (A.Spin != 0.f) A.Mesh->SetRelativeRotation(FRotator(0, Age * A.Spin + A.Phase * 30.f, 0));
		if (A.Mat) A.Mat->SetScalarParameterValue(TEXT("Glow"), 2.f + 1.5f * FMath::Sin(Age * 1.7f + A.Phase));
	}
}
