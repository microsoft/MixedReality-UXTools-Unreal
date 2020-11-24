// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXToolsTests : ModuleRules
{
	public UXToolsTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Required to avoid errors about undefined preprocessor macros (C4668) when building DirectXMath.h
		bEnableUndefinedIdentifierWarnings = false;

		// Never enable unity builds for tests
		MinSourceFilesForUnityBuildOverride = System.Int32.MaxValue;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "LiveLinkInterface", "UXTools" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Projects", "RenderCore", "Slate", "SlateCore", "UMG", "FunctionalTesting" });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}

