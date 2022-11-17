// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

using System.IO;
using System.Linq;
using System;
using System.Collections.Generic;
using EpicGames.Core;
using UnrealBuildTool;

public class NuGetModule : ModuleRules
{
	public NuGetModule(ReadOnlyTargetRules Target) : base(Target)
	{
		PrecompileForTargets = PrecompileTargetsType.Any;
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		// WinRT with Nuget support
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.HoloLens)
		{
			// these parameters mandatory for winrt support
			bEnableExceptions = true;
			bUseUnity = false;
			CppStandard = CppStandardVersion.Cpp17;
			PublicSystemLibraries.AddRange(new string [] { "shlwapi.lib", "runtimeobject.lib" });

			// prepare everything for nuget
			string MyModuleName = GetType().Name;
			string NugetFolder = Path.Combine(PluginDirectory, "Intermediate", "Nuget", MyModuleName);
			Directory.CreateDirectory(NugetFolder);

			string BinariesSubFolder = Path.Combine("Binaries", "ThirdParty", Target.Type.ToString(), Target.Platform.ToString(), Target.Architecture);

			PublicDefinitions.Add(string.Format("THIRDPARTY_BINARY_SUBFOLDER=\"{0}\"", BinariesSubFolder.Replace(@"\", @"\\")));

			string BinariesFolder = Path.Combine(PluginDirectory, BinariesSubFolder);
			Directory.CreateDirectory(BinariesFolder);

			ExternalDependencies.Add("packages.config");

			// download nuget
			string NugetExe = Path.Combine(NugetFolder, "nuget.exe");
			if (!File.Exists(NugetExe))
			{
				// The System.Net assembly is not referenced by the build tool so it must be loaded dynamically.
				var assembly = System.Reflection.Assembly.Load("System.Net");
				var webClient = assembly.CreateInstance("System.Net.WebClient");
				using ((IDisposable)webClient)
				{
					// we aren't focusing on a specific nuget version, we can use any of them but the latest one is preferable
					var downloadFileMethod = webClient.GetType().GetMethod("DownloadFile", new Type[] { typeof(string), typeof(string) });
					downloadFileMethod.Invoke(webClient, new object[] { @"https://dist.nuget.org/win-x86-commandline/latest/nuget.exe", NugetExe } );
				}
			}

			// run nuget to update the packages
			{
				int ExitCode = 0;
				Utils.RunLocalProcessAndReturnStdOut(NugetExe, string.Format("install \"{0}\" -OutputDirectory \"{1}\"", Path.Combine(ModuleDirectory, "packages.config"), NugetFolder), out ExitCode, true);
				if (ExitCode < 0)
				{
					throw new BuildException("Failed to get nuget packages.  See log for details.");
				}
			}

			// get list of the installed packages, that's needed because the code should get particular versions of the installed packages
			string[] InstalledPackages = Utils.RunLocalProcessAndReturnStdOut(NugetExe, string.Format("list -Source \"{0}\"", NugetFolder)).Split(new char[] { '\r', '\n' });

			// winmd files of the packages
			List<string> WinMDFiles = new List<string>();

			// WinRT lib for some job
			string QRPackage = InstalledPackages.FirstOrDefault(x => x.StartsWith("Microsoft.MixedReality.QR"));
			if (!string.IsNullOrEmpty(QRPackage))
			{
				string QRFolderName = QRPackage.Replace(" ", ".");

				// copying dll and winmd binaries to our local binaries folder
				// !!!!! please make sure that you use the path of file! Unreal can't do it for you !!!!!
				string WinMDFile = Path.Combine(NugetFolder, QRFolderName, @"lib\uap10.0.18362\Microsoft.MixedReality.QR.winmd");
				SafeCopy(WinMDFile, Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.winmd"));

				SafeCopy(Path.Combine(NugetFolder, QRFolderName, string.Format(@"runtimes\win10-{0}\native\Microsoft.MixedReality.QR.dll", Target.WindowsPlatform.Architecture.ToString())),
					Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.dll"));

				// also both both binaries must be in RuntimeDependencies, unless you get failures in Hololens platform
				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.dll"));
				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.MixedReality.QR.winmd"));

				//add winmd file to the list for further processing using cppwinrt.exe
				WinMDFiles.Add(WinMDFile);
			}

			string ASAPackage = InstalledPackages.FirstOrDefault(x => x.StartsWith("Microsoft.Azure.SpatialAnchors.WinRT"));
			if (!string.IsNullOrEmpty(ASAPackage))
			{
				string ASAFolderName = ASAPackage.Replace(" ", ".");

				// copying dll and winmd binaries to our local binaries folder
				// !!!!! please make sure that you use the path of file! Unreal can't do it for you !!!!!
				string WinMDFile = Path.Combine(NugetFolder, ASAFolderName, @"lib\uap10.0\Microsoft.Azure.SpatialAnchors.winmd");
				SafeCopy(WinMDFile, Path.Combine(BinariesFolder, "Microsoft.Azure.SpatialAnchors.winmd"));

				SafeCopy(Path.Combine(NugetFolder, ASAFolderName, string.Format(@"runtimes\win10-{0}\native\Microsoft.Azure.SpatialAnchors.dll", Target.WindowsPlatform.Architecture.ToString())),
					Path.Combine(BinariesFolder, "Microsoft.Azure.SpatialAnchors.dll"));

				SafeCopy(Path.Combine(NugetFolder, ASAFolderName, string.Format(@"runtimes\win10-{0}\native\CoarseRelocUW.dll", Target.WindowsPlatform.Architecture.ToString())),
					Path.Combine(BinariesFolder, "CoarseRelocUW.dll"));

				// also all binaries must be in RuntimeDependencies
				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.Azure.SpatialAnchors.dll"));
				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.Azure.SpatialAnchors.winmd"));
				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "CoarseRelocUW.dll"));

				// add winmd file to the list for further processing using cppwinrt.exe
				WinMDFiles.Add(WinMDFile);
			}

			string AOAPackage = InstalledPackages.FirstOrDefault(x => x.StartsWith("Microsoft.Azure.ObjectAnchors.Runtime.WinRT"));
			if (!string.IsNullOrEmpty(AOAPackage))
			{
				string AOAFolderName = AOAPackage.Replace(" ", ".");

				// Copy dll and winmd binaries to our local binaries folder
				string WinMDFile = Path.Combine(NugetFolder, AOAFolderName, @"lib\uap10.0\Microsoft.Azure.ObjectAnchors.winmd");
				SafeCopy(WinMDFile, Path.Combine(BinariesFolder, "Microsoft.Azure.ObjectAnchors.winmd"));

				String[] Binaries = {
					"Microsoft.Azure.ObjectAnchors.dll",
					"ObjectTrackerApi.dll",
					"ObjectTrackerDiagnostics.dll",
					"ObjectTrackerFusion.dll",
					"ObjectTrackerRefinement.dll",
					"VolumeFusionAPI.dll" 
				};

				foreach (String Binary in Binaries)
                {
					SafeCopy(Path.Combine(NugetFolder, AOAFolderName, string.Format(@"runtimes\win10-{0}\native\{1}", Target.WindowsPlatform.Architecture.ToString(), Binary)),
						Path.Combine(BinariesFolder, Binary));

					RuntimeDependencies.Add(Path.Combine(BinariesFolder, Binary));
				}

				RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.Azure.ObjectAnchors.winmd"));

				// Add winmd file to the list for further processing using cppwinrt.exe
				WinMDFiles.Add(WinMDFile);
			}

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				// Microsoft.VCRTForwarders.140 is needed to run WinRT dlls in Win64 platforms
				string VCRTForwardersPackage = InstalledPackages.FirstOrDefault(x => x.StartsWith("Microsoft.VCRTForwarders.140"));
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

				string RemotingPackage = InstalledPackages.FirstOrDefault(x => x.StartsWith("Microsoft.Holographic.Remoting.OpenXr"));
				if (!string.IsNullOrEmpty(RemotingPackage))
				{
					string RemotingFolderName = RemotingPackage.Replace(" ", ".");

					SafeCopy(Path.Combine(NugetFolder, RemotingFolderName, @"build\native\bin\x64\Desktop\Microsoft.Holographic.AppRemoting.OpenXr.dll"),
						Path.Combine(BinariesFolder, "Microsoft.Holographic.AppRemoting.OpenXr.dll"));

					SafeCopy(Path.Combine(NugetFolder, RemotingFolderName, @"build\native\bin\x64\Desktop\Microsoft.Holographic.AppRemoting.OpenXr.SU.dll"),
						Path.Combine(BinariesFolder, "Microsoft.Holographic.AppRemoting.OpenXr.SU.dll"));

					SafeCopy(Path.Combine(NugetFolder, RemotingFolderName, @"build\native\bin\x64\Desktop\RemotingXR.json"),
						Path.Combine(BinariesFolder, "RemotingXR.json"));

					PublicIncludePaths.Add(Path.Combine(NugetFolder, RemotingFolderName, @"build\native\include\openxr"));

					RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.Holographic.AppRemoting.OpenXr.dll"));
					RuntimeDependencies.Add(Path.Combine(BinariesFolder, "Microsoft.Holographic.AppRemoting.OpenXr.SU.dll"));
					RuntimeDependencies.Add(Path.Combine(BinariesFolder, "RemotingXR.json"));
				}
			}

			// get WinRT package 
			string CppWinRTPackage = InstalledPackages.FirstOrDefault(x => x.StartsWith("Microsoft.Windows.CppWinRT"));
			if (!string.IsNullOrEmpty(CppWinRTPackage))
			{
				string CppWinRTName = CppWinRTPackage.Replace(" ", ".");
				string CppWinRTExe = Path.Combine(NugetFolder, CppWinRTName, "bin", "cppwinrt.exe");
				string CppWinRTFolder = Path.Combine(PluginDirectory, "Intermediate", CppWinRTName, MyModuleName);
				Directory.CreateDirectory(CppWinRTFolder);

				// all downloaded winmd file with WinSDK to be processed by cppwinrt.exe
				var WinMDFilesStringbuilder = new System.Text.StringBuilder();
				foreach (var winmd in WinMDFiles)
				{
					WinMDFilesStringbuilder.Append(" -input \"");
					WinMDFilesStringbuilder.Append(winmd);
					WinMDFilesStringbuilder.Append("\"");
				}

				// generate winrt headers and add them into include paths
				int ExitCode = 0;
				Utils.RunLocalProcessAndReturnStdOut(CppWinRTExe, string.Format("{0} -input \"{1}\" -output \"{2}\"", WinMDFilesStringbuilder, Target.WindowsPlatform.WindowsSdkVersion, CppWinRTFolder), out ExitCode, true);   
				if (ExitCode < 0)
				{
					throw new BuildException("Failed to get generate WinRT headers.  See log for details.");
				}

				PublicIncludePaths.Add(CppWinRTFolder);
			}
			else
			{
				// fall back to default WinSDK headers if no winrt package in our list
				PublicIncludePaths.Add(Path.Combine(Target.WindowsPlatform.WindowsSdkDir, "Include", Target.WindowsPlatform.WindowsSdkVersion, "cppwinrt"));
			}
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
