// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRInputSimulationRuntimeSettings.h"

#include "CoreGlobals.h"

#include "Misc/ConfigCacheIni.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

UXRInputSimulationRuntimeSettings* UXRInputSimulationRuntimeSettings::XRInputSimSettingsSingleton = nullptr;

UXRInputSimulationRuntimeSettings::UXRInputSimulationRuntimeSettings(const FObjectInitializer& ObjectInitializer)
{
	// Default hand pose button mappings
	HandPoseButtonMappings.Add(TEXT("Pinch"), {{EHMDInputControllerButtons::Grasp, EHMDInputControllerButtons::Select}});

	// Input simulation assets are only available on certain platforms, avoid errors trying to find content on unsupported platforms.
#if WITH_INPUT_SIMULATION
	// Default hand mesh and animation assets
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandMeshFinder(TEXT("/XRInputSimulation/InputSimulationHands"));
	HandMesh = HandMeshFinder.Object;
	static ConstructorHelpers::FClassFinder<UAnimInstance> HandAnimFinder(
		TEXT("/XRInputSimulation/InputSimulationHands_AnimInstance"));
	HandAnimInstance = HandAnimFinder.Class;
#endif
}

#if WITH_EDITOR

void UXRInputSimulationRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false);
}

#endif

UXRInputSimulationRuntimeSettings* UXRInputSimulationRuntimeSettings::Get()
{
	if (XRInputSimSettingsSingleton == nullptr)
	{
		static const TCHAR* SettingsContainerName = TEXT("XRInputSimulationRuntimeSettingsContainer");

		XRInputSimSettingsSingleton = FindObject<UXRInputSimulationRuntimeSettings>(GetTransientPackage(), SettingsContainerName);

		if (XRInputSimSettingsSingleton == nullptr)
		{
			XRInputSimSettingsSingleton =
				NewObject<UXRInputSimulationRuntimeSettings>(GetTransientPackage(), UXRInputSimulationRuntimeSettings::StaticClass(), SettingsContainerName);
			XRInputSimSettingsSingleton->AddToRoot();
		}
	}
	return XRInputSimSettingsSingleton;
}