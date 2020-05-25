// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "PointerTestSequence.h"

#include "Input/UxtNearPointerComponent.h"
#include "UxtTestUtils.h"
#include "UxtTestHandTracker.h"

#include "Components/PrimitiveComponent.h"

void UTestGrabTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
}

void UTestGrabTarget::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestGrabTarget::OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestGrabTarget::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

bool UTestGrabTarget::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return true;
}

void UTestGrabTarget::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginGrabCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(true);
	}
}

void UTestGrabTarget::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndGrabCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(false);
	}
}

void UTestPokeTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
}

void UTestPokeTarget::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestPokeTarget::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestPokeTarget::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

EUxtPokeBehaviour UTestPokeTarget::GetPokeBehaviour() const
{
	return EUxtPokeBehaviour::FrontFace;
}

bool UTestPokeTarget::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return true;
}

void UTestPokeTarget::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginPokeCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(true);
	}
}

void UTestPokeTarget::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndPokeCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(false);
	}
}

namespace UxtPointerTests
{

	void PointerTestSequence::Init(UWorld* World, int NumPointers)
	{
		FrameQueue.Init(World->GetGameInstance()->TimerManager);

		Pointers.SetNum(NumPointers);
		for (int i = 0; i < NumPointers; ++i)
		{
			Pointers[i] = UxtTestUtils::CreateNearPointer(World, *FString::Printf(TEXT("TestPointer%d"), i), FVector::ZeroVector);
		}
	}

	void PointerTestSequence::AddTarget(UWorld* World, const FVector& Location)
	{
		const FString& targetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
		const float targetScale = 0.3f;
		Targets.Add(UxtTestUtils::CreateNearPointerGrabTarget(World, Location, targetFilename, targetScale));
	}

	// Enter/Exit events must be incremented separately based on expected behavior.
	void PointerTestSequence::AddMovementKeyframe(const FVector& PointerLocation)
	{
		PointerKeyframe& keyframe = CreateKeyframe();
		keyframe.Location = PointerLocation;
	};

	void PointerTestSequence::AddGrabKeyframe(bool bEnableGrasp)
	{
		PointerKeyframe& keyframe = CreateKeyframe();
		keyframe.bIsGrabbing = bEnableGrasp;
	}

	PointerKeyframe& PointerTestSequence::CreateKeyframe()
	{
		PointerKeyframe keyframe;
		if (Keyframes.Num() == 0)
		{
			keyframe.Location = FVector::ZeroVector;
			keyframe.bIsGrabbing = false;
			keyframe.ExpectedFocusTargetIndex = -1;
		}
		else
		{
			keyframe.Location = Keyframes.Last().Location;
			keyframe.bIsGrabbing = Keyframes.Last().bIsGrabbing;
			keyframe.ExpectedFocusTargetIndex = Keyframes.Last().ExpectedFocusTargetIndex;
		}

		Keyframes.Add(keyframe);

		return Keyframes.Last();
	}

	void PointerTestSequence::ExpectFocusTargetIndex(int TargetIndex, bool bExpectEvents)
	{
		Keyframes.Last().ExpectedFocusTargetIndex = TargetIndex;
		Keyframes.Last().bExpectEvents = bExpectEvents;
	}

	void PointerTestSequence::ExpectFocusTargetNone(bool bExpectEvents)
	{
		ExpectFocusTargetIndex(-1, bExpectEvents);
	}

	void PointerTestSequence::Reset()
	{
		for (UUxtNearPointerComponent* Pointer : Pointers)
		{
			Pointer->GetOwner()->Destroy();
		}
		Pointers.Empty();

		for (UTestGrabTarget* Target : Targets)
		{
			Target->GetOwner()->Destroy();
		}
		Targets.Empty();

		Keyframes.Empty();

		FrameQueue.Reset();
	}

