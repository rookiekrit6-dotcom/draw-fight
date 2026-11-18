// DFMenuGameMode.h — set this as the GameMode Override of an empty "MainMenu" level.
// It shows UDFMenuWidget with the mouse cursor and no player pawn.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "DFMenuGameMode.generated.h"

class UDFMenuWidget;
class ADFMenuStage;

UCLASS()
class ADFMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	// Leave empty to use the code-only menu; set a Blueprint child to change font / arena level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu") TSubclassOf<UDFMenuWidget> MenuClass;
	UPROPERTY(BlueprintReadOnly, Category = "Menu") TObjectPtr<UDFMenuWidget> Menu;
	UPROPERTY(BlueprintReadOnly, Category = "Menu") TObjectPtr<ADFMenuStage> Stage;
protected:
	virtual void BeginPlay() override;
};

UCLASS()
class ADFMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADFMenuGameMode();
};
