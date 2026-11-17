// Merge these dependencies into the project's own DrawFight.Build.cs
// (the Third Person C++ template creates one). Niagara is needed because
// FDFSkillRow references UNiagaraSystem.
using UnrealBuildTool;

public class DrawFight : ModuleRules
{
	public DrawFight(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Niagara", "Json"
		});
	}
}
