// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

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

