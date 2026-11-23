#include "DFMenuStage.h"
#include "DFProfileSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* StageManny = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const TCHAR* StageAnim  = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");
	UMaterialInterface* LoadMat(const TCHAR* P) { return LoadObject<UMaterialInterface>(nullptr, P); }
	UTexture* LoadTex(const TCHAR* Name) { return LoadObject<UTexture>(nullptr, *FString::Printf(TEXT("/Game/DrawFight/FX/Tex/%s.%s"), Name, Name)); }
}

ADFMenuStage::ADFMenuStage()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// sky + sun + ambient + fog: a dusk sky that makes the glow pop
	USkyAtmosphereComponent* Sky = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Sky"));
	Sky->SetupAttachment(RootComponent);
	UDirectionalLightComponent* Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetRelativeRotation(FRotator(-8.f, 140.f, 0.f)); // low sun = orange horizon
	Sun->SetIntensity(5.f);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetLightColor(FLinearColor(1.f, .75f, .55f));
	USkyLightComponent* Amb = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	Amb->SetupAttachment(RootComponent);
	Amb->bRealTimeCapture = true;
	Amb->SetIntensity(1.2f);
	UExponentialHeightFogComponent* Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->SetFogDensity(.035f);
	Fog->SetFogHeightFalloff(.4f);
	Fog->SetFogInscatteringColor(FLinearColor(.12f, .1f, .2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(RootComponent);
	if (Cyl.Succeeded()) Floor->SetStaticMesh(Cyl.Object);
	Floor->SetRelativeLocation(FVector(0, 0, -105));
	Floor->SetRelativeScale3D(FVector(80.f, 80.f, .1f));
	Pedestal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pedestal"));
	Pedestal->SetupAttachment(RootComponent);
	if (Cyl.Succeeded()) Pedestal->SetStaticMesh(Cyl.Object);
	Pedestal->SetRelativeLocation(FVector(0, 0, -98));
	Pedestal->SetRelativeScale3D(FVector(2.6f, 2.6f, .16f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshF(StageManny);
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimF(StageAnim);
	Hero = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Hero"));
	Hero->SetupAttachment(RootComponent);
	if (MeshF.Succeeded()) Hero->SetSkeletalMesh(MeshF.Object);
	if (AnimF.Succeeded()) Hero->SetAnimInstanceClass(AnimF.Class);
	Hero->SetRelativeLocation(FVector(0, 0, -90));
	Hero->SetRelativeRotation(FRotator(0, -90.f, 0));

	HeroLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("HeroLight"));
	HeroLight->SetupAttachment(RootComponent);
	HeroLight->SetRelativeLocation(FVector(-120, 80, 60));
	HeroLight->SetIntensity(9000.f);
	HeroLight->SetAttenuationRadius(900.f);
	HeroLight->SetCastShadows(false);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(42.f);
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_BloomIntensity = true; PP.BloomIntensity = 1.6f;
	PP.bOverride_VignetteIntensity = true; PP.VignetteIntensity = .6f;
	PP.bOverride_AutoExposureMinBrightness = true; PP.AutoExposureMinBrightness = 1.f;
	PP.bOverride_AutoExposureMaxBrightness = true; PP.AutoExposureMaxBrightness = 1.f;
	PP.bOverride_DepthOfFieldFocalDistance = true; PP.DepthOfFieldFocalDistance = 520.f;
	PP.bOverride_DepthOfFieldFstop = true; PP.DepthOfFieldFstop = 2.2f;
}

UStaticMeshComponent* ADFMenuStage::Plane(const TCHAR* Tex, bool bFlat, UMaterialInstanceDynamic*& OutMat)
{
	UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(this);
	M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	M->SetCastShadow(false);
	M->SetupAttachment(RootComponent);
	M->RegisterComponent();
	OutMat = nullptr;
	if (UMaterialInterface* S = LoadMat(TEXT("/Game/DrawFight/FX/M_Sprite.M_Sprite")))
	{
		OutMat = UMaterialInstanceDynamic::Create(S, this);
		if (UTexture* T = LoadTex(Tex)) OutMat->SetTextureParameterValue(TEXT("Tex"), T);
		OutMat->SetScalarParameterValue(TEXT("Opacity"), 1.f);
		M->SetMaterial(0, OutMat);
	}
	Keep.Add(M);
	return M;
}

void ADFMenuStage::BeginPlay()
{
	Super::BeginPlay();
	// dark stone floor and pedestal
	if (UMaterialInterface* Basic = LoadMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		UMaterialInstanceDynamic* F = UMaterialInstanceDynamic::Create(Basic, this);
		F->SetVectorParameterValue(TEXT("Color"), FLinearColor(.035f, .035f, .05f));
		Floor->SetMaterial(0, F);
		UMaterialInstanceDynamic* P = UMaterialInstanceDynamic::Create(Basic, this);
		P->SetVectorParameterValue(TEXT("Color"), FLinearColor(.08f, .08f, .1f));
		Pedestal->SetMaterial(0, P);
	}
	// glowing ring + pool of light under the hero
	UMaterialInstanceDynamic* M = nullptr;
	Ring = Plane(TEXT("T_Ring"), true, M); RingMat = M;
	Ring->SetRelativeLocation(FVector(0, 0, -81)); Ring->SetRelativeScale3D(FVector(3.4f));
	FloorGlow = Plane(TEXT("T_SoftGlow"), true, M); GlowMat = M;
	FloorGlow->SetRelativeLocation(FVector(0, 0, -82)); FloorGlow->SetRelativeScale3D(FVector(9.f));
	// rim glow shell on the hero
	if (UMaterialInterface* G = LoadMat(TEXT("/Game/DrawFight/FX/M_Glow.M_Glow")))
	{
		RimMat = UMaterialInstanceDynamic::Create(G, this);
		RimMat->SetScalarParameterValue(TEXT("Glow"), .6f);
		RimMat->SetScalarParameterValue(TEXT("Opacity"), .35f);
		Hero->SetOverlayMaterial(RimMat);
	}
	// floating motes
	for (int32 I = 0; I < 40; ++I)
	{
		FMote Mo;
		Mo.Mesh = Plane(I % 4 ? TEXT("T_SoftGlow") : TEXT("T_Star"), false, Mo.Mat);
		Mo.Base = FVector(FMath::FRandRange(-500.f, 500.f), FMath::FRandRange(-700.f, 700.f), FMath::FRandRange(-90.f, 300.f));
		Mo.Phase = FMath::FRandRange(0.f, 10.f); Mo.Speed = FMath::FRandRange(15.f, 45.f); Mo.Size = FMath::FRandRange(.08f, .22f);
		Motes.Add(Mo);
	}
	ShowHero(NAME_None, Color);
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this))
		ShowHero(Prof->GetSelected(), Prof->GetPowerColor());
}

