#include "DFVoiceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	// seconds before the same event may speak again; chance to speak at all
	const float EventCooldown[(int32)EDFVoiceEvent::MAX] = { 0, 0, 6, 2, 0, 4, 30, 5, 3, 0, 0, 0, 0 };
	const float EventChance[(int32)EDFVoiceEvent::MAX]   = { 1, 1, .15f, .6f, 1, .35f, 1, .25f, .3f, 1, 1, 1, 1 };
	// events that may cut into a line already playing
	bool IsImportant(EDFVoiceEvent E)
	{
		return E == EDFVoiceEvent::Ultimate || E == EDFVoiceEvent::Win || E == EDFVoiceEvent::Lose
			|| E == EDFVoiceEvent::BossAppear || E == EDFVoiceEvent::RareAura || E == EDFVoiceEvent::Select;
	}
}

bool UDFVoiceSubsystem::ParseEvent(const FString& Key, EDFVoiceEvent& Out)
{
	static const TMap<FString, EDFVoiceEvent> Map = {
		{ TEXT("select"), EDFVoiceEvent::Select }, { TEXT("fightstart"), EDFVoiceEvent::FightStart },
		{ TEXT("attack"), EDFVoiceEvent::Attack }, { TEXT("skill"), EDFVoiceEvent::Skill },
		{ TEXT("ultimate"), EDFVoiceEvent::Ultimate }, { TEXT("hurt"), EDFVoiceEvent::Hurt },
		{ TEXT("lowhp"), EDFVoiceEvent::LowHP }, { TEXT("dash"), EDFVoiceEvent::Dash },
		{ TEXT("kill"), EDFVoiceEvent::Kill }, { TEXT("bossappear"), EDFVoiceEvent::BossAppear },
		{ TEXT("win"), EDFVoiceEvent::Win }, { TEXT("lose"), EDFVoiceEvent::Lose },
		{ TEXT("rareaura"), EDFVoiceEvent::RareAura } };
	const EDFVoiceEvent* E = Map.Find(Key.ToLower());
	if (E) Out = *E;
	return E != nullptr;
}

void UDFVoiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IFileManager::Get().MakeDirectory(*PacksFolder(), true);
	ScanPacks();
	LoadAssignments();
}

UDFVoiceSubsystem* UDFVoiceSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UDFVoiceSubsystem>() : nullptr;
}

FString UDFVoiceSubsystem::PacksFolder()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("VoicePacks"));
}

void UDFVoiceSubsystem::OpenPacksFolder() const
{
	FPlatformProcess::ExploreFolder(*PacksFolder());
}

void UDFVoiceSubsystem::ScanPacks()
{
	Packs.Reset();
	Cache.Reset();
	TArray<FString> Dirs;
	IFileManager::Get().FindFiles(Dirs, *(PacksFolder() / TEXT("*")), false, true);
	for (const FString& Dir : Dirs)
	{
		const FString Root = PacksFolder() / Dir;
		FString Json;
		if (!FFileHelper::LoadFileToString(Json, *(Root / TEXT("voice.json")))) continue;
		TSharedPtr<FJsonObject> O;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O.IsValid()) continue;
		if (O->GetStringField(TEXT("format")) != TEXT("dfvoice")) continue;

		FDFVoicePack P;
		P.Id = Dir;
		P.Name = O->HasField(TEXT("name")) ? O->GetStringField(TEXT("name")) : Dir;
		P.Author = O->HasField(TEXT("author")) ? O->GetStringField(TEXT("author")) : FString();
		if (O->HasField(TEXT("character"))) P.ForHero = FName(*O->GetStringField(TEXT("character")));

		const TSharedPtr<FJsonObject>* Lines;
		if (O->TryGetObjectField(TEXT("lines"), Lines))
			for (const auto& KV : (*Lines)->Values)
			{
				EDFVoiceEvent E;
				if (!ParseEvent(KV.Key, E)) continue;
				const TArray<TSharedPtr<FJsonValue>>* Arr;
				if (!KV.Value->TryGetArray(Arr)) continue;
				for (const TSharedPtr<FJsonValue>& V : *Arr)
				{
					FDFVoiceLine L;
					FString FileName;
					const TSharedPtr<FJsonObject>* LO;
					if (V->TryGetObject(LO)) { (*LO)->TryGetStringField(TEXT("file"), FileName); (*LO)->TryGetStringField(TEXT("text"), L.Text); }
					else V->TryGetString(FileName);
					if (FileName.IsEmpty() || FileName.Contains(TEXT(".."))) continue; // stay inside the pack folder
					L.File = Root / FileName;
					if (FPaths::FileExists(L.File)) P.Lines.FindOrAdd(E).Add(L);
				}
			}
		if (P.LineCount() > 0) Packs.Add(Dir, P);
	}
}

TArray<FString> UDFVoiceSubsystem::GetPackIds() const
{
	TArray<FString> Ids;
	Packs.GetKeys(Ids);
	Ids.Sort();
	return Ids;
}

FString UDFVoiceSubsystem::PackLabel(const FString& Id) const
{
	const FDFVoicePack* P = Packs.Find(Id);
	if (!P) return TEXT("ไม่มีเสียงพากย์");
	return FString::Printf(TEXT("%s%s · %d ประโยค"), *P->Name,
		P->Author.IsEmpty() ? TEXT("") : *(TEXT(" โดย ") + P->Author), P->LineCount());
}

void UDFVoiceSubsystem::AssignPack(FName Hero, const FString& PackId)
{
	if (PackId.IsEmpty()) Assigned.Remove(Hero);
	else Assigned.Add(Hero, PackId);
	SaveAssignments();
}

