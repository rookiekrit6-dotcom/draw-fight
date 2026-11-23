#include "DFMenuGameMode.h"
#include "DFMenuWidget.h"
#include "DFMenuStage.h"
#include "DFAudio.h"
#include "Blueprint/UserWidget.h"

void ADFMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	// 3D backdrop with the hero on a pedestal
	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Stage = GetWorld()->SpawnActor<ADFMenuStage>(ADFMenuStage::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SP);
	bAutoManageActiveCameraTarget = false;
	if (Stage) SetViewTarget(Stage);
	DFAudio::Music(this, TEXT("M_Menu"));
	UClass* C = MenuClass ? MenuClass.Get() : UDFMenuWidget::StaticClass();
	Menu = CreateWidget<UDFMenuWidget>(this, C);
	if (Menu) Menu->AddToViewport();
	FInputModeUIOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = true;
}

ADFMenuGameMode::ADFMenuGameMode()
{
	PlayerControllerClass = ADFMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}
