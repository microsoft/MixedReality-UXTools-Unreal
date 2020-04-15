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

BEGIN_DEFINE_SPEC(NearPointerGrabSpec, "UXTools.NearPointer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	PointerTestSequence Sequence;

	const int NumPointers = 2;
	const FVector pTarget = FVector(120, -20, -5);
	const FVector pInside = FVector(113, -24, -8);
	const FVector pOutside = FVector(150, 40, -40);

END_DEFINE_SPEC(NearPointerGrabSpec)

void NearPointerGrabSpec::Define()
{
	Describe("Near pointer", [this]
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


			LatentIt("should focus target when overlapping initially", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					Sequence.AddTarget(World, pTarget);

					Sequence.AddMovementKeyframe(pInside);
					Sequence.ExpectFocusTargetIndex(0);

					Sequence.AddGrabKeyframe(true);

					Sequence.AddGrabKeyframe(false);

					Sequence.EnqueueFrames(this, Done);
				});

			LatentIt("should focus target when entering", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					Sequence.AddTarget(World, pTarget);

					Sequence.AddMovementKeyframe(pOutside);
					Sequence.ExpectFocusTargetNone();

					Sequence.AddMovementKeyframe(pInside);
					Sequence.ExpectFocusTargetIndex(0);

					Sequence.AddGrabKeyframe(true);

					Sequence.AddMovementKeyframe(pOutside);
					Sequence.ExpectFocusTargetNone();

					Sequence.AddGrabKeyframe(false);

					Sequence.EnqueueFrames(this, Done);
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
