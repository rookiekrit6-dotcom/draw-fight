#include "DFFighterCharacter.h"
#include "Misc/App.h"
#include "DFCombatComponent.h"
#include "DFAutoPilotComponent.h"
#include "DFProfileSubsystem.h"
#include "DFVoiceSubsystem.h"
#include "DFSkillFX.h"
#include "Engine/Engine.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Components/InputComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* ManPath   = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	const TCHAR* QuinnPath = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple");
	const TCHAR* AnimBP    = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");
	const TCHAR* Atk[3] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03") };
	const TCHAR* DashPath  = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jump/MM_Dash.MM_Dash");
}

// ===================== shared fighter =====================
ADFFighterCharacter::ADFFighterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	Combat = CreateDefaultSubobject<UDFCombatComponent>(TEXT("Combat"));
	AutoPilot = CreateDefaultSubobject<UDFAutoPilotComponent>(TEXT("AutoPilot"));

	UCharacterMovementComponent* M = GetCharacterMovement();
	M->bOrientRotationToMovement = true;
	M->RotationRate = FRotator(0.f, 720.f, 0.f);
	M->MaxWalkSpeed = 520.f;
	M->JumpZVelocity = 620.f;
	M->AirControl = .4f;
	bUseControllerRotationYaw = false;

	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -96.f), FRotator(0, -90.f, 0));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshF(ManPath);
	if (MeshF.Succeeded()) GetMesh()->SetSkeletalMesh(MeshF.Object);
	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimF(AnimBP);
	if (AnimF.Succeeded()) GetMesh()->SetAnimInstanceClass(AnimF.Class);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> A0(Atk[0]);
	static ConstructorHelpers::FObjectFinder<UAnimSequence> A1(Atk[1]);
	static ConstructorHelpers::FObjectFinder<UAnimSequence> A2(Atk[2]);
	static ConstructorHelpers::FObjectFinder<UAnimSequence> D(DashPath);
	if (A0.Succeeded()) AttackAnims.Add(A0.Object);
	if (A1.Succeeded()) AttackAnims.Add(A1.Object);
	if (A2.Succeeded()) AttackAnims.Add(A2.Object);
	if (D.Succeeded()) DashAnim = D.Object;
}

void ADFFighterCharacter::BeginPlay()
{
	Super::BeginPlay();
	Combat->OnAttackStarted.AddUniqueDynamic(this, &ADFFighterCharacter::HandleAttack);
	Combat->OnDashed.AddUniqueDynamic(this, &ADFFighterCharacter::HandleDash);
	Combat->OnDied.AddUniqueDynamic(this, &ADFFighterCharacter::HandleDied);
}

void ADFFighterCharacter::PlayClip(UAnimSequence* Clip, float Rate)
{
	UAnimInstance* AI = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (AI && Clip) AI->PlaySlotAnimationAsDynamicMontage(Clip, TEXT("DefaultSlot"), .05f, .15f, Rate);
}

void ADFFighterCharacter::HandleAttack(UDFCombatComponent* Self)
{
	if (!AttackAnims.Num()) return;
	PlayClip(AttackAnims[ComboIndex % AttackAnims.Num()], 1.6f);
	ComboIndex++;
}

void ADFFighterCharacter::HandleDash(UDFCombatComponent* Self)
{
	PlayClip(DashAnim, 1.4f);
}

void ADFFighterCharacter::HandleDied(UDFCombatComponent* Self)
{
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);
	AutoPilot->bEnabled = false;
}

// ===================== player =====================
ADFPlayerCharacter::ADFPlayerCharacter()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = TargetZoom;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;
	CameraBoom->SocketOffset = FVector(0, 0, 120.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 80.f;
	// strong bloom so glowing skills bleed light like anime VFX
	{
		FPostProcessSettings& PP = FollowCamera->PostProcessSettings;
		PP.bOverride_BloomIntensity = true; PP.BloomIntensity = 2.2f;
		PP.bOverride_BloomThreshold = true; PP.BloomThreshold = 0.8f;
		PP.bOverride_LensFlareIntensity = true; PP.LensFlareIntensity = 0.6f;
		PP.bOverride_VignetteIntensity = true; PP.VignetteIntensity = 0.45f;
		FollowCamera->PostProcessBlendWeight = 1.f;
	}

	AutoPilot->bEnabled = false;   // player starts manual; T toggles
	AutoPilot->bDodge = true;
	AutoPilot->bUseSkills = true;
	AutoPossessPlayer = EAutoReceiveInput::Disabled; // GameMode possesses us
}

void ADFPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	AutoPilot->OnWantsSkill.AddUniqueDynamic(this, &ADFPlayerCharacter::HandleAutoSkill);
	AutoPilot->SkillReady.Init(false, 3);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FInputModeGameOnly Mode;
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = false;
		if (PC->PlayerCameraManager) { PC->PlayerCameraManager->ViewPitchMin = -65.f; PC->PlayerCameraManager->ViewPitchMax = 20.f; }
	}
}

void ADFPlayerCharacter::SetupPlayerInputComponent(UInputComponent* IC)
{
	Super::SetupPlayerInputComponent(IC);
	if (!IC) return;
	const TPair<FKey, void (ADFPlayerCharacter::*)()> Binds[] = {
		{ EKeys::One, &ADFPlayerCharacter::KeySkill1 }, { EKeys::Two, &ADFPlayerCharacter::KeySkill2 },
		{ EKeys::Three, &ADFPlayerCharacter::KeySkill3 }, { EKeys::R, &ADFPlayerCharacter::KeyUltimate },
		{ EKeys::NumPadOne, &ADFPlayerCharacter::KeySkill1 }, { EKeys::NumPadTwo, &ADFPlayerCharacter::KeySkill2 },
		{ EKeys::NumPadThree, &ADFPlayerCharacter::KeySkill3 },
		{ EKeys::Gamepad_LeftShoulder, &ADFPlayerCharacter::KeySkill1 }, { EKeys::Gamepad_RightShoulder, &ADFPlayerCharacter::KeySkill2 },
		{ EKeys::Gamepad_FaceButton_Top, &ADFPlayerCharacter::KeySkill3 }, { EKeys::Gamepad_RightTrigger, &ADFPlayerCharacter::KeyUltimate },
		{ EKeys::LeftMouseButton, &ADFPlayerCharacter::KeyAttack }, { EKeys::J, &ADFPlayerCharacter::KeyAttack },
		{ EKeys::Gamepad_FaceButton_Left, &ADFPlayerCharacter::KeyAttack },
		{ EKeys::LeftShift, &ADFPlayerCharacter::KeyDash }, { EKeys::Gamepad_FaceButton_Right, &ADFPlayerCharacter::KeyDash },
		{ EKeys::SpaceBar, &ADFPlayerCharacter::KeyJump }, { EKeys::Gamepad_FaceButton_Bottom, &ADFPlayerCharacter::KeyJump },
		{ EKeys::K, &ADFPlayerCharacter::KeyTestNext }, { EKeys::L, &ADFPlayerCharacter::KeyTestPrev },
		{ EKeys::T, &ADFPlayerCharacter::KeyAuto }, { EKeys::Gamepad_Special_Left, &ADFPlayerCharacter::KeyAuto },
		{ EKeys::Escape, &ADFPlayerCharacter::KeyMenu }, { EKeys::Gamepad_Special_Right, &ADFPlayerCharacter::KeyMenu } };
	for (const auto& B : Binds) IC->BindKey(B.Key, IE_Pressed, this, B.Value);

	// movement keys as held flags (event-driven, works with Enhanced Input projects)
	auto Hold = [this, IC](FKey Key, bool ADFPlayerCharacter::* Flag)
	{
		for (EInputEvent Ev : { IE_Pressed, IE_Released })
		{
			FInputKeyBinding KB(FInputChord(Key), Ev);
			KB.bConsumeInput = false;
			KB.KeyDelegate.GetDelegateForManualSet().BindLambda([this, Flag, Ev]() { this->*Flag = (Ev == IE_Pressed); });
			IC->KeyBindings.Add(KB);
		}
	};
	Hold(EKeys::W, &ADFPlayerCharacter::bKeyW); Hold(EKeys::Up, &ADFPlayerCharacter::bKeyW);
	Hold(EKeys::S, &ADFPlayerCharacter::bKeyS); Hold(EKeys::Down, &ADFPlayerCharacter::bKeyS);
	Hold(EKeys::A, &ADFPlayerCharacter::bKeyA); Hold(EKeys::Left, &ADFPlayerCharacter::bKeyA);
	Hold(EKeys::D, &ADFPlayerCharacter::bKeyD); Hold(EKeys::Right, &ADFPlayerCharacter::bKeyD);
	for (FKey K : { EKeys::MouseScrollUp, EKeys::MouseScrollDown })
	{
		FInputKeyBinding KB(FInputChord(K), IE_Pressed);
		const float Step = K == EKeys::MouseScrollUp ? -120.f : 120.f;
		KB.KeyDelegate.GetDelegateForManualSet().BindLambda([this, Step]() { TargetZoom = FMath::Clamp(TargetZoom + Step, MinZoom, MaxZoom); });
		IC->KeyBindings.Add(KB);
	}
}

