#include "DFSkillPack.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"

const TSet<FName>& UDFSkillPackLib::KnownSteps()
{
	static const TSet<FName> S = {
		"burst","ring","shot","area","fall","beam","dash","heal","shake","sound","sprite",
		"circle","pillar","portal","buff","stun","pull","blink","orb" };
	return S;
}

static float ClampNum(const TSharedPtr<FJsonValue>& V, float Def, float Min, float Max)
{
	double D = 0.0;
	if (!V.IsValid()) return Def;
	if (V->Type == EJson::Number) D = V->AsNumber();
	else if (V->Type == EJson::String) { if (!LexTryParseString(D, *V->AsString())) return Def; }
	else return Def;
	if (!FMath::IsFinite(D)) return Def;
	return FMath::Clamp((float)D, Min, Max);
}

static FString ValueToString(const TSharedPtr<FJsonValue>& V)
{
	switch (V->Type)
	{
	case EJson::String: return V->AsString().Left(64);
	case EJson::Number: return FString::SanitizeFloat(V->AsNumber());
	case EJson::Boolean: return V->AsBool() ? TEXT("true") : TEXT("false");
	default: return FString(); // objects/arrays are not valid step params
	}
}

static bool IsHexColor(const FString& S)
{
	if (S.Len() != 7 || S[0] != '#') return false;
	for (int32 I = 1; I < 7; ++I) if (!FChar::IsHexDigit(S[I])) return false;
	return true;
}

bool UDFSkillPackLib::ParseSkillPack(const FString& Json, FDFSkillPack& Out, FString& Error)
{
	Out = FDFSkillPack();
	TSharedPtr<FJsonObject> O;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(R, O) || !O.IsValid()) { Error = TEXT("ไฟล์นี้ไม่ใช่ข้อความ JSON ที่อ่านได้"); return false; }

	FString Format;
	const TArray<TSharedPtr<FJsonValue>>* StepsIn = nullptr;
	if (!O->TryGetStringField(TEXT("format"), Format) || Format != TEXT("dfskill") || !O->TryGetArrayField(TEXT("steps"), StepsIn))
	{
		Error = TEXT("ไม่ใช่ไฟล์สกิลของเกมนี้");
		return false;
	}

	FString S;
	Out.Name = O->TryGetStringField(TEXT("name"), S) && !S.IsEmpty() ? S.Left(24) : TEXT("สกิลไม่มีชื่อ");
	Out.Author = O->TryGetStringField(TEXT("author"), S) ? S.Left(24) : FString();
	if (O->TryGetStringField(TEXT("color"), S) && IsHexColor(S)) Out.Color = FLinearColor(FColor::FromHex(S));
	Out.DeclaredCooldown = ClampNum(O->TryGetField(TEXT("cd")), 4.f, 0.f, 120.f);

	// images: max 4, data URLs only, < 2.2 MB each
	const TSharedPtr<FJsonObject>* Img = nullptr;
	if (O->TryGetObjectField(TEXT("images"), Img))
		for (const auto& KV : (*Img)->Values)
		{
			if (Out.ImagesBase64.Num() >= 4 || KV.Value->Type != EJson::String) continue;
			const FString U = KV.Value->AsString();
			if (U.Len() < 2200000 && (U.StartsWith(TEXT("data:image/png;base64,")) || U.StartsWith(TEXT("data:image/jpeg;base64,")) || U.StartsWith(TEXT("data:image/webp;base64,"))))
				Out.ImagesBase64.Add(FString(*KV.Key).Left(40), U);
		}

	// steps: max 40, unknown "do" dropped
	for (int32 I = 0; I < StepsIn->Num() && I < 40; ++I)
	{
		const TSharedPtr<FJsonObject>* Q = nullptr;
		if (!(*StepsIn)[I]->TryGetObject(Q)) continue;
		FString Do;
		if (!(*Q)->TryGetStringField(TEXT("do"), Do) || !KnownSteps().Contains(FName(*Do))) continue;
		FDFSkillStep St;
		St.Do = FName(*Do);
		St.At = ClampNum((*Q)->TryGetField(TEXT("at")), 0.f, 0.f, 6.f);
		for (const auto& KV : (*Q)->Values)
		{
			if (KV.Key == TEXT("do") || KV.Key == TEXT("at")) continue;
			const FString V = ValueToString(KV.Value);
			if (!V.IsEmpty()) St.Params.Add(FName(*FString(*KV.Key).Left(16)), V);
		}
		Out.Steps.Add(St);
	}
	if (Out.Steps.Num() == 0) { Error = TEXT("ไฟล์สกิลไม่มีขั้นตอนที่เกมรู้จัก"); return false; }
	Out.Steps.StableSort([](const FDFSkillStep& A, const FDFSkillStep& B) { return A.At < B.At; });

	// bone animation
	const TSharedPtr<FJsonObject>* Anim = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Keys = nullptr;
	if (O->TryGetObjectField(TEXT("anim"), Anim) && (*Anim)->TryGetArrayField(TEXT("keys"), Keys) && Keys->Num() > 0)
	{
		Out.bHasAnim = true;
		Out.AnimDuration = ClampNum((*Anim)->TryGetField(TEXT("dur")), .6f, .15f, 3.f);
		for (int32 I = 0; I < Keys->Num() && I < 24; ++I)
		{
			FDFSkillAnimKey K;
			const TSharedPtr<FJsonObject>* KO = nullptr;
			if ((*Keys)[I]->TryGetObject(KO))
			{
				K.T = ClampNum((*KO)->TryGetField(TEXT("t")), 0.f, 0.f, 1.f);
				const TSharedPtr<FJsonObject>* Rot = nullptr;
				if ((*KO)->TryGetObjectField(TEXT("r"), Rot))
					for (const auto& B : (*Rot)->Values)
					{
						const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
						if (!B.Value->TryGetArray(A)) continue;
						auto At = [&](int32 J) { return J < A->Num() ? ClampNum((*A)[J], 0.f, -360.f, 360.f) : 0.f; };
						// web order is [x, y, z] Euler degrees -> Pitch=x, Yaw=y, Roll=z
						K.Bones.Add(FName(*FString(*B.Key).Left(12)), FRotator(At(0), At(1), At(2)));
					}
			}
			Out.AnimKeys.Add(K);
		}
		Out.AnimKeys.StableSort([](const FDFSkillAnimKey& A, const FDFSkillAnimKey& B) { return A.T < B.T; });
	}
	return true;
}

