// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SuperHeavySim : ModuleRules
{
	public SuperHeavySim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG", "Json", "Niagara", "PhysicsCore", "Chaos", "RHI", "RenderCore", "RecoveryLoading", "AudioMixer" });
		bool HasDLSS = Target.Platform == UnrealTargetPlatform.Win64 && System.IO.Directory.Exists(System.IO.Path.Combine(ModuleDirectory, "..", "..", "Plugins", "NVIDIA", "DLSS"));
		PublicDefinitions.Add("RECOVERY_WITH_DLSS=" + (HasDLSS ? "1" : "0"));
		if (HasDLSS) PrivateDependencyModuleNames.Add("DLSSBlueprint");
	}
}
