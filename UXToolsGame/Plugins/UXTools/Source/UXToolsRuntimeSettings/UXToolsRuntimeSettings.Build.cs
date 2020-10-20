// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXToolsRuntimeSettings : ModuleRules
{
	public UXToolsRuntimeSettings(ReadOnlyTargetRules Target) : base(Target)
	{
		/** Runtime settings are in a separate module so that both UXTools and input simulation can use settings
		 *  without a direct dependency.
		 *  The settings module should also be loaded at an early stage which is easier to ensure with a separate module.
		 */

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePathModuleNames.AddRange(
			new string[]
			{
				"WindowsMixedRealityHandTracking"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"WindowsMixedRealityHMD"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Win64 && Target.bBuildEditor == true)
		{
			PrivateDefinitions.Add("WITH_INPUT_SIMULATION=1");
		}
		else
		{
			PrivateDefinitions.Add("WITH_INPUT_SIMULATION=0");
		}
	}
}
