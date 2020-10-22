// Copyright (c) Microsoft Corporation.

using System.IO;
using System.Linq;
using System.Runtime.Remoting.Messaging;
using UnrealBuildTool;
using Tools.DotNETCommon;
using System;

public class MRPlatExt : ModuleRules
{
	public MRPlatExt(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[] {
				// This private include path ensures our newer copy of the openxr headers take precedence over the engine's copy.
				"MRPlatExt/Private/External"
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
				"MRPlatExtRuntimeSettings",
				"HeadMountedDisplay",
				"AugmentedReality",
				"OpenXRAR",
				"RHI",
				"RenderCore",
				"Projects",
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
				"HeadMountedDisplay"
			}
			);

		PublicIncludePathModuleNames.AddRange(
			new string[]
			{
				"HeadMountedDisplay"
			}
			);

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "MRPlatExt_UPL.xml"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "ThirdParty", "armeabi-v7a", "libopenxr_loader.so"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "ThirdParty", "arm64-v8a", "libopenxr_loader.so"));
		}
		else if(Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			// these parameters mandatory for winrt support
			bEnableExceptions = true;
			bUseUnity = false;
			CppStandard = CppStandardVersion.Cpp17;
			PublicSystemLibraries.Add("shlwapi.lib");
			PublicSystemLibraries.Add("runtimeobject.lib");


			string NugetFolder = Path.Combine(PluginDirectory, "Intermediate", "Nuget", "MRPlatExt");
			Directory.CreateDirectory(NugetFolder);

			string BinariesSubFolder = Path.Combine("Binaries", "ThirdParty", Target.Type.ToString(), Target.Platform.ToString(), Target.Architecture);

			PrivateDefinitions.Add(string.Format("THIRDPARTY_BINARY_SUBFOLDER=\"{0}\"", BinariesSubFolder.Replace(@"\", @"\\")));

			string BinariesFolder = Path.Combine(PluginDirectory, BinariesSubFolder);
			Directory.CreateDirectory(BinariesFolder);

			string NugetExe = Path.Combine(NugetFolder, "nuget.exe");
			if(!File.Exists(NugetExe))
			{
				using (System.Net.WebClient myWebClient = new System.Net.WebClient())
				{
					myWebClient.DownloadFile(@"https://dist.nuget.org/win-x86-commandline/latest/nuget.exe", NugetExe);
				}
			}

			{
				var StartInfo = new System.Diagnostics.ProcessStartInfo(NugetExe, string.Format("install \"{0}\" -OutputDirectory \"{1}\"", Path.Combine(ModuleDirectory, "packages.config"), NugetFolder));
				StartInfo.UseShellExecute = false;
				StartInfo.CreateNoWindow = true;
				var ExitCode = Utils.RunLocalProcessAndPrintfOutput(StartInfo);
				if (ExitCode < 0)
				{
					throw new BuildException("Failed to get nuget packages.  See log for details.");
				}
			}
			
			string[] InstalledPackages = Utils.RunLocalProcessAndReturnStdOut(NugetExe, string.Format("list -Source \"{0}\"", NugetFolder)).Split(new char[] {'\r', '\n' });

			string CppWinRTPackage = InstalledPackages.First(x => x.StartsWith("Microsoft.Windows.CppWinRT"));
			if(!string.IsNullOrEmpty(CppWinRTPackage))
			{
				string CppWinRTName = CppWinRTPackage.Replace(" ", ".");
				string CppWinRTExe = Path.Combine(NugetFolder, CppWinRTName, "bin", "cppwinrt.exe");
				string CppWinRTFolder = Path.Combine(PluginDirectory, "Intermediate", CppWinRTName, "MRPlatExt");
				Directory.CreateDirectory(CppWinRTFolder);

				string[] WinMDFiles = Directory.GetFiles(NugetFolder, "*.winmd", SearchOption.AllDirectories);

				var WinMDFilesStringbuilder = new System.Text.StringBuilder();
				foreach(var winmd in WinMDFiles)
				{
					WinMDFilesStringbuilder.Append(" -input \"");
					WinMDFilesStringbuilder.Append(winmd);
					WinMDFilesStringbuilder.Append("\"");
				}

				var StartInfo = new System.Diagnostics.ProcessStartInfo(CppWinRTExe, string.Format("{0} -input \"{1}\" -output \"{2}\"", WinMDFilesStringbuilder, Target.WindowsPlatform.WindowsSdkVersion, CppWinRTFolder));
				StartInfo.UseShellExecute = false;
				StartInfo.CreateNoWindow = true;
				var ExitCode = Utils.RunLocalProcessAndPrintfOutput(StartInfo);
				if (ExitCode < 0)
				{
					throw new BuildException("Failed to get generate WinRT headers.  See log for details.");
				}

				PrivateIncludePaths.Add(CppWinRTFolder);

			}
			else
			{
				PrivateIncludePaths.Add(Path.Combine(Target.WindowsPlatform.WindowsSdkDir, "Include", Target.WindowsPlatform.WindowsSdkVersion, "cppwinrt"));
			}

			string QRPackage = InstalledPackages.First(x => x.StartsWith("Microsoft.MixedReality.QR"));
			if (!string.IsNullOrEmpty(QRPackage))
			{
				string QRFolderName = QRPackage.Replace(" ", ".");

				SafeCopy(Path.Combine(NugetFolder, QRFolderName, @"lib\uap10.0.18362\Microsoft.MixedReality.QR.winmd"), 
					Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.winmd"));

				SafeCopy(Path.Combine(NugetFolder, QRFolderName, string.Format(@"runtimes\win10-{0}\native\Microsoft.MixedReality.QR.dll", Target.WindowsPlatform.Architecture.ToString())), 
					Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.dll"));

				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.dll"));
				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.winmd"));
			}

			if(Target.Platform == UnrealTargetPlatform.Win64)
			{
				string VCRTForwardersPackage = InstalledPackages.First(x => x.StartsWith("Microsoft.VCRTForwarders.140"));
				if (!string.IsNullOrEmpty(VCRTForwardersPackage))
				{
					string VCRTForwardersName = VCRTForwardersPackage.Replace(" ", ".");
					foreach (var Dll in Directory.EnumerateFiles(Path.Combine(NugetFolder, VCRTForwardersName, "runtimes/win10-x64/native/release"), "*_app.dll"))
					{
						string newDll = Path.Combine(BinariesFolder, Path.GetFileName(Dll));
						SafeCopy(Dll, newDll);
						RuntimeDependencies.Add(newDll);
					}
				}
			}
		}

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "HolographicAppRemoting"));
			
			RuntimeDependencies.Add("$(PluginDir)/ThirdParty/HolographicAppRemoting/Windows/Win64/Microsoft.Holographic.AppRemoting.OpenXr.dll");
			RuntimeDependencies.Add("$(PluginDir)/ThirdParty/HolographicAppRemoting/Windows/Win64/RemotingXR.json");
		}
	}

	private void SafeCopy(string source, string destination)
	{
		if(!File.Exists(source))
		{
			Log.TraceError("Class {0} can't find {1} file for copying", this.GetType().Name, source);
			return;
		}

		try
		{
			File.Copy(source, destination, true);
		}
		catch(IOException ex)
		{
			Log.TraceWarning("Failed to copy {0} to {1} with exception: {2}", source, destination, ex.Message);
			if (!File.Exists(destination))
			{
				Log.TraceError("Destination file {0} does not exist", destination);
				return;
			}

			Log.TraceWarning("Destination file {0} already existed and is probably in use.  The old file will be used for the runtime dependency.  This may happen when packaging a Win64 exe from the editor.", destination);
		}
	}
}
