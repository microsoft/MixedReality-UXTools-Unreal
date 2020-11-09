// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXTools : ModuleRules
{
	public UXTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Required to avoid errors about undefined preprocessor macros (C4668) when building DirectXMath.h
		bEnableUndefinedIdentifierWarnings = false;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "LiveLinkInterface", "RenderCore", "ProceduralMeshComponent", "EyeTracker" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG", "RHI" });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		// DirectXMath is not present by default in non-Windows platforms and doesn't build in Android 
		// when using gcc so we add a definition to disable code using it to avoid build breaks.
		// See Task 218: Investigate building DirectXMath for Android and iOS.
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			PrivateDefinitions.Add("UXT_DIRECTXMATH_SUPPORTED=1");
		}
		else
		{
			PrivateDefinitions.Add("UXT_DIRECTXMATH_SUPPORTED=0");
		}
	}
}