FString UDFVoiceSubsystem::GetAssignedPack(FName Hero) const
{
	const FString* S = Assigned.Find(Hero);
	return (S && Packs.Contains(*S)) ? *S : FString();
}

FString UDFVoiceSubsystem::CycleAssignedPack(FName Hero)
{
	TArray<FString> Ids = GetPackIds();
	Ids.Insert(FString(), 0); // "none" first
	const int32 I = Ids.IndexOfByKey(GetAssignedPack(Hero));
	const FString Next = Ids[(I + 1) % Ids.Num()];
	AssignPack(Hero, Next);
	return Next;
}

void UDFVoiceSubsystem::LoadAssignments()
{
	FString Json;
	if (!FFileHelper::LoadFileToString(Json, *(PacksFolder() / TEXT("_assigned.json")))) return;
	TSharedPtr<FJsonObject> O;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), O) || !O.IsValid()) return;
	for (const auto& KV : O->Values) Assigned.Add(FName(*KV.Key), KV.Value->AsString());
}

void UDFVoiceSubsystem::SaveAssignments() const
{
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	for (const auto& KV : Assigned) O->SetStringField(KV.Key.ToString(), KV.Value);
	FString Out;
	FJsonSerializer::Serialize(O, TJsonWriterFactory<>::Create(&Out));
	FFileHelper::SaveStringToFile(Out, *(PacksFolder() / TEXT("_assigned.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

// Minimal RIFF/WAVE reader: 16-bit PCM, 1 or 2 channels, any sample rate.
bool UDFVoiceSubsystem::LoadWav(const FString& Path, FPcm& Out)
{
	if (const FPcm* C = Cache.Find(Path)) { Out = *C; return true; }
	TArray<uint8> B;
	if (!FFileHelper::LoadFileToArray(B, *Path) || B.Num() < 44) return false;
	auto U32 = [&](int32 I) { return (uint32)B[I] | ((uint32)B[I + 1] << 8) | ((uint32)B[I + 2] << 16) | ((uint32)B[I + 3] << 24); };
	auto U16 = [&](int32 I) { return (uint16)(B[I] | (B[I + 1] << 8)); };
	if (FMemory::Memcmp(B.GetData(), "RIFF", 4) || FMemory::Memcmp(B.GetData() + 8, "WAVE", 4)) return false;
	int32 Pos = 12, Fmt = -1, Bits = 0;
	FPcm P;
	while (Pos + 8 <= B.Num())
	{
		const uint32 Size = U32(Pos + 4);
		const int32 Body = Pos + 8;
		if (!FMemory::Memcmp(B.GetData() + Pos, "fmt ", 4) && Body + 16 <= B.Num())
		{
			Fmt = U16(Body);
			P.Channels = U16(Body + 2);
			P.Rate = (int32)U32(Body + 4);
			Bits = U16(Body + 14);
		}
		else if (!FMemory::Memcmp(B.GetData() + Pos, "data", 4))
		{
			const int32 N = FMath::Min<int32>((int32)Size, B.Num() - Body);
			P.Data.Append(B.GetData() + Body, N);
			break;
		}
		Pos = Body + (int32)Size + (Size & 1);
	}
	if (Fmt != 1 || Bits != 16 || P.Channels < 1 || P.Channels > 2 || P.Rate <= 0 || !P.Data.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawFight voice: %s must be 16-bit PCM WAV (mono/stereo)"), *Path);
		return false;
	}
	P.Seconds = (float)P.Data.Num() / (P.Rate * P.Channels * 2);
	if (P.Seconds > 12.f) { UE_LOG(LogTemp, Warning, TEXT("DrawFight voice: %s longer than 12 s, skipped"), *Path); return false; }
	Cache.Add(Path, P);
	Out = MoveTemp(P);
	return true;
}

bool UDFVoiceSubsystem::Play(const UObject* WorldContext, FName Hero, EDFVoiceEvent Event)
{
	const FDFVoicePack* P = Packs.Find(GetAssignedPack(Hero));
	if (!P || !WorldContext) return false;
	const TArray<FDFVoiceLine>* Lines = P->Lines.Find(Event);
	if (!Lines || !Lines->Num()) return false;

	const int32 E = (int32)Event;
	const double Now = FPlatformTime::Seconds();
	if (Now < NextAllowed[E] || FMath::FRand() > EventChance[E]) return false;
	if (Now < AnyUntil && !IsImportant(Event)) return false; // don't talk over yourself

	const FDFVoiceLine& L = (*Lines)[FMath::RandRange(0, Lines->Num() - 1)];
	FPcm Pcm;
	if (!LoadWav(L.File, Pcm)) return false;

	USoundWaveProcedural* W = NewObject<USoundWaveProcedural>();
	W->SetSampleRate(Pcm.Rate);
	W->NumChannels = Pcm.Channels;
	W->Duration = Pcm.Seconds;
	W->SoundGroup = SOUNDGROUP_Voice;
	W->bLooping = false;
	W->QueueAudio(Pcm.Data.GetData(), Pcm.Data.Num());
	UGameplayStatics::PlaySound2D(WorldContext, W, Volume);

	NextAllowed[E] = Now + EventCooldown[E];
	AnyUntil = Now + Pcm.Seconds;
	if (!L.Text.IsEmpty()) OnSubtitle.Broadcast(L.Text, FMath::Max(1.5f, Pcm.Seconds + .5f));
	return true;
}
