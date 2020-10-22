// Copyright (c) Microsoft Corporation. All rights reserved.

using UnrealBuildTool;

public class MRPlatExtRuntimeSettings : ModuleRules
{
	public MRPlatExtRuntimeSettings(ReadOnlyTargetRules Target) : base(Target)
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