bool ADFPlayerCharacter::CanAct() const
{
	return Combat && !Combat->IsDead() && !AutoPilot->bEnabled;
}

void ADFPlayerCharacter::KeyAttack()
{
	if (!CanAct()) return;
	if (UDFCombatComponent* E = Combat->FindNearestEnemy(Combat->Reach * 2.f))
	{
		FVector To = E->GetOwner()->GetActorLocation() - GetActorLocation(); To.Z = 0;
		if (!To.IsNearlyZero()) SetActorRotation(To.Rotation());
	}
	Combat->TryAttack();
}

void ADFPlayerCharacter::KeyDash()
{
	if (!CanAct()) return;
	const FVector V = GetLastMovementInputVector();
	Combat->TryDash(V.IsNearlyZero() ? GetActorForwardVector() : V);
}

void ADFPlayerCharacter::KeyJump() { if (CanAct()) Jump(); }
void ADFPlayerCharacter::KeyAuto() { if (Combat && !Combat->IsDead()) AutoPilot->Toggle(); }
void ADFPlayerCharacter::KeyMenu() { UGameplayStatics::OpenLevel(this, TEXT("MainMenu")); }

void ADFPlayerCharacter::CycleTest(int32 Step)
{
	UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this);
	if (!P) return;
	const TArray<FName> All = P->GetAllSkillIds();
	if (!All.Num()) return;
	TestIdx = (TestIdx + Step + All.Num()) % All.Num();
	TestSkill = All[TestIdx];
	CdLeft[0] = 0.f;
	FDFSkillRow Row;
	const FString Th = P->GetSkill(TestSkill, Row) ? Row.NameTH : FString();
	if (GEngine) GEngine->AddOnScreenDebugMessage(77, 4.f, FColor::Cyan, FString::Printf(TEXT("[ทดสอบสกิล %d/%d] %s %s — กด 1"), TestIdx + 1, All.Num(), *TestSkill.ToString(), *Th));
}

FName ADFPlayerCharacter::SlotSkill(int32 Slot) const
{
	if (Slot == 0 && !TestSkill.IsNone()) return TestSkill;
	UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this);
	if (!P) return NAME_None;
	if (Slot == 3) return P->GetUltimate();
	const TArray<FName> L = P->GetLoadout(P->GetSelected());
	return L.IsValidIndex(Slot) ? L[Slot] : NAME_None;
}

float ADFPlayerCharacter::SlotCooldownLeft(int32 Slot) const { return (Slot >= 0 && Slot < 4) ? CdLeft[Slot] : 0.f; }
float ADFPlayerCharacter::SlotCooldownTotal(int32 Slot) const { return (Slot >= 0 && Slot < 4) ? CdTotal[Slot] : 1.f; }

static void DFDebug(const FString& Msg)
{
	UE_LOG(LogTemp, Display, TEXT("DrawFight skill: %s"), *Msg);
	// on-screen debug off (log only)
}

