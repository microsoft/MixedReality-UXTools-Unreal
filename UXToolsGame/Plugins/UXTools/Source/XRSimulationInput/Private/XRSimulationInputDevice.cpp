// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRSimulationInputDevice.h"

#include "XRSimulationActor.h"
#include "XRSimulationHMD.h"
#include "XRSimulationRuntimeSettings.h"

FXRSimulationInputDevice::FXRSimulationInputDevice(FXRSimulationHMD* InputSimHMD)
	: InputSimHMD(InputSimHMD)
	, MessageHandler(new FGenericApplicationMessageHandler())
{
}

FXRSimulationInputDevice::~FXRSimulationInputDevice()
{
}

void FXRSimulationInputDevice::Tick(float DeltaTime)
{
}

void FXRSimulationInputDevice::SendControllerEvents()
{
	if (InputSimHMD)
	{
		if (AXRSimulationActor* InputSimActor = InputSimHMD->GetSimulationActor())
		{
			SendHandEvents(InputSimActor, EControllerHand::Left);
			SendHandEvents(InputSimActor, EControllerHand::Right);
		}
	}
}

void FXRSimulationInputDevice::SendHandEvents(const AXRSimulationActor* InputSimActor, EControllerHand Hand)
{
	bool& bSelectPressedState = (Hand == EControllerHand::Left ? bLeftSelectPressed : bRightSelectPressed);
	bool& bGripPressedState = (Hand == EControllerHand::Left ? bLeftGripPressed : bRightGripPressed);
	const FKey& SelectKey = (Hand == EControllerHand::Left ? FXRSimulationKeys::LeftSelect : FXRSimulationKeys::RightSelect);
	const FKey& GripKey = (Hand == EControllerHand::Left ? FXRSimulationKeys::LeftGrip : FXRSimulationKeys::RightGrip);

	bool bNewSelectPressed, bNewGripPressed;
	InputSimActor->GetControllerActionState(Hand, bNewSelectPressed, bNewGripPressed);

	if (bNewSelectPressed && !bSelectPressedState)
	{
		bSelectPressedState = true;
		MessageHandler->OnControllerButtonPressed(SelectKey.GetFName(), 0, /*IsRepeat =*/false);
	}
	if (!bNewSelectPressed && bSelectPressedState)
	{
		bSelectPressedState = false;
		MessageHandler->OnControllerButtonReleased(SelectKey.GetFName(), 0, /*IsRepeat =*/false);
	}

	if (bNewGripPressed && !bGripPressedState)
	{
		bGripPressedState = true;
		MessageHandler->OnControllerButtonPressed(GripKey.GetFName(), 0, /*IsRepeat =*/false);
	}
	if (!bNewGripPressed && bGripPressedState)
	{
		bGripPressedState = false;
		MessageHandler->OnControllerButtonReleased(GripKey.GetFName(), 0, /*IsRepeat =*/false);
	}
}

void FXRSimulationInputDevice::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
	MessageHandler = InMessageHandler;
}

bool FXRSimulationInputDevice::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

void FXRSimulationInputDevice::SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value)
{
}

void FXRSimulationInputDevice::SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values)
{
}
