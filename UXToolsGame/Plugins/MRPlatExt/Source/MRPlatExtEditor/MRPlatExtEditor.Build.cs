// Copyright (c) Microsoft Corporation. All rights reserved.

using UnrealBuildTool;

public class MRPlatExtEditor : ModuleRules
{
	public MRPlatExtEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"InputCore",
				"Engine",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"EditorWidgets",
				"DesktopWidgets",
				"PropertyEditor",
				"UnrealEd",
				"SharedSettingsWidgets",
				"TargetPlatform",
				"RenderCore",
				"MRPlatExtRuntimeSettings"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Settings"
			}
		);
    }
}
