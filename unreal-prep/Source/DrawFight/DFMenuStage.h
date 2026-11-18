// DFMenuStage.h — the 3D backdrop of the menus: sky, fog, a glowing pedestal and the player's hero
// slowly turning on it. Spawned by ADFMenuPlayerController in the (empty) MainMenu level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFMenuStage.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;

UCLASS()
class ADFMenuStage : public AActor
{
	GENERATED_BODY()
public:
	ADFMenuStage();
	virtual void Tick(float Dt) override;

	// hero on the pedestal (None = mannequin) and the colour of its glow
	void ShowHero(FName Hero, FLinearColor Color);
	// where the camera looks: 0 = hero centred-right (lobby), 1 = hero close-up (creation)
	void SetFocus(float NewFocus) { TargetFocus = NewFocus; }

	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Hero;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Floor;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Pedestal;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> HeroLight;

protected:
	virtual void BeginPlay() override;

private:
	struct FMote { UStaticMeshComponent* Mesh; UMaterialInstanceDynamic* Mat; FVector Base; float Phase, Speed, Size; };
	TArray<FMote> Motes;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Keep;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RingMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowMat;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RimMat;
	UStaticMeshComponent* Ring = nullptr;
	UStaticMeshComponent* FloorGlow = nullptr;
	FLinearColor Color = FLinearColor(1.f, .45f, .15f);
	float Age = 0.f, Focus = 0.f, TargetFocus = 0.f;
	UStaticMeshComponent* Plane(const TCHAR* Tex, bool bFlat, UMaterialInstanceDynamic*& OutMat);
};
