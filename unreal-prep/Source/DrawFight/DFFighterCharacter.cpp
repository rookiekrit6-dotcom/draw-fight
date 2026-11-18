#include "DFFighterCharacter.h"
#include "DFCombatComponent.h"
#include "DFAutoPilotComponent.h"
#include "DFProfileSubsystem.h"
#include "DFVoiceSubsystem.h"
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
	}
}

FName ADFPlayerCharacter::SlotSkill(int32 Slot) const
{
	UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this);
	if (!P) return NAME_None;
	if (Slot == 3) return P->GetUltimate();
	const TArray<FName> L = P->GetLoadout(P->GetSelected());
	return L.IsValidIndex(Slot) ? L[Slot] : NAME_None;
}

float ADFPlayerCharacter::SlotCooldownLeft(int32 Slot) const { return (Slot >= 0 && Slot < 4) ? CdLeft[Slot] : 0.f; }
float ADFPlayerCharacter::SlotCooldownTotal(int32 Slot) const { return (Slot >= 0 && Slot < 4) ? CdTotal[Slot] : 1.f; }

bool ADFPlayerCharacter::TryCastSlot(int32 Slot)
{
	if (Slot < 0 || Slot > 3 || CdLeft[Slot] > 0.f || !Combat || Combat->IsDead() || Combat->IsStunned()) return false;
	const FName Skill = SlotSkill(Slot);
	if (Skill.IsNone()) return false;

	float Cd = Slot == 3 ? 40.f : 5.f;
	FDFSkillRow Row;
	if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
		if (P->GetSkill(Skill, Row)) Cd = FMath::Max(1.5f, Row.Cooldown);
	CdLeft[Slot] = CdTotal[Slot] = Cd;

	OnSkillCast.Broadcast(Slot, Skill);
	if (UDFVoiceSubsystem* V = UDFVoiceSubsystem::Get(this))
		if (UDFProfileSubsystem* P = UDFProfileSubsystem::Get(this))
			V->Play(this, P->GetSelected(), Slot == 3 ? EDFVoiceEvent::Ultimate : EDFVoiceEvent::Skill);

	if (bPlaceholderSkills)
	{
		// stand-in until the real skill runner: damage everything nearby
		const float Radius = Slot == 3 ? 900.f : 450.f;
		const float Dmg = Combat->Damage * (Slot == 3 ? 5.f : 2.2f);
		for (UDFCombatComponent* O : UDFCombatComponent::Registry())
			if (O && O != Combat && O->Team != Combat->Team && !O->IsDead() && O->GetOwner()
				&& FVector::Dist2D(O->GetOwner()->GetActorLocation(), GetActorLocation()) < Radius)
				O->ApplyDamage(Dmg, this);
		Combat->TryAttack(); // reuse the swing animation
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
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp)) TargetZoom = FMath::Max(MinZoom, TargetZoom - 120.f);
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown)) TargetZoom = FMath::Min(MaxZoom, TargetZoom + 120.f);

	if (PC->WasInputKeyJustPressed(EKeys::T) || PC->WasInputKeyJustPressed(EKeys::Gamepad_Special_Left)) AutoPilot->Toggle();
	if (PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(EKeys::Gamepad_Special_Right))
	{
		UGameplayStatics::OpenLevel(this, TEXT("MainMenu"));
		return;
	}
	if (AutoPilot->bEnabled) return; // the AI drives the body

	// movement relative to the camera
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Fwd = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	float F = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY), R = PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	if (PC->IsInputKeyDown(EKeys::W)) F += 1.f;
	if (PC->IsInputKeyDown(EKeys::S)) F -= 1.f;
	if (PC->IsInputKeyDown(EKeys::D)) R += 1.f;
	if (PC->IsInputKeyDown(EKeys::A)) R -= 1.f;
	const FVector Dir = (Fwd * F + Right * R).GetClampedToMaxSize(1.f);
	if (!Combat->IsBusy()) AddMovementInput(Dir);

	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton) || PC->WasInputKeyJustPressed(EKeys::J)
		|| PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Left))
	{
		// face the nearest enemy in reach so melee feels sticky
		if (UDFCombatComponent* E = Combat->FindNearestEnemy(Combat->Reach * 2.f))
		{
			FVector To = E->GetOwner()->GetActorLocation() - GetActorLocation(); To.Z = 0;
			if (!To.IsNearlyZero()) SetActorRotation(To.Rotation());
		}
		Combat->TryAttack();
	}
	if (PC->WasInputKeyJustPressed(EKeys::LeftShift) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
		Combat->TryDash(Dir.IsNearlyZero() ? GetActorForwardVector() : Dir);
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)) Jump();

	if (PC->WasInputKeyJustPressed(EKeys::One) || PC->WasInputKeyJustPressed(EKeys::Gamepad_LeftShoulder)) TryCastSlot(0);
	if (PC->WasInputKeyJustPressed(EKeys::Two) || PC->WasInputKeyJustPressed(EKeys::Gamepad_RightShoulder)) TryCastSlot(1);
	if (PC->WasInputKeyJustPressed(EKeys::Three) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top)) TryCastSlot(2);
	if (PC->WasInputKeyJustPressed(EKeys::R) || PC->WasInputKeyJustPressed(EKeys::Gamepad_RightTrigger)) TryCastSlot(3);
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
