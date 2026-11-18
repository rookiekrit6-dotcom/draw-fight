#include "DFMenuGameMode.h"
#include "DFMenuWidget.h"
#include "Blueprint/UserWidget.h"

void ADFMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
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
