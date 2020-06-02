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
#include "UxtTestTargetComponent.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

struct PointerTargetState
{
	UTestGrabTarget* Target;

	/** Expected event counts */
	int BeginFocusCount = 0;
	int EndFocusCount = 0;
	int BeginGrabCount = 0;
	int EndGrabCount = 0;
};

BEGIN_DEFINE_SPEC(NearPointerPokeSpec, "UXTools.NearPointer", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	void AddTarget(const FVector& Location);

	void TestKeyframe();

	/**
	 * Move pointer to new location and test for focus changes in the next frame.
	 * NewFocusTargetIndex is the expected focus target.
	 * Changed focus target is expected to increment event counters accordingly.
	 * Target index -1 means no target is focused.
	 */
	void AddMovementKeyframe(const FVector& PointerLocation);
	void ExpectFocusTargetIndex(int NewFocusTargetIndex);
	void ExpectFocusTargetNone();
	void AddGrabKeyframe(bool bEnableGrab);

	FFrameQueue FrameQueue;
	TArray<UUxtNearPointerComponent*> Pointers;
	TArray<PointerTargetState> Targets;
	int CurrentFocusTargetIndex = -1;
	bool bCurrentGrabbing = false;

	const int NumPointers = 2;
	const FVector TargetLocation = FVector(120, -20, -5);
	const FVector InsideTargetLocation = FVector(113, -24, -8);
	const FVector OutsideTargetLocation = FVector(150, 40, -40);
	const FVector FocusStartLocation = FVector(40, -50, 30);
	const FVector FocusEndLocation = FVector(150, 40, -40);

END_DEFINE_SPEC(NearPointerPokeSpec)

void NearPointerPokeSpec::AddTarget(const FVector& Location)
{
	UWorld* World = UxtTestUtils::GetTestWorld();
	const FString& targetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
	const float targetScale = 0.3f;
	PointerTargetState TargetState;
	TargetState.Target = UxtTestUtils::CreateNearPointerGrabTarget(World, Location, targetFilename, targetScale);
	Targets.Add(TargetState);
}

/** Test event counts for all targets. */
void NearPointerPokeSpec::TestKeyframe()
{
	for (int TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
	{
		const PointerTargetState& TargetState = Targets[TargetIndex];

		FString whatFocusStarted; whatFocusStarted.Appendf(TEXT("Target %d EnterFocus count"), TargetIndex);
		FString whatFocusEnded; whatFocusEnded.Appendf(TEXT("Target %d ExitFocus count"), TargetIndex);
		FString whatGraspStarted; whatGraspStarted.Appendf(TEXT("Target %d BeginGrab count"), TargetIndex);
		FString whatGraspEnded; whatGraspEnded.Appendf(TEXT("Target %d EndGrab count"), TargetIndex);
		TestEqual(whatFocusStarted, TargetState.Target->BeginFocusCount, TargetState.BeginFocusCount);
		TestEqual(whatFocusEnded, TargetState.Target->EndFocusCount, TargetState.EndFocusCount);
		TestEqual(whatGraspStarted, TargetState.Target->BeginGrabCount, TargetState.BeginGrabCount);
		TestEqual(whatGraspEnded, TargetState.Target->EndGrabCount, TargetState.EndGrabCount);
	}
}

void NearPointerPokeSpec::AddMovementKeyframe(const FVector& PointerLocation)
{
	FrameQueue.Enqueue([PointerLocation]
		{
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(PointerLocation);
		});
}

void NearPointerPokeSpec::ExpectFocusTargetIndex(int NewFocusTargetIndex)
{
	FrameQueue.Enqueue([this, NewFocusTargetIndex]
	{
		// Focus changed: Increment EndFocusCount of the previous target and the BeginGrabCount of the new target.
		if (NewFocusTargetIndex != CurrentFocusTargetIndex)
		{
			for (int i = 0; i < Targets.Num(); ++i)
			{
				if (i == CurrentFocusTargetIndex)
				{
					Targets[i].EndFocusCount += Pointers.Num();
				}
				if (i == NewFocusTargetIndex)
				{
					Targets[i].BeginFocusCount += Pointers.Num();
				}
			}

			CurrentFocusTargetIndex = NewFocusTargetIndex;
		}

		// Test state of targets after update from the previous frame.
		TestKeyframe();
	});
}

void NearPointerPokeSpec::ExpectFocusTargetNone()
{
	ExpectFocusTargetIndex(-1);
}

void NearPointerPokeSpec::AddGrabKeyframe(bool bEnableGrab)
{
	FrameQueue.Enqueue([bEnableGrab]
		{
			UxtTestUtils::GetTestHandTracker().SetGrabbing(bEnableGrab);
		});

	FrameQueue.Enqueue([this, bEnableGrab]
		{
			if (bCurrentGrabbing != bEnableGrab)
			{
				// Increment EndFocusCount of the current target when released and the BeginFocusCount when grabbed.
				for (int i = 0; i < Targets.Num(); ++i)
				{
					if (i == CurrentFocusTargetIndex)
					{
						if (bEnableGrab)
						{
							Targets[i].BeginGrabCount += Pointers.Num();
						}
						else
						{
							Targets[i].EndGrabCount += Pointers.Num();
						}
					}
				}
			}
		});
}

void NearPointerPokeSpec::Define()
{
	Describe("Near pointer", [this]
		{
			BeforeEach([this]
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));
					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(World->GetGameInstance()->TimerManager);
					UxtTestUtils::EnableTestHandTracker();

					Pointers.SetNum(NumPointers);
					for (int i = 0; i < NumPointers; ++i)
					{
						Pointers[i] = UxtTestUtils::CreateNearPointer(World, *FString::Printf(TEXT("TestPointer%d"), i), FVector::ZeroVector);
					}

					// Register all new components.
					World->UpdateWorldComponents(false, false);
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();

					for (UUxtNearPointerComponent* Pointer : Pointers)
					{
						Pointer->GetOwner()->Destroy();
					}
					Pointers.Empty();

					for (PointerTargetState& TargetState : Targets)
					{
						TargetState.Target->GetOwner()->Destroy();
					}
					Targets.Empty();
					CurrentFocusTargetIndex = -1;
					bCurrentGrabbing = false;

					FrameQueue.Reset();

					// Force GC so that destroyed actors are removed from the world.
					// Running multiple tests will otherwise cause errors when creating duplicate actors.
					GEngine->ForceGarbageCollection();
				});


			LatentIt("should focus poke target when overlapping initially", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					AddTarget(TargetLocation);

					AddMovementKeyframe(InsideTargetLocation);
					ExpectFocusTargetIndex(0);

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should focus poke target when entering", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					AddTarget(TargetLocation);

					AddMovementKeyframe(OutsideTargetLocation);
					ExpectFocusTargetNone();

					AddMovementKeyframe(InsideTargetLocation);
					ExpectFocusTargetIndex(0);

					AddMovementKeyframe(OutsideTargetLocation);
					ExpectFocusTargetNone();

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should have no focus without targets", [this](const FDoneDelegate& Done)
				{
					AddMovementKeyframe(FocusStartLocation);
					ExpectFocusTargetNone();

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should focus single target", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					FVector p1(120, -20, -5);
					AddTarget(p1);

					AddMovementKeyframe(FocusStartLocation);
					ExpectFocusTargetNone();
					AddMovementKeyframe(p1);
					ExpectFocusTargetIndex(0);
					AddMovementKeyframe(FocusEndLocation);
					ExpectFocusTargetNone();

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should focus two separate targets", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					FVector p1(120, -40, -5);
					FVector p2(100, 30, 15);
					AddTarget(p1);
					AddTarget(p2);

					AddMovementKeyframe(FocusStartLocation);
					ExpectFocusTargetNone();
					AddMovementKeyframe(p1);
					ExpectFocusTargetIndex(0);
					AddMovementKeyframe(p2);
					ExpectFocusTargetIndex(1);
					AddMovementKeyframe(FocusEndLocation);
					ExpectFocusTargetNone();

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should focus two overlapping targets", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					FVector p1(110, 4, -5);
					FVector p2(115, 12, -2);
					AddTarget(p1);
					AddTarget(p2);

					AddMovementKeyframe(FocusStartLocation);
					ExpectFocusTargetNone();
					AddMovementKeyframe(p1 + FVector(0, -10, 0));
					ExpectFocusTargetIndex(0);
					AddMovementKeyframe(p2 + FVector(0, 10, 0));
					ExpectFocusTargetIndex(1);
					AddMovementKeyframe(FocusEndLocation);
					ExpectFocusTargetNone();

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should focus grab target when overlapping initially", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					AddTarget(TargetLocation);

					AddMovementKeyframe(InsideTargetLocation);
					ExpectFocusTargetIndex(0);

					AddGrabKeyframe(true);

					AddGrabKeyframe(false);

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should focus grab target when entering", [this](const FDoneDelegate& Done)
				{
					UWorld* World = UxtTestUtils::GetTestWorld();
					AddTarget(TargetLocation);

					AddMovementKeyframe(OutsideTargetLocation);
					ExpectFocusTargetNone();

					AddMovementKeyframe(InsideTargetLocation);
					ExpectFocusTargetIndex(0);

					AddGrabKeyframe(true);

					AddMovementKeyframe(OutsideTargetLocation);
					ExpectFocusTargetNone();

					AddGrabKeyframe(false);

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
