// DFArenaDressing.h — builds one of the five themed arenas in code on top of any level:
// hides the level's own meshes, lays a floor, walls/props, glowing details, lights and fog.
// Themes: lab (ห้องวิจัย), white (ห้องสีขาว), sky (ลานกลางเวหา), volcano (ลานภูเขาไฟ), ring (ลานมวย).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DFArenaDressing.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class ADFArenaDressing : public AActor
{
	GENERATED_BODY()
public:
	ADFArenaDressing();
	static ADFArenaDressing* Build(UWorld* World, FName Theme, float Radius);
	static TArray<FName> Themes();
	static FString ThemeNameTH(FName Theme);
	static FLinearColor ThemeColor(FName Theme);
	virtual void Tick(float Dt) override;

private:
	struct FAnim { UStaticMeshComponent* Mesh; FVector Base; float Phase, Bob, Spin, Rise; bool bBill; UMaterialInstanceDynamic* Mat; };
	TArray<FAnim> Anims;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Keep;
	float Age = 0.f;
	FName Theme;

	void Make(float Radius, float FloorZ);
	UStaticMeshComponent* Solid(int32 Shape, FVector Loc, FRotator Rot, FVector Scale, FLinearColor Color, bool bCollide);
	UStaticMeshComponent* Glow(const TCHAR* Tex, FVector Loc, FRotator Rot, FVector Scale, FLinearColor Color, float Glow, bool bDust = false);
	void Mood(FLinearColor SunColor, float SunIntensity, float SunPitch, float SkyIntensity, FLinearColor FogColor, float FogDensity, float FogFalloff);
	void Embers(int32 Count, float Radius, float FloorZ, FLinearColor C, const TCHAR* Tex);
};