	TArray<TargetEventCountMap> PointerTestSequence::ComputeTargetEventCounts() const
	{
		TArray<TargetEventCountMap> result;

		result.Reserve(Keyframes.Num());

		int PrevFocusTarget = -1;
		bool bPrevIsGrasped = false;
		for (const auto& Keyframe : Keyframes)
		{
			TargetEventCountMap KeyframeEventCounts;
			if (result.Num() == 0)
			{
				KeyframeEventCounts.SetNum(Targets.Num());
			}
			else
			{
				KeyframeEventCounts = result.Last();
			}

			const int FocusTarget = Keyframe.ExpectedFocusTargetIndex;
			const bool bIsGrasped = Keyframe.bIsGrabbing;

			// Increment expected event counts if the keyframe is expected to trigger events
			if (Keyframe.bExpectEvents)
			{
				if (FocusTarget != PrevFocusTarget)
				{
					// Focused target changed: Increment the FocusEndCount of the previous target and the FocusStartCount of the new target.

					for (int Target = 0; Target < Targets.Num(); ++Target)
					{
						if (Target == PrevFocusTarget)
						{
							KeyframeEventCounts[Target].EndFocusCount += Pointers.Num();
						}
						if (Target == FocusTarget)
						{
							KeyframeEventCounts[Target].BeginFocusCount += Pointers.Num();
						}
					}
				}

				if (bIsGrasped != bPrevIsGrasped)
				{
					// Grasp changed: Increment GraspEndCount of the previous target when released, or the GraspStartCount of the current target when grasped.

					for (int Target = 0; Target < Targets.Num(); ++Target)
					{
						if (!bIsGrasped && Target == PrevFocusTarget)
						{
							KeyframeEventCounts[Target].EndGrabCount += Pointers.Num();
						}
						if (bIsGrasped && Target == FocusTarget)
						{
							KeyframeEventCounts[Target].BeginGrabCount += Pointers.Num();
						}
					}
				}
			}

			result.Add(KeyframeEventCounts);
			PrevFocusTarget = FocusTarget;
			bPrevIsGrasped = bIsGrasped;
		}

		return result;
	}

	/** Test enter/exit events for all targets at the given keyframe position. */
	void PointerTestSequence::TestKeyframe(FAutomationTestBase* Test, const TargetEventCountMap& EventCounts, int KeyframeIndex) const
	{
		for (int TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
		{
			const UTestGrabTarget* Target = Targets[TargetIndex];

			const TargetEventCount& ExpectedEventCounts = EventCounts[TargetIndex];

			FString whatFocusStarted; whatFocusStarted.Appendf(TEXT("Keyframe %d: Target %d EnterFocus count"), KeyframeIndex, TargetIndex);
			FString whatFocusEnded; whatFocusEnded.Appendf(TEXT("Keyframe %d: Target %d ExitFocus count"), KeyframeIndex, TargetIndex);
			FString whatGraspStarted; whatGraspStarted.Appendf(TEXT("Keyframe %d: Target %d BeginGrab count"), KeyframeIndex, TargetIndex);
			FString whatGraspEnded; whatGraspEnded.Appendf(TEXT("Keyframe %d: Target %d EndGrab count"), KeyframeIndex, TargetIndex);
			Test->TestEqual(whatFocusStarted, Target->BeginFocusCount, ExpectedEventCounts.BeginFocusCount);
			Test->TestEqual(whatFocusEnded, Target->EndFocusCount, ExpectedEventCounts.EndFocusCount);
			Test->TestEqual(whatGraspStarted, Target->BeginGrabCount, ExpectedEventCounts.BeginGrabCount);
			Test->TestEqual(whatGraspEnded, Target->EndGrabCount, ExpectedEventCounts.EndGrabCount);
		}
	}

	void PointerTestSequence::EnqueueFrames(FAutomationTestBase* Test, const FDoneDelegate& Done)
	{
		auto EventCountSequence = ComputeTargetEventCounts();

		// Iterating over n+1 frames.
		// Frame n+1 for checking the result of the last keyframe.
		const int NumKeyframes = Keyframes.Num();
		for (int iKeyframe = 0; iKeyframe <= NumKeyframes; ++iKeyframe)
		{
			FrameQueue.Enqueue([this, Test, Done, EventCountSequence, iKeyframe, NumKeyframes]
				{
					if (iKeyframe > 0)
					{
						// Test state of targets after update from the previous keyframe.
						TestKeyframe(Test, EventCountSequence[iKeyframe - 1], iKeyframe - 1);
					}

					if (iKeyframe < NumKeyframes)
					{
						const PointerKeyframe& Keyframe = Keyframes[iKeyframe];
						UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Keyframe.Location);
						UxtTestUtils::GetTestHandTracker().SetGrabbing(Keyframe.bIsGrabbing);
					}
					else
					{
						Done.Execute();
					}
				});
		}
	}

}
