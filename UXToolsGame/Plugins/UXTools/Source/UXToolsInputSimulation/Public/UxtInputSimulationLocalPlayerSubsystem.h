// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "WindowsMixedRealityHandTrackingTypes.h"
#include "UxtInputSimulationLocalPlayerSubsystem.generated.h"

class AGameModeBase;
class APlayerController;
class UCameraComponent;

/** Subsystem that creates an actor for simulation when a game is started. */
UCLASS(ClassGroup = UXTools)
class UXTOOLSINPUTSIMULATION_API UUxtInputSimulationLocalPlayerSubsystem
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

	void CreateActors(UWorld* World);
	void CreateInputSimActor(UWorld* World);
	void CreateHmdCameraActor(UWorld* World);

	void DestroyInputSimActor();
	void DestroyHmdCameraActor();

	void SetPlayerCameraTarget(APlayerController* PlayerController);

	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);

	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

private:

	/** Primary actor that performs input simulation and stores resulting data in the input simulation engine subsystem. */
	TWeakObjectPtr<AActor> InputSimActorWeak;

	/** Actor with a camera component that acts as the view target.
	 * This is needed because in a non-stereo viewport the camera will not use the XRSystem HMD position on its own.
	 * Using a separate camera component will use the HMD position though, even if the camera itself is not moved.
	 */
	TWeakObjectPtr<AActor> HmdCameraActorWeak;

};