void ADFMenuStage::ShowHero(FName Id, FLinearColor C)
{
	Color = C;
	if (UDFProfileSubsystem* Prof = UDFProfileSubsystem::Get(this))
	{
		FDFCharacterRow R;
		if (!Id.IsNone() && Prof->GetCharacter(Id, R))
		{
			USkeletalMesh* SM = R.Mesh.IsNull() ? nullptr : R.Mesh.LoadSynchronous();
			if (SM) Hero->SetSkeletalMesh(SM);
			else Hero->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, StageManny));
			Hero->SetRelativeScale3D(FVector(FMath::Clamp(R.SizeMult, .7f, 1.4f)));
		}
	}
	if (RingMat) { RingMat->SetVectorParameterValue(TEXT("Color"), C); RingMat->SetScalarParameterValue(TEXT("Glow"), 5.f); }
	if (GlowMat) { GlowMat->SetVectorParameterValue(TEXT("Color"), C); GlowMat->SetScalarParameterValue(TEXT("Glow"), .8f); }
	if (RimMat) RimMat->SetVectorParameterValue(TEXT("Color"), C);
	for (FMote& Mo : Motes)
		if (Mo.Mat) { Mo.Mat->SetVectorParameterValue(TEXT("Color"), FMath::RandBool() ? C : FLinearColor(1.f, .85f, .6f)); Mo.Mat->SetScalarParameterValue(TEXT("Glow"), 3.f); }
	HeroLight->SetLightColor(C);
}

void ADFMenuStage::Tick(float Dt)
{
	Super::Tick(Dt);
	Age += Dt;
	Focus = FMath::FInterpTo(Focus, TargetFocus, Dt, 3.f);

	// camera: hero sits right of centre so the menu can live on the left
	const FVector Far(-640.f, -150.f, 75.f), Near(-380.f, -95.f, 55.f);
	const FVector CamPos = FMath::Lerp(Far, Near, Focus) + FVector(0, FMath::Sin(Age * .25f) * 12.f, FMath::Sin(Age * .4f) * 5.f);
	const FVector Look = FMath::Lerp(FVector(0, -135.f, 35.f), FVector(0, -85.f, 40.f), Focus);
	Camera->SetRelativeLocation(CamPos);
	Camera->SetRelativeRotation((Look - CamPos).Rotation());

	Hero->SetRelativeRotation(FRotator(0, 100.f + FMath::Sin(Age * .35f) * 35.f, 0));
	const float Pulse = .8f + .2f * FMath::Sin(Age * 2.2f);
	if (RingMat) RingMat->SetScalarParameterValue(TEXT("Opacity"), Pulse);
	Ring->SetRelativeRotation(FRotator(0, Age * 20.f, 0));

	const FVector CamW = Camera->GetComponentLocation();
	for (FMote& Mo : Motes)
	{
		float Z = FMath::Fmod(Mo.Base.Z + 90.f + (Age + Mo.Phase) * Mo.Speed, 420.f) - 90.f;
		const FVector P = GetActorLocation() + FVector(Mo.Base.X + FMath::Sin(Age * .6f + Mo.Phase) * 30.f, Mo.Base.Y, Z);
		Mo.Mesh->SetWorldLocation(P);
		Mo.Mesh->SetWorldRotation(FRotationMatrix::MakeFromZ(CamW - P).Rotator());
		Mo.Mesh->SetWorldScale3D(FVector(Mo.Size));
		if (Mo.Mat) Mo.Mat->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(FMath::Min(Z + 90.f, 330.f - Z) / 80.f, 0.f, 1.f));
	}
}
