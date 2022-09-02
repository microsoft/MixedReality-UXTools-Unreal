// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

class FUxtTestHandTracker;
class IUxtHandTracker;
class UTestGrabTarget;
class UTestPokeTarget;
class UUxtNearPointerComponent;
class UUxtGrabTargetComponent;
class UUxtFarPointerComponent;

/** Utility functions for UXTools testing. */
struct UxtTestUtils
{
public:
	/** Loads the given map. Returns null in case of failure. */
	static UWorld* LoadMap(const FString& MapName);

	/** To be called at the end of each test to make sure the current test map is unloaded. */
	static void ExitGame();

	// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
	// Marked as temporary there, hence, this one is temporary, too.
	static UWorld* GetTestWorld();

	static UWorld* CreateTestWorld();

	/** Creates a static mesh with the asset and scale specified (defaults to a cube with unscaled side length of 100). Does not attach or
	 * register it. */
	static UStaticMeshComponent* CreateStaticMesh(
		AActor* Owner, FVector Scale = FVector::OneVector, const FString& AssetReference = TEXT("/Engine/BasicShapes/Cube.Cube"));

	/** Create a basic near pointer actor. */
	static UUxtNearPointerComponent* CreateNearPointer(
		UWorld* World, FName Name, const FVector& Position, EControllerHand Hand = EControllerHand::Right, bool IsGrasped = false,
		bool AddMeshVisualizer = true);

	/** Create a basic far pointer actor. */
	static UUxtFarPointerComponent* CreateFarPointer(
		UWorld* World, FName Name, const FVector& Position, EControllerHand Hand = EControllerHand::Right, bool IsGrasped = false);

	/** Create a grabbable target actor. */
	static UTestGrabTarget* CreateNearPointerGrabTarget(
		UWorld* World, const FVector& Location, const FString& meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube"),
		float meshScale = 1.0f);

	/** Create a test box actor with attached TestComponent*/
	template <typename TestComponent>
	static TestComponent* CreateTestMeshWithComponent(
		UWorld* World, const FVector& Location, const FString& AssetReference = TEXT("/Engine/BasicShapes/Cube.Cube"));

	/** Create a pokable target actor. */
	static UTestPokeTarget* CreateNearPointerPokeTarget(
		UWorld* World, const FVector& Location, const FString& meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube"),
		float meshScale = 1.0f);

	/** Create a background target without a scene component or primitive. */
	static UTestGrabTarget* CreateNearPointerBackgroundTarget(UWorld* World);

	/** Set the simulated head pose location to the given location. */
	static void SetTestHeadLocation(const FVector& Location);

	/** Set the simulated head pose rotation to the given rotation. */
	static void SetTestHeadRotation(const FRotator& Rotation);

	/** Creates a test camera that is set as the new view target in the playercontroller.
		Transform modifications on the returned USceneComponent will transform the active test camera.*/
	static USceneComponent* CreateTestCamera(UWorld* World);
	//
	// Hand tracker implementation with mutable state for testing.

	/** Get the instance of the testing hand tracker. */
	static FUxtTestHandTracker& GetTestHandTracker();

	/** Enable the test head and hand trackers. */
	static void EnableTestInputSystem();

	/** Disable the test head and hand trackers. */
	static void DisableTestInputSystem();

public:
	/** Hand tracker implementation for tests. */
	static FUxtTestHandTracker TestHandTracker;

	/** Cached hand tracker implementation to restore after tests are completed. */
	static IUxtHandTracker* MainHandTracker;
};

/** Latent command to ensure the hand tracker is restored after a test is completed */
DEFINE_LATENT_AUTOMATION_COMMAND(FUxtDisableTestHandTrackerCommand);

template <typename TestComponent>
TestComponent* UxtTestUtils::CreateTestMeshWithComponent(UWorld* World, const FVector& Location, const FString& AssetReference)
{
	AActor* TargetActor = World->SpawnActor<AActor>();

	// Box Mesh
	UStaticMeshComponent* MeshComponent = UxtTestUtils::CreateStaticMesh(TargetActor, FVector::OneVector, AssetReference);
	TargetActor->SetRootComponent(MeshComponent);
	MeshComponent->RegisterComponent();

	// Add TestComponent
	TestComponent* TestTarget = NewObject<TestComponent>(TargetActor);
	TestTarget->RegisterComponent();

	TargetActor->SetActorLocation(Location);

	return TestTarget;
}
