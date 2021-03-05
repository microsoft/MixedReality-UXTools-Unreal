// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXToolsInput : ModuleRules
{
	public UXToolsInput(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Required to avoid errors about undefined preprocessor macros (C4668) when building DirectXMath.h
		bEnableUndefinedIdentifierWarnings = false;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "AugmentedReality", "LiveLinkInterface", "XRSimulation" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UXTools" });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

	}
}

