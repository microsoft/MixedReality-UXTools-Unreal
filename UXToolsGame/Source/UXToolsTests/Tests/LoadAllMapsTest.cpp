// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "Logging/MessageLog.h"
#include "Misc/AutomationTest.h"
#include "Misc/MapErrors.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Tests/AutomationCommon.h"

static const FName MapsPathRoot = TEXT("/UXTools/Maps");
static const bool LoadMapsRecursive = true;
static const bool IncludeOnlyOnDiskAssets = true;
static const float StreamResourceTimeout = 10.0f;

IMPLEMENT_COMPLEX_AUTOMATION_TEST(
	FLoadAllMapsTest, "UXTools.LoadAllMaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

void FLoadAllMapsTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> PackageAssetData;
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.GetAssetsByPath(MapsPathRoot, PackageAssetData, LoadMapsRecursive, IncludeOnlyOnDiskAssets);

	for (const FAssetData& asset : PackageAssetData)
	{
		if (asset.GetAsset()->IsA<UWorld>())
		{
			FString Name = asset.AssetName.GetPlainNameString();
			FString Path = asset.PackageName.GetPlainNameString();

			OutBeautifiedNames.Add(Path);
			OutTestCommands.Add(Path);
		}
	}
}

bool FLoadAllMapsTest::RunTest(const FString& Parameters)
{
	FString Path = Parameters;
	AutomationOpenMap(Path);

	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FStreamAllResourcesLatentCommand(StreamResourceTimeout));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompilingInGame());

	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}
