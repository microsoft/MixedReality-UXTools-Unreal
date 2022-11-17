// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnrealBuildTool;

public class MicrosoftOpenXR : ModuleRules
{
	public MicrosoftOpenXR(ReadOnlyTargetRules Target) : base(Target)
	{
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			// these parameters mandatory for winrt support
			bEnableExceptions = true;
			bUseUnity = false;
			CppStandard = CppStandardVersion.Cpp17;
			PublicSystemLibraries.AddRange(new string[] { "shlwapi.lib", "runtimeobject.lib" });
		}

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = @"Private\OpenXRCommon.h";

		PrivateIncludePaths.AddRange(
			new string[] {
				// This private include path ensures our newer copy of the openxr headers take precedence over the engine's copy.
				"MicrosoftOpenXR/Private/External"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"ApplicationCore",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"OpenXRHMD",
				"MicrosoftOpenXRRuntimeSettings",
				"HeadMountedDisplay",
				"AugmentedReality",
				"OpenXRAR",
				"RHI",
				"RHICore",
				"RenderCore",
				"Projects",
				"NuGetModule"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"NuGetModule"
			}
		);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd"
				}
			);
		}

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

		// DX-specific code for webcam texture processing
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
					"D3D11RHI",
					"D3D12RHI"
				});

			var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
			PrivateIncludePaths.AddRange(
				new string[] {
							Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private"),
							Path.Combine(EngineDir, @"Source\Runtime\Windows\D3D11RHI\Private\Windows"),
							Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private"),
							Path.Combine(EngineDir, @"Source\Runtime\D3D12RHI\Private\Windows")
							});

			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "AMD_AGS");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
		}
	}
}
