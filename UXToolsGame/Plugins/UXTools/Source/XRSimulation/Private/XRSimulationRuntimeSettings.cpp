// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRSimulationRuntimeSettings.h"

#include "CoreGlobals.h"

#include "Misc/ConfigCacheIni.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

const FKey FXRSimulationKeys::LeftSelect = FKey("XRSimulation_Left_Select");
const FKey FXRSimulationKeys::RightSelect = FKey("XRSimulation_Right_Select");
const FKey FXRSimulationKeys::LeftGrip = FKey("XRSimulation_Left_Grip");
const FKey FXRSimulationKeys::RightGrip = FKey("XRSimulation_Right_Grip");

UXRSimulationRuntimeSettings* UXRSimulationRuntimeSettings::XRInputSimSettingsSingleton = nullptr;

UXRSimulationRuntimeSettings::UXRSimulationRuntimeSettings(const FObjectInitializer& ObjectInitializer)
{
	// Default hand pose button mappings
	HandPoseKeys.Add({EControllerHand::Left, TEXT("Pinch"), FXRSimulationKeys::LeftSelect});
	HandPoseKeys.Add({EControllerHand::Left, TEXT("Pinch"), FXRSimulationKeys::LeftGrip});
	HandPoseKeys.Add({EControllerHand::Right, TEXT("Pinch"), FXRSimulationKeys::RightSelect});
	HandPoseKeys.Add({EControllerHand::Right, TEXT("Pinch"), FXRSimulationKeys::RightGrip});

	// Default hand mesh and animation assets
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandMeshFinder(TEXT("/UXTools/XRSimulation/SK_Hand"));
	HandMesh = HandMeshFinder.Object;
	static ConstructorHelpers::FClassFinder<UAnimInstance> HandAnimFinder(TEXT("/UXTools/XRSimulation/HandAnimBlueprint"));
	HandAnimInstance = HandAnimFinder.Class;
}

#if WITH_EDITOR

void UXRSimulationRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false);
}

#endif

UXRSimulationRuntimeSettings* UXRSimulationRuntimeSettings::Get()
{
	if (XRInputSimSettingsSingleton == nullptr)
	{
		static const TCHAR* SettingsContainerName = TEXT("XRSimulationRuntimeSettingsContainer");

		XRInputSimSettingsSingleton = FindObject<UXRSimulationRuntimeSettings>(GetTransientPackage(), SettingsContainerName);

		if (XRInputSimSettingsSingleton == nullptr)
		{
			XRInputSimSettingsSingleton = NewObject<UXRSimulationRuntimeSettings>(
				GetTransientPackage(), UXRSimulationRuntimeSettings::StaticClass(), SettingsContainerName);
			XRInputSimSettingsSingleton->AddToRoot();
		}
	}
	return XRInputSimSettingsSingleton;
}
