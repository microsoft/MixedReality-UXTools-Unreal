// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXToolsInputSimulation : ModuleRules
{
	public UXToolsInputSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		
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