bool UDFSkillPackLib::LoadSkillPackFile(const FString& Path, FDFSkillPack& Out, FString& Error)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path)) { Error = TEXT("เปิดไฟล์ไม่ได้"); return false; }
	return ParseSkillPack(Text, Out, Error);
}

float UDFSkillPackLib::Num(const FDFSkillStep& Step, FName Key, float Default, float Min, float Max)
{
	const FString* V = Step.Params.Find(Key);
	double D = 0.0;
	if (!V || !LexTryParseString(D, **V) || !FMath::IsFinite(D)) return Default;
	return FMath::Clamp((float)D, Min, Max);
}

FString UDFSkillPackLib::Str(const FDFSkillStep& Step, FName Key, const FString& Default)
{
	const FString* V = Step.Params.Find(Key);
	return V ? *V : Default;
}

bool UDFSkillPackLib::Bool(const FDFSkillStep& Step, FName Key)
{
	const FString* V = Step.Params.Find(Key);
	return V && (*V == TEXT("true") || *V == TEXT("1"));
}

int32 UDFSkillPackLib::Power(const FDFSkillPack& P)
{
	float Sum = 0.f;
	for (const FDFSkillStep& Q : P.Steps)
	{
		const FName D = Q.Do;
		if (D == "shot") Sum += Num(Q, "n", 1, 1, 24) * Num(Q, "dmg", 8, 0, 999) * (Bool(Q, "pierce") ? 1.6f : 1.f);
		else if (D == "area") Sum += Num(Q, "dmg", 10, 0, 999) * 1.6f;
		else if (D == "fall") Sum += Num(Q, "n", 6, 1, 30) * Num(Q, "dmg", 10, 0, 999) * .5f;
		else if (D == "beam") Sum += Num(Q, "dmg", 12, 0, 999) * 1.6f;
		else if (D == "pillar" || D == "portal") Sum += Num(Q, "dmg", 20, 0, 999) * 1.6f;
		else if (D == "stun") Sum += Num(Q, "dur", .8f, 0, 3) * 8.f;
		else if (D == "buff") Sum += Num(Q, "dur", 3, 0, 10) * 3.f;
		else if (D == "heal") Sum += Num(Q, "amt", 15, 0, 999) * 1.2f;
	}
	return FMath::RoundToInt(Sum);
}

float UDFSkillPackLib::EffectiveCooldown(const FDFSkillPack& P)
{
	const float FromPower = FMath::RoundToFloat(Power(P) / 14.f * 10.f) / 10.f;
	return FMath::Max3(P.DeclaredCooldown, 1.5f, FromPower);
}
