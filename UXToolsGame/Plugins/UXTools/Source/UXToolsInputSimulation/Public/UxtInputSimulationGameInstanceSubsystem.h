// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "WindowsMixedRealityHandTrackingTypes.h"
#include "UxtInputSimulationGameInstanceSubsystem.generated.h"

class AGameModeBase;
class APlayerController;
class UCameraComponent;

/** Subsystem that creates an actor for simulation when a game is started. */
UCLASS(ClassGroup = UXTools)
class UXTOOLSINPUTSIMULATION_API UUxtInputSimulationGameInstanceSubsystem
	: public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	//
	// USubsystem implementation

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:

	void CreateInputSimActor();
	void DestroyInputSimActor();

	void CreateHmdCameraActor();
	void DestroyHmdCameraActor();

	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);


private:

	/** Primary actor that performs input simulation and stores resulting data in the input simulation engine subsystem. */
	AActor* InputSimActor;

	/** Actor with a camera component that acts as the view target.
	 * This is needed because in a non-stereo viewport the camera will not use the XRSystem HMD position on its own.
	 * Using a separate camera component will use the HMD position though, even if the camera itself is not moved.
	 */
	AActor* HmdCameraActor;
	/** Camera component for the HMD camera actor. */
	UCameraComponent* HmdCameraComponent;

};
