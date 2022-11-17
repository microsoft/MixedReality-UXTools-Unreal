// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using UnrealBuildTool;

public class MicrosoftOpenXREditor : ModuleRules
{
	public MicrosoftOpenXREditor(ReadOnlyTargetRules Target) : base(Target)
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
				"MicrosoftOpenXRRuntimeSettings"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Settings"
			}
		);
    }
}
