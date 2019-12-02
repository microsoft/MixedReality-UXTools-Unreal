#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include <functional>
#include <vector>

class UTestTouchPointerTarget;
class UTouchPointer;
class UWorld;

/** Utility functions for MixedRealityUtils testing. */
struct MixedRealityTestUtils
{
	// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
	// Marked as temporary there, hence, this one is temporary, too.
	static UWorld* GetTestWorld();

	static UWorld* CreateTestWorld();

	/** Create a basic touch pointer actor. */
	static UTouchPointer* CreateTouchPointer(UWorld *World, const FVector &Position, bool IsGrasped = false, bool AddMeshVisualizer = true);

	/** Create a touchable target actor. */
	static UTestTouchPointerTarget* CreateTouchPointerTarget(UWorld *World, const FVector &Location, const FString &meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube"), float meshScale = 1.0f);
};
