// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class XRInputSimulationEditor : ModuleRules
{
	public XRInputSimulationEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "XRInputSimulation" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}

