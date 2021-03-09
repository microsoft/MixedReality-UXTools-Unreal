// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "HeadMountedDisplayTypes.h"
#include "XRSimulationState.h"

#include "Subsystems/LocalPlayerSubsystem.h"

#include "UxtXRSimulationSubsystem.generated.h"

class AXRSimulationActor;

/** Subsystem for managing XR simulation actors. */
UCLASS()
class UUxtXRSimulationSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsSimulationEnabled() const { return bSimulationEnabled; }

	bool GetMotionControllerData(
		EControllerHand Hand, FXRMotionControllerData& OutMotionControllerData, bool& OutSelectPressed, bool& OutGrabbing) const;

	bool GetHeadPose(FQuat& OutHeadRotation, FVector& OutHeadPosition) const;

private:
	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

	AXRSimulationActor* GetOrCreateInputSimActor(APlayerController* PlayerController);
	void DestroyInputSimActor();

	AActor* GetOrCreateHmdCameraActor(APlayerController* PlayerController);
	void DestroyHmdCameraActor();

private:
	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;

	/** True if input simulation is enabled.
	 * Data from the simulation actor will be used for hand tracking instead of the XRTrackingSystem API.
	 */
	bool bSimulationEnabled = false;

	/** Actor that handles user input to simulate XR. */
	TWeakObjectPtr<AXRSimulationActor> SimulationActorWeak;

	/** Simulation state for the input sim actor.
	 * This is cached by the HMD to keep it persistent across map loads.
	 */
	TSharedPtr<FXRSimulationState> SimulationState;

	/** Actor with a camera component that acts as the view target.
	 * This is needed because in a non-stereo viewport the camera will not use the XRSystem HMD position on its own.
	 * Using a separate camera component will use the HMD position though, even if the camera itself is not moved.
	 */
	TWeakObjectPtr<AActor> HmdCameraActorWeak;

	TSharedPtr<class FUxtXRSimulationViewExtension, ESPMode::ThreadSafe> XRSimulationViewExtension;
};
