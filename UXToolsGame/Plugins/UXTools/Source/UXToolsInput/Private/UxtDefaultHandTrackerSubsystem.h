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

private:
	FUxtDefaultHandTracker DefaultHandTracker;

	FDelegateHandle TickDelegateHandle;

	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;
};
