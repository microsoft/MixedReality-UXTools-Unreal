// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "UxtDefaultHandTracker.h"

#include "Engine/EngineBaseTypes.h"
#include "Subsystems/EngineSubsystem.h"

#include "UxtDefaultHandTrackerSubsystem.generated.h"

class AController;
class AGameModeBase;

/** Subsystem for registering the default hand tracker.
 *
 * This subsystem creates the default hand tracker on player login.
 * It registers input action mappings and binds to input events for Select/Grip actions.
 * It also updates MotionControllerData of the default hand tracker once per world tick.
 */
UCLASS()
class UUxtDefaultHandTrackerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsSimulationEnabled() const { return bSimulationEnabled; }

private:
	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

	void OnWorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaTime);

	void OnLeftSelectPressed();
	void OnLeftSelectReleased();
	void OnLeftGripPressed();
	void OnLeftGripReleased();
	void OnRightSelectPressed();
	void OnRightSelectReleased();
	void OnRightGripPressed();
	void OnRightGripReleased();

	AXRSimulationActor* GetOrCreateInputSimActor(APlayerController* PlayerController);
	void DestroyInputSimActor();

	AActor* GetOrCreateHmdCameraActor(APlayerController* PlayerController);
	void DestroyHmdCameraActor();

private:
	FUxtDefaultHandTracker DefaultHandTracker;

	FDelegateHandle TickDelegateHandle;

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
