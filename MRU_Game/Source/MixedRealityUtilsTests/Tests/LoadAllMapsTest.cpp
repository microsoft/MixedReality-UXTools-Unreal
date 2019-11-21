#include "AssetRegistryModule.h"
#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "MessageLog.h"
#include "Paths.h"
#include "Misc/AutomationTest.h"
#include "Misc/MapErrors.h"
#include "Modules/ModuleManager.h"
#include "Tests/AutomationCommon.h"

static const FName MapsPathRoot = TEXT("/MixedRealityUtils/Maps");
static const bool LoadMapsRecursive = true;
static const bool IncludeOnlyOnDiskAssets = true;
static const float StreamResourceTimeout = 10.0f;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadAllMapsTest, "MixedRealityUtils.LoadAllMaps",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

bool FLoadAllMapsTest::RunTest(const FString& Parameters)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> PackageAssetData;
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.GetAssetsByPath(MapsPathRoot, PackageAssetData, LoadMapsRecursive, IncludeOnlyOnDiskAssets);

	for (const FAssetData &asset : PackageAssetData)
	{
		if (asset.GetAsset()->IsA<UWorld>())
		{
			FString path = asset.PackageName.GetPlainNameString();
			AutomationOpenMap(path);

			ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
			ADD_LATENT_AUTOMATION_COMMAND(FStreamAllResourcesLatentCommand(StreamResourceTimeout));
			ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompilingInGame());
		}
	}

	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}
