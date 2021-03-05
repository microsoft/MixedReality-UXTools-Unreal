// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "IHeadMountedDisplayModule.h"
#include "SceneViewExtension.h"
#include "XRSimulationActor.h"
#include "XRSimulationHMD.h"
#include "XRSimulationRuntimeSettings.h"

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "XRSimulation"

/** XXX HMD disabled for the time being.
 * This module would create a fully defined virtual HMD implementation.
 * Due to workflow issues and engine bugs this is disabled for the time being.
 * XR simulation is integrated into UXTools as a submodule instead.
 *
 * When XRSimulationHMD is reinstated make sure to un-comment the ConfigRestartRequired flag on bEnableSimulation in the runtime settings
 */
#ifndef WITH_XRSIMULATIONHMD
#define WITH_XRSIMULATIONHMD 0
#endif
#if WITH_XRSIMULATIONHMD

class FXRSimulationModule : public IHeadMountedDisplayModule
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** IHeadMountedDisplayModule implementation */
	virtual TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> CreateTrackingSystem() override;

	FString GetModuleKeyName() const override { return FString(TEXT("XRSimulation")); }

	virtual bool IsHMDConnected() override;
};

IMPLEMENT_MODULE(FXRSimulationModule, XRSimulation);

void FXRSimulationModule::StartupModule()
{
	IHeadMountedDisplayModule::StartupModule();

	// Register keys for simulated events
	EKeys::AddMenuCategoryDisplayInfo(
		"XRSimulation", LOCTEXT("XRSimulationSubCategory", "XR Simulation"), TEXT("GraphEditor.PadEvent_16x"));

	EKeys::AddKey(FKeyDetails(
		FXRSimulationKeys::LeftSelect, LOCTEXT("XRSimulation_Left_Select_Axis", "XRSimulation (L) Select"),
		FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "XRSimulation"));
	EKeys::AddKey(FKeyDetails(
		FXRSimulationKeys::RightSelect, LOCTEXT("XRSimulation_Right_Select_Axis", "XRSimulation (R) Select"),
		FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "XRSimulation"));

	EKeys::AddKey(FKeyDetails(
		FXRSimulationKeys::LeftGrip, LOCTEXT("XRSimulation_Left_Grip_Axis", "XRSimulation (L) Grip"),
		FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "XRSimulation"));
	EKeys::AddKey(FKeyDetails(
		FXRSimulationKeys::RightGrip, LOCTEXT("XRSimulation_Right_Grip_Axis", "XRSimulation (R) Grip"),
		FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "XRSimulation"));
}

void FXRSimulationModule::ShutdownModule()
{
	EKeys::RemoveKeysWithCategory("XRSimulation");
}

TSharedPtr<class IXRTrackingSystem, ESPMode::ThreadSafe> FXRSimulationModule::CreateTrackingSystem()
{
	TSharedPtr<FXRSimulationHMD, ESPMode::ThreadSafe> HMD = FSceneViewExtensions::NewExtension<FXRSimulationHMD>();
	if (HMD->IsInitialized())
	{
		return HMD;
	}
	return nullptr;
}

bool FXRSimulationModule::IsHMDConnected()
{
	const UXRSimulationRuntimeSettings* Settings = UXRSimulationRuntimeSettings::Get();
	if (Settings)
	{
		return Settings->bEnableSimulation;
	}
	return false;
}

#else // WITH_XRSIMULATIONHMD

IMPLEMENT_MODULE(FDefaultModuleImpl, XRSimulation);

#endif // WITH_XRSIMULATIONHMD
