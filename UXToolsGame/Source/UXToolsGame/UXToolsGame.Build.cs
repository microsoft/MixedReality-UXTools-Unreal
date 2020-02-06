// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class UXToolsGame : ModuleRules
{
	public UXToolsGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Required to avoid errors about undefined preprocessor macros (C4668) when building DirectXMath.h
        bEnableUndefinedIdentifierWarnings = false;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "WindowsMixedRealityHandTracking", "LiveLinkInterface" });

		if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
	}
}
