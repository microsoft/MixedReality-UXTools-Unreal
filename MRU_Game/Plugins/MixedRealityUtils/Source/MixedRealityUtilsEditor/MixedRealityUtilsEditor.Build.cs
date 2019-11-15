// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MixedRealityUtilsEditor : ModuleRules
{
	public MixedRealityUtilsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "MixedRealityUtils" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
