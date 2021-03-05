// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "IInputDevice.h"
#include "InputCoreTypes.h"

class AXRSimulationActor;
class FXRSimulationHMD;

/**
 * Virtual device for simulated XR input events.
 */
class FXRSimulationInputDevice
	: public IInputDevice
	, public TSharedFromThis<FXRSimulationInputDevice>
{
public:
	FXRSimulationInputDevice(FXRSimulationHMD* InputSimHMD);
	virtual ~FXRSimulationInputDevice();

	/** IInputDevice implementation */
	virtual void Tick(float DeltaTime) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override;
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values) override;

private:
	void SendHandEvents(const AXRSimulationActor* InputSimActor, EControllerHand Hand);

private:
	FXRSimulationHMD* InputSimHMD;

	/** handler to send all messages to */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;

	bool bLeftSelectPressed;
	bool bRightSelectPressed;
	bool bLeftGripPressed;
	bool bRightGripPressed;
};
