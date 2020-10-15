// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXToolsInputSimulation : ModuleRules
{
	public UXToolsInputSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"HeadMountedDisplay",
				"InputCore",
				"UXToolsRuntimeSettings",
				"WindowsMixedRealityHandTracking",
				"WindowsMixedRealityInputSimulation"
			}
		);

		PrivateDependencyModuleNames.AddRange(new string[]
			{
			}
		);
	}
}
