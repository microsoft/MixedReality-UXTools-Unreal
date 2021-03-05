// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class XRSimulationEditor : ModuleRules
{
	public XRSimulationEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "XRSimulation" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}

