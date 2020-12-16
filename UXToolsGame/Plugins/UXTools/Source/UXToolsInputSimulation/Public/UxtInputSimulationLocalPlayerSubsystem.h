// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "WindowsMixedRealityHandTrackingTypes.h"

#include "Subsystems/LocalPlayerSubsystem.h"

#include "UxtInputSimulationLocalPlayerSubsystem.generated.h"

class AGameModeBase;
class AController;
class APlayerController;
class UCameraComponent;
class UUxtInputSimulationState;

/** Subsystem that creates an actor for simulation when a game is started. */
UCLASS(ClassGroup = "UXTools")
class UXTOOLSINPUTSIMULATION_API UUxtInputSimulationLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/** Get the persistent simulation state */
	UFUNCTION(BlueprintGetter, Category = "Uxt Input Simulation|Local Player")
	UUxtInputSimulationState* GetSimulationState() const;

	//
	// USubsystem implementation

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void CreateInputSimActor(UWorld* World);
	void CreateHmdCameraActor(UWorld* World);

	void DestroyInputSimActor();
	void DestroyHmdCameraActor();

	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

private:
	/** Primary actor that performs input simulation and stores resulting data in the input simulation engine subsystem. */
	TWeakObjectPtr<AActor> InputSimActorWeak;

	/** Actor with a camera component that acts as the view target.
	 * This is needed because in a non-stereo viewport the camera will not use the XRSystem HMD position on its own.
	 * Using a separate camera component will use the HMD position though, even if the camera itself is not moved.
	 */
	TWeakObjectPtr<AActor> HmdCameraActorWeak;

	UPROPERTY(Category = "Uxt Input Simulation|Local Player", BlueprintGetter = GetSimulationState)
	UUxtInputSimulationState* SimulationState;
};
