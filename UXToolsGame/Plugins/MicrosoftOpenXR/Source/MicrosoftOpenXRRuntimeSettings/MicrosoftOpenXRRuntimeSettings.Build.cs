// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class MicrosoftOpenXRRuntimeSettings : ModuleRules
{
	public MicrosoftOpenXRRuntimeSettings(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		if (Target.Type == TargetRules.TargetType.Editor || Target.Type == TargetRules.TargetType.Program)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"TargetPlatform"
				}
			);
		}
	}
}
