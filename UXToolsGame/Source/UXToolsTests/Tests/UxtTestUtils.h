#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

class FUxtTestHandTracker;
class IUxtHandTracker;
class UTestTouchPointerTarget;
class UUxtNearPointerComponent;
class UWorld;

/** Utility functions for UXTools testing. */
struct UxtTestUtils
{
public:

	// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
	// Marked as temporary there, hence, this one is temporary, too.
	static UWorld* GetTestWorld();

	static UWorld* CreateTestWorld();

	/** Create a basic touch pointer actor. */
	static UUxtNearPointerComponent* CreateTouchPointer(UWorld *World, FName Name, const FVector &Position, bool IsGrasped = false, bool AddMeshVisualizer = true);

	/** Create a touchable target actor. */
	static UTestTouchPointerTarget* CreateTouchPointerTarget(UWorld *World, const FVector &Location, const FString &meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube"), float meshScale = 1.0f);

	/** Create a background target without a scene component or primitive. */
	static UTestTouchPointerTarget* CreateTouchPointerBackgroundTarget(UWorld* World);


	//
	// Hand tracker implementation with mutable state for testing.

	/** Get the instance of the testing hand tracker. */
	static FUxtTestHandTracker& GetTestHandTracker();

	/** Replace the default hand tracker with a testing implementing. */
	static FUxtTestHandTracker& EnableTestHandTracker();

	/** Restore the default hand tracker implementation. */
	static void DisableTestHandTracker();

public:

	/** Hand tracker implementation for tests. */
	static FUxtTestHandTracker TestHandTracker;

	/** Cached hand tracker implementation to restore after tests are completed. */
	static IUxtHandTracker* MainHandTracker;
};

/** Latent command to ensure the hand tracker is restored after a test is completed */
DEFINE_LATENT_AUTOMATION_COMMAND(FUxtDisableTestHandTrackerCommand);
