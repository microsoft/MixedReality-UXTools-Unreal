// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class AzureSpatialAnchorsForOpenXR : ModuleRules
	{
		public AzureSpatialAnchorsForOpenXR(ReadOnlyTargetRules Target) : base(Target)
		{
			if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
			{
				// these parameters mandatory for winrt support
				bEnableExceptions = true;
				bUseUnity = false;
				CppStandard = CppStandardVersion.Cpp17;
				PublicSystemLibraries.AddRange(new string[] { "shlwapi.lib", "runtimeobject.lib" });
			}

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"AugmentedReality",
					"HeadMountedDisplay",
					"OpenXRHMD",
					"NuGetModule",
					"Projects",
					"MicrosoftOpenXR"
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"AzureSpatialAnchors",
					"MicrosoftOpenXR"
				}
			);
			
			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"HeadMountedDisplay",
					"NuGetModule"
				}
			);
			
			PublicIncludePathModuleNames.AddRange(
				new string[]
				{
					"HeadMountedDisplay",
					"NuGetModule"
				}
			);

			string OpenXRPluginIncludePath = Path.Combine(PluginDirectory, "Source", "MicrosoftOpenXR", "Private");
			string OpenXRPrivateIncludePath = Path.Combine(PluginDirectory, "Source", "MicrosoftOpenXR", "Private", "External");
			PrivateIncludePaths.Add(OpenXRPluginIncludePath);
			PrivateIncludePaths.Add(OpenXRPrivateIncludePath);
		}
	}
}
