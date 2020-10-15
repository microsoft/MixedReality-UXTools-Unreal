// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class UXToolsEditor : ModuleRules
{
	public UXToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "UXTools", "UXToolsRuntimeSettings" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "SlateCore", "Slate", "EditorStyle", "Blutility", "UMG", "UMGEditor" });
	}
}

