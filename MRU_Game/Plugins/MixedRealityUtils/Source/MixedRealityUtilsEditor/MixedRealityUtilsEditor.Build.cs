// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MixedRealityUtilsEditor : ModuleRules
{
	public MixedRealityUtilsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		//PublicIncludePaths.AddRange(
  //          new string[]
  //          {
  //              "MixedRealityUtilsEditor/Public"
  //          });

  //      PrivateIncludePaths.AddRange(
  //          new string[] 
  //          {
		//		"MixedRealityUtilsEditor/Private"
		//	});

		PublicDependencyModuleNames.AddRange(new string[] { "Engine", "Core", "CoreUObject", "MixedRealityUtils" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
