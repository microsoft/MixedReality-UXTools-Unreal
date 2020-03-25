// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class UXToolsWMR : ModuleRules
{
	public UXToolsWMR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UXTools" });
		PrivateDependencyModuleNames.AddRange(new string[] { "WindowsMixedRealityHandTracking", "WindowsMixedRealityHMD" });
	}
}
