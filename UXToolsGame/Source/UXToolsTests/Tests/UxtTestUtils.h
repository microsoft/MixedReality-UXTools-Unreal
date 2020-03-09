#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

class UTestTouchPointerTarget;
class UUxtTouchPointer;
class UWorld;

/** Utility functions for UXTools testing. */
struct UxtTestUtils
{
	// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
	// Marked as temporary there, hence, this one is temporary, too.
	static UWorld* GetTestWorld();

	static UWorld* CreateTestWorld();

	/** Create a basic touch pointer actor. */
	static UUxtTouchPointer* CreateTouchPointer(UWorld *World, const FVector &Position, bool IsGrasped = false, bool AddMeshVisualizer = true);

	/** Create a touchable target actor. */
	static UTestTouchPointerTarget* CreateTouchPointerTarget(UWorld *World, const FVector &Location, const FString &meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube"), float meshScale = 1.0f);

	/** Create a background target without a scene component or primitive. */
	static UTestTouchPointerTarget* CreateTouchPointerBackgroundTarget(UWorld* World);
};
