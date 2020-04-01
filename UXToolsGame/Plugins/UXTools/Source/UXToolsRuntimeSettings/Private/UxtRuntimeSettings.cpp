// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtRuntimeSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "CoreGlobals.h"
#include "UObject/Package.h"
#include "UObject/ConstructorHelpers.h"

UUxtRuntimeSettings* UUxtRuntimeSettings::UXToolsSettingsSingleton = nullptr;

UUxtRuntimeSettings::UUxtRuntimeSettings(const FObjectInitializer& ObjectInitializer)
{
	// Default hand pose button mappings
	HandPoseButtonMappings.Add(TEXT("Pinch"), { { EHMDInputControllerButtons::Grasp, EHMDInputControllerButtons::Select } });

	// Input simulation assets are only available on certain platforms, avoid errors trying to find content on unsupported platforms.
#if WITH_INPUT_SIMULATION
	// Default hand mesh and animation assets
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandMeshFinder(TEXT("/UXTools/InputSimulation/InputSimulationHands"));
	HandMesh = HandMeshFinder.Object;
	static ConstructorHelpers::FClassFinder<UAnimInstance> HandAnimFinder(TEXT("/UXTools/InputSimulation/InputSimulationHands_AnimInstance"));
	HandAnimInstance = HandAnimFinder.Class;
#endif
}

#if WITH_EDITOR

void UUxtRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false);
}

#endif

UUxtRuntimeSettings* UUxtRuntimeSettings::Get()
{
	if (UXToolsSettingsSingleton == nullptr)
	{
		static const TCHAR* SettingsContainerName = TEXT("UXToolsRuntimeSettingsContainer");

		UXToolsSettingsSingleton = FindObject<UUxtRuntimeSettings>(GetTransientPackage(), SettingsContainerName);

		if (UXToolsSettingsSingleton == nullptr)
		{
			UXToolsSettingsSingleton = NewObject<UUxtRuntimeSettings>(GetTransientPackage(), UUxtRuntimeSettings::StaticClass(), SettingsContainerName);
			UXToolsSettingsSingleton->AddToRoot();
		}
	}
	return UXToolsSettingsSingleton;
}