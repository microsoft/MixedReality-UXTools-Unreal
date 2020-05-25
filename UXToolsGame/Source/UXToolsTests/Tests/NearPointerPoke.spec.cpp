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

BEGIN_DEFINE_SPEC(NearPointerPokeSpec, "UXTools.NearPointer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	PointerTestSequence Sequence;

	const int NumPointers = 2;
	const FVector pTarget = FVector(120, -20, -5);
	const FVector pInside = FVector(113, -24, -8);
	const FVector pOutside = FVector(150, 40, -40);

END_DEFINE_SPEC(NearPointerPokeSpec)

void NearPointerPokeSpec::Define()
{
	Describe("Near pointer poke", [this]
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


			LatentIt("should focus poke target when overlapping initially", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					Sequence.AddTarget(World, pTarget);

					Sequence.AddMovementKeyframe(pInside);
					Sequence.ExpectFocusTargetIndex(0);

					Sequence.EnqueueFrames(this, Done);
				});

			LatentIt("should focus poke target when entering", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					Sequence.AddTarget(World, pTarget);

					Sequence.AddMovementKeyframe(pOutside);
					Sequence.ExpectFocusTargetNone();

					Sequence.AddMovementKeyframe(pInside);
					Sequence.ExpectFocusTargetIndex(0);

					Sequence.AddMovementKeyframe(pOutside);
					Sequence.ExpectFocusTargetNone();

					Sequence.EnqueueFrames(this, Done);
				});
		});
}


BEGIN_DEFINE_SPEC(NearPointerPokeFocusLostSpec, "UXTools.NearPointer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	AUxtHandInteractionActor* HandActor;
	UTestPokeTarget* Target;
	FFrameQueue FrameQueue;

	const FString TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
	const float TargetScale = 0.3f;
	const FVector TargetLocation = FVector(120, -20, -5);

END_DEFINE_SPEC(NearPointerPokeFocusLostSpec)

void NearPointerPokeFocusLostSpec::Define()
{
	Describe("Near pointer poke", [this]
		{
			BeforeEach([this]()
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(World->GetGameInstance()->TimerManager);

					UxtTestUtils::EnableTestHandTracker();
					// Set hand position so the near pointer is activated
					UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TargetLocation + FVector(-25, 0, 0));

					HandActor = World->SpawnActor<AUxtHandInteractionActor>();
					Target = UxtTestUtils::CreateNearPointerPokeTarget(World, TargetLocation, TargetFilename, TargetScale);
					// Focus lock is also tested, enable so the target locks focus on the pointer when poked.
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

			LatentIt("should end poke when tracking is lost", [this](const FDoneDelegate& Done)
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
							FVector PokeLocation;
							TestEqual(TEXT("Poke Target"), NearPointer->GetFocusedPokeTarget(PokeLocation), (UObject*)Target);
							TestEqual(TEXT("Begin Focus Count"), Target->BeginFocusCount, 1);
							TestEqual(TEXT("End Focus Count"), Target->EndFocusCount, 0);
							TestEqual(TEXT("Begin Poke Count"), Target->BeginPokeCount, 0);
							TestEqual(TEXT("End Poke Count"), Target->EndPokeCount, 0);

							// Poke the target.
							// Wait one tick so the pointer can update overlaps and raise events.
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TargetLocation);
						});

					FrameQueue.Enqueue([this, Done]
						{
							UUxtNearPointerComponent* NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();

							// Ensure target has been poked.
							FVector PokeLocation;
							TestEqual(TEXT("Poke Target"), NearPointer->GetFocusedPokeTarget(PokeLocation), (UObject*)Target);
							TestEqual(TEXT("Begin Focus Count"), Target->BeginFocusCount, 1);
							TestEqual(TEXT("End Focus Count"), Target->EndFocusCount, 0);
							TestEqual(TEXT("Begin Poke Count"), Target->BeginPokeCount, 1);
							TestEqual(TEXT("End Poke Count"), Target->EndPokeCount, 0);
							// Ensure focus lock has been enabled.
							TestTrue(TEXT("Focus lock enabled"), NearPointer->GetFocusLocked());

							// Simulate tracking loss
							UxtTestUtils::GetTestHandTracker().SetTracked(false);
						});

					FrameQueue.Enqueue([this, Done]
						{
							UUxtNearPointerComponent* NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();

							// Pointer component should be deactivated by the hand interaction actor when tracking is lost
							TestFalse(TEXT("Near pointer active"), NearPointer->IsActive());

							// Ensure target has been released.
							FVector PokeLocation;
							TestNull(TEXT("Poke Target"), NearPointer->GetFocusedPokeTarget(PokeLocation));
							TestEqual(TEXT("Begin Focus Count"), Target->BeginFocusCount, 1);
							TestEqual(TEXT("End Focus Count"), Target->EndFocusCount, 1);
							TestEqual(TEXT("Begin Poke Count"), Target->BeginPokeCount, 1);
							TestEqual(TEXT("End Poke Count"), Target->EndPokeCount, 1);
							// Ensure focus lock has been enabled.
							TestFalse(TEXT("Focus lock enabled"), NearPointer->GetFocusLocked());

							Done.Execute();
						});
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
