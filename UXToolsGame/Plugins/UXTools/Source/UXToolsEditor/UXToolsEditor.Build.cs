// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class UXToolsEditor : ModuleRules
{
	public UXToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "UXTools", "UXToolsRuntimeSettings" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
