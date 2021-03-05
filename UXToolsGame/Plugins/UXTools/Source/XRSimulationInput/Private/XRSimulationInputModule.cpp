// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "IInputDeviceModule.h"
#include "XRSimulationHMD.h"
#include "XRSimulationInputDevice.h"

#include "Modules/ModuleManager.h"

/** XXX HMD disabled for the time being.
 * This module would create a fully defined virtual HMD implementation.
 * Due to workflow issues and engine bugs this is disabled for the time being.
 * XR simulation is integrated into UXTools as a submodule instead.
 */
#ifndef WITH_XRSIMULATIONHMD
#define WITH_XRSIMULATIONHMD 0
#endif
#if WITH_XRSIMULATIONHMD

class FXRSimulationInputModule : public IInputDeviceModule
{
public:
	FXRSimulationInputModule();
	virtual ~FXRSimulationInputModule();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;

	/** IInputDeviceModule implementation */
	virtual TSharedPtr<class IInputDevice> CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;

private:
	FXRSimulationHMD* GetSimulationHMD() const;

private:
	TSharedPtr<FXRSimulationInputDevice> InputDevice;
};

IMPLEMENT_MODULE(FXRSimulationInputModule, XRSimulationInput);

FXRSimulationInputModule::FXRSimulationInputModule() : InputDevice()
{
}

FXRSimulationInputModule::~FXRSimulationInputModule()
{
}

void FXRSimulationInputModule::StartupModule()
{
	IInputDeviceModule::StartupModule();

	FXRSimulationHMD* InputSimHMD = GetSimulationHMD();
	// Note: InputSimHMD may be null, for example in the editor. But we still need the input device to enumerate sources.
	InputDevice = MakeShared<FXRSimulationInputDevice>(InputSimHMD);
}

TSharedPtr<class IInputDevice> FXRSimulationInputModule::CreateInputDevice(
	const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
	if (InputDevice)
		InputDevice->SetMessageHandler(InMessageHandler);
	return InputDevice;
}

FXRSimulationHMD* FXRSimulationInputModule::GetSimulationHMD() const
{
	static FName SystemName(TEXT("XRSimulation"));
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
	{
		return static_cast<FXRSimulationHMD*>(GEngine->XRSystem.Get());
	}

	return nullptr;
}

#else // WITH_XRSIMULATIONHMD

IMPLEMENT_MODULE(FDefaultModuleImpl, XRSimulationInput);

#endif // WITH_XRSIMULATIONHMD
