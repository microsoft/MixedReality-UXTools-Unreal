// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class UXToolsWMR : ModuleRules
{
	public UXToolsWMR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UXTools" });
		PrivateDependencyModuleNames.AddRange(new string[] { "WindowsMixedRealityHandTracking", "WindowsMixedRealityHMD" });

		// TODO Remove when we fix BUG 141: https://dev.azure.com/MRDevPlat/DevPlat/_workitems/edit/141
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.Add("WindowsMixedRealityInputSimulation");
		}
	}
}
