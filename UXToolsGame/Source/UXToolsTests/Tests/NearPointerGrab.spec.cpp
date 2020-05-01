// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
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


BEGIN_DEFINE_SPEC(NearPointerFocusLostSpec, "UXTools.NearPointer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	AUxtHandInteractionActor* HandActor;
	UTestGrabTarget* Target;
	FFrameQueue FrameQueue;

	const FString& TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
	const float TargetScale = 0.3f;
	const FVector TargetLocation = FVector(120, -20, -5);

END_DEFINE_SPEC(NearPointerFocusLostSpec)

void NearPointerFocusLostSpec::Define()
{
	Describe("Near pointer", [this]
		{
			BeforeEach([this]()
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(World->GetGameInstance()->TimerManager);

					UxtTestUtils::EnableTestHandTracker();
					// Set hand position so the near pointer is activated
					UxtTestUtils::GetTestHandTracker().TestPosition = TargetLocation + FVector(-15, 0, 0);

					HandActor = World->SpawnActor<AUxtHandInteractionActor>();
					Target = UxtTestUtils::CreateNearPointerTarget(World, TargetLocation, TargetFilename, TargetScale);
					// Focus lock is also tested, enable so the target locks focus on the pointer when grabbed.
					Target->bUseFocusLock = true;

					// Register all new components.
					World->UpdateWorldComponents(false, false);
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();

					FrameQueue.Reset();
					HandActor->Destroy();
					HandActor = nullptr;
					Target->GetOwner()->Destroy();
					Target = nullptr;

					// Force GC so that destroyed actors are removed from the world.
					// Running multiple tests will otherwise cause errors when creating duplicate actors.
					GEngine->ForceGarbageCollection();
				});

			LatentIt("should end grab when tracking is lost", [this](const FDoneDelegate& Done)
				{
					// Have to skip two frames for the hand actor to detect near pointer activation
					FrameQueue.Skip();
					FrameQueue.Skip();

					FrameQueue.Enqueue([this]
						{
							UUxtNearPointerComponent* NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();

							// Near pointer should now be active
							TestTrue(TEXT("Near pointer active"), NearPointer->IsActive());
							// Ensure target is focused
							FVector GrabLocation;
							TestEqual(TEXT("Grab Target"), NearPointer->GetFocusedGrabTarget(GrabLocation), (UObject*)Target);
							TestEqual(TEXT("Begin Focus Count"), Target->BeginFocusCount, 1);
							TestEqual(TEXT("End Focus Count"), Target->EndFocusCount, 0);
							TestEqual(TEXT("Begin Grab Count"), Target->BeginGrabCount, 0);
							TestEqual(TEXT("End Grab Count"), Target->EndGrabCount, 0);

							// Enable grab.
							// Wait one tick so the pointer can update overlaps and raise events.
							UxtTestUtils::GetTestHandTracker().bIsGrabbing = true;
						});

					FrameQueue.Enqueue([this, Done]
						{
							UUxtNearPointerComponent* NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();

							// Ensure target has been grabbed.
							FVector GrabLocation;
							TestEqual(TEXT("Grab Target"), NearPointer->GetFocusedGrabTarget(GrabLocation), (UObject*)Target);
							TestEqual(TEXT("Begin Focus Count"), Target->BeginFocusCount, 1);
							TestEqual(TEXT("End Focus Count"), Target->EndFocusCount, 0);
							TestEqual(TEXT("Begin Grab Count"), Target->BeginGrabCount, 1);
							TestEqual(TEXT("End Grab Count"), Target->EndGrabCount, 0);
							// Ensure focus lock has been enabled.
							TestTrue(TEXT("Focus lock enabled"), NearPointer->GetFocusLocked());

							// Simulate tracking loss
							UxtTestUtils::GetTestHandTracker().bIsTracked = false;
						});

					FrameQueue.Enqueue([this, Done]
						{
							UUxtNearPointerComponent* NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();

							// Pointer component should be deactivated by the hand interaction actor when tracking is lost
							TestFalse(TEXT("Near pointer active"), NearPointer->IsActive());

							// Ensure target has been released.
							FVector GrabLocation;
							TestNull(TEXT("Grab Target"), NearPointer->GetFocusedGrabTarget(GrabLocation));
							TestEqual(TEXT("Begin Focus Count"), Target->BeginFocusCount, 1);
							TestEqual(TEXT("End Focus Count"), Target->EndFocusCount, 1);
							TestEqual(TEXT("Begin Grab Count"), Target->BeginGrabCount, 1);
							TestEqual(TEXT("End Grab Count"), Target->EndGrabCount, 1);
							// Ensure focus lock has been enabled.
							TestFalse(TEXT("Focus lock enabled"), NearPointer->GetFocusLocked());

							Done.Execute();
						});
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
