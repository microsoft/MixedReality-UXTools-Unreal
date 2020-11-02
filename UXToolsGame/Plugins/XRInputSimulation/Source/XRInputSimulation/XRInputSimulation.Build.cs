// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class XRInputSimulation : ModuleRules
{
	public XRInputSimulation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"HeadMountedDisplay",
				"InputCore",
				"WindowsMixedRealityHandTracking",
				"WindowsMixedRealityHMD",
				"WindowsMixedRealityInputSimulation"
			}
		);

		PrivateDependencyModuleNames.AddRange(new string[]
			{
			}
		);
	}
}
