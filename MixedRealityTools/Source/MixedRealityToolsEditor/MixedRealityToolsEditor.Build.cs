// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MixedRealityToolsEditor : ModuleRules
{
	public MixedRealityToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
            new string[]
            {
                "MixedRealityToolsEditor/Public"
            });

        PrivateIncludePaths.AddRange(
            new string[] 
            {
				"MixedRealityToolsEditor/Private"
			});

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "MixedRealityTools" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
