// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class UXTools : ModuleRules
{
	public UXTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Required to avoid errors about undefined preprocessor macros (C4668) when building DirectXMath.h
        bEnableUndefinedIdentifierWarnings = false;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "WindowsMixedRealityHandTracking", "LiveLinkInterface" });

		// TODO Slate required for hand input simulation in hand joint attachment component. Remove once we have simulation at platform level.
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate" });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
	}
}
