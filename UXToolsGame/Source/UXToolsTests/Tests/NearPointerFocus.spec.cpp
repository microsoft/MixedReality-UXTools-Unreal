// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "PointerTestSequence.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

using namespace UxtPointerTests;

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(NearPointerFocusSpec, "UXTools.NearPointer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	PointerTestSequence Sequence;

	const int NumPointers = 2;
	const FVector pStart = FVector(40, -50, 30);
	const FVector pEnd = FVector(150, 40, -40);

END_DEFINE_SPEC(NearPointerFocusSpec)

void NearPointerFocusSpec::Define()
{
	Describe("Near pointer focus", [this]
		{
			BeforeEach([this]
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();

					UxtTestUtils::EnableTestHandTracker();

					Sequence.Init(World, NumPointers);

					// Register all new components.
					World->UpdateWorldComponents(false, false);
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();

					Sequence.Reset();

					// Force GC so that destroyed actors are removed from the world.
					// Running multiple tests will otherwise cause errors when creating duplicate actors.
					GEngine->ForceGarbageCollection();
				});

			LatentIt("should have no focus without targets", [this](const FDoneDelegate& Done)
				{
					Sequence.AddMovementKeyframe(pStart);
					Sequence.ExpectFocusTargetNone();

					Sequence.EnqueueFrames(this, Done);
				});

			LatentIt("should focus single target", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					FVector p1(120, -20, -5);
					Sequence.AddTarget(World, p1);

					Sequence.AddMovementKeyframe(pStart);
					Sequence.ExpectFocusTargetNone();
					Sequence.AddMovementKeyframe(p1);
					Sequence.ExpectFocusTargetIndex(0);
					Sequence.AddMovementKeyframe(pEnd);
					Sequence.ExpectFocusTargetNone();

					Sequence.EnqueueFrames(this, Done);
				});

			LatentIt("should focus two separate targets", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					FVector p1(120, -40, -5);
					FVector p2(100, 30, 15);
					Sequence.AddTarget(World, p1);
					Sequence.AddTarget(World, p2);

					Sequence.AddMovementKeyframe(pStart);
					Sequence.ExpectFocusTargetNone();
					Sequence.AddMovementKeyframe(p1);
					Sequence.ExpectFocusTargetIndex(0);
					Sequence.AddMovementKeyframe(p2);
					Sequence.ExpectFocusTargetIndex(1);
					Sequence.AddMovementKeyframe(pEnd);
					Sequence.ExpectFocusTargetNone();

					Sequence.EnqueueFrames(this, Done);
				});

			LatentIt("should focus two overlapping targets", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					FVector p1(110, 4, -5);
					FVector p2(115, 12, -2);
					Sequence.AddTarget(World, p1);
					Sequence.AddTarget(World, p2);

					Sequence.AddMovementKeyframe(pStart);
					Sequence.ExpectFocusTargetNone();
					Sequence.AddMovementKeyframe(p1 + FVector(0, -10, 0));
					Sequence.ExpectFocusTargetIndex(0);
					Sequence.AddMovementKeyframe(p2 + FVector(0, 10, 0));
					Sequence.ExpectFocusTargetIndex(1);
					Sequence.AddMovementKeyframe(pEnd);
					Sequence.ExpectFocusTargetNone();

					Sequence.EnqueueFrames(this, Done);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