bool ADFPlayerCharacter::TryCastSlot(int32 Slot)
{
	if (Slot < 0 || Slot > 3 || !Combat) return false;
	if (CdLeft[Slot] > 0.f) { DFDebug(FString::Printf(TEXT("slot %d cooldown %.1f"), Slot + 1, CdLeft[Slot])); return false; }
	if (Combat->IsDead() || Combat->IsStunned()) { DFDebug(TEXT("dead or stunned")); return false; }
	const FName Skill = SlotSkill(Slot);
	if (Skill.IsNone()) { DFDebug(FString::Printf(TEXT("slot %d is empty"), Slot + 1)); return false; }
	DFDebug(FString::Printf(TEXT("cast slot %d: %s"), Slot + 1, *Skill.ToString()));

	float Cd = Slot == 3 ? 40.f : 5.f;
	FDFSkillRow Row;
	if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
		if (P->GetSkill(Skill, Row)) Cd = FMath::Max(1.5f, Row.Cooldown);
	if (Slot == 0 && !TestSkill.IsNone()) Cd = 1.f; // test mode: fast repeat
	CdLeft[Slot] = CdTotal[Slot] = Cd;

	OnSkillCast.Broadcast(Slot, Skill);
	if (UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this))
		if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
			V->Play(this, P->GetSelected(), Slot == 3 ? EDFVoiceEvent::Ultimate : EDFVoiceEvent::Skill);

	if (bPlaceholderSkills)
	{
		// code-built visuals + hits until the Niagara versions exist
		FLinearColor Col(1.f, .45f, .15f);
		int32 Lv = 1;
		if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
		{
			Col = P->GetPowerColor();
			Lv = FMath::Max(1, P->GetSkillLevel(Skill));
		}
		ADFSkillFX::CastSkill(this, Skill, Lv, Col, (Slot == 3 || Skill.ToString().StartsWith(TEXT("ult_"))) ? 2.5f : 1.f);
		Combat->TryAttack(); // swing animation
	}
	return true;
}

void ADFPlayerCharacter::HandleAutoSkill(int32 Slot)
{
	TryCastSlot(Slot);
}

void ADFPlayerCharacter::Tick(float Dt)
{
	Super::Tick(Dt);
	for (int32 I = 0; I < 4; ++I) CdLeft[I] = FMath::Max(0.f, CdLeft[I] - Dt);
	if (AutoPilot->SkillReady.Num() >= 3)
		for (int32 I = 0; I < 3; ++I) AutoPilot->SkillReady[I] = CdLeft[I] <= 0.f && !SlotSkill(I).IsNone();
	ReadInput(Dt);
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetZoom, Dt, 8.f);
	const float RealDt = FApp::GetDeltaTime();
	Shake = FMath::Max(0.f, Shake - RealDt * (25.f + Shake * 6.f));
	FollowCamera->SetRelativeLocation(Shake > .1f ? FVector(0, FMath::FRandRange(-Shake, Shake), FMath::FRandRange(-Shake, Shake)) : FVector::ZeroVector);
}

void ADFPlayerCharacter::ReadInput(float Dt)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !Combat || Combat->IsDead()) return;

	// camera (works even in auto mode)
	float Mx = 0.f, My = 0.f;
	PC->GetInputMouseDelta(Mx, My);
	Mx += PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX) * 2.5f;
	My += PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY) * 2.5f;
	AddControllerYawInput(Mx * MouseSensitivity);
	AddControllerPitchInput(-My * MouseSensitivity);

	if (AutoPilot->bEnabled) return; // the AI drives the body

	// movement relative to the camera
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Fwd = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	float F = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY), R = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	if (bKeyW) F += 1.f;
	if (bKeyS) F -= 1.f;
	if (bKeyD) R += 1.f;
	if (bKeyA) R -= 1.f;
	const FVector Dir = (Fwd * F + Right * R).GetClampedToMaxSize(1.f);
	if (!Combat->IsBusy()) AddMovementInput(Dir);

}

// ===================== enemy =====================
ADFEnemyCharacter::ADFEnemyCharacter()
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshF(QuinnPath);
	if (MeshF.Succeeded()) GetMesh()->SetSkeletalMesh(MeshF.Object);
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	AutoPilot->bEnabled = true;
	AutoPilot->bDodge = false;
	AutoPilot->bUseSkills = false;
}
