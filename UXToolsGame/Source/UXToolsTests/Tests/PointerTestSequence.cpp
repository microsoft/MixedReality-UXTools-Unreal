// Fill out your copyright notice in the Description page of Project Settings.

#include "PointerTestSequence.h"

#include "Input/UxtNearPointerComponent.h"
#include "UxtTestUtils.h"
#include "UxtTestHandTracker.h"

#include "Components/PrimitiveComponent.h"

void UTestTouchPointerTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
}

void UTestTouchPointerTarget::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestTouchPointerTarget::OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestTouchPointerTarget::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

void UTestTouchPointerTarget::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginGrabCount;
}

void UTestTouchPointerTarget::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndGrabCount;
}

bool UTestTouchPointerTarget::GetClosestGrabPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const
{
	OutPointOnSurface = Primitive->GetComponentLocation();
	return true;
}

namespace UxtPointerTests
{

	/**
	 * Latent command that moves pointers to a specific keyframe and tests expected conditions.
	 */
	class FTestPointerKeyframeCommand : public IAutomationLatentCommand
	{
	public:
		FTestPointerKeyframeCommand(
			FAutomationTestBase* Test,
			const PointerTestSequence& Sequence,
			int KeyframeIndex,
			const TargetEventCountMap& ExpectedEventCounts)
			: Test(Test)
			, Sequence(Sequence)
			, KeyframeIndex(KeyframeIndex)
			, ExpectedEventCounts(ExpectedEventCounts)
			, UpdateCount(0)
		{
		}

		virtual ~FTestPointerKeyframeCommand()
		{}

		virtual bool Update() override
		{
			// Two step update:
			// 
			// 1. First move the hand(s) to the keyframe location.
			//    Return false, so the pointer has one frame to update overlaps and raise events.
			// 2. Test the expected event counts on targets after the pointer update.

			const PointerKeyframe& Keyframe = Sequence.GetKeyframes()[KeyframeIndex];
			switch (UpdateCount)
			{
				case 0:
					UxtTestUtils::GetTestHandTracker().TestPosition = Keyframe.Location;
					UxtTestUtils::GetTestHandTracker().bIsGrabbing = Keyframe.bIsGrabbing;
					// Wait for one frame to update pointer overlap.
					break;

				case 1:
					Sequence.TestKeyframe(Test, ExpectedEventCounts, KeyframeIndex);
					break;
			}

			++UpdateCount;
			return (UpdateCount >= 1);
		}

	private:

		FAutomationTestBase* Test;
		const PointerTestSequence Sequence;
		const int KeyframeIndex;
		const TargetEventCountMap ExpectedEventCounts;

		int32 UpdateCount;
	};

	void PointerTestSequence::CreatePointers(UWorld* world, int Count)
	{
		Pointers.SetNum(Count);
		for (int i = 0; i < Count; ++i)
		{
			Pointers[i] = UxtTestUtils::CreateTouchPointer(world, TEXT("TestPointer"), FVector::ZeroVector);
		}
	}

	void PointerTestSequence::AddTarget(UWorld* world, const FVector& pos)
	{
		const FString& targetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
		const float targetScale = 0.3f;
		Targets.Add(UxtTestUtils::CreateTouchPointerTarget(world, pos, targetFilename, targetScale));
	}

	// Enter/Exit events must be incremented separately based on expected behavior.
	void PointerTestSequence::AddMovementKeyframe(const FVector& pos)
	{
		PointerKeyframe& keyframe = CreateKeyframe();
		keyframe.Location = pos;
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
			const UTestTouchPointerTarget* Target = Targets[TargetIndex];

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

	void PointerTestSequence::EnqueueTestSequence(FAutomationTestBase* Test) const
	{
		auto EventCountSequence = ComputeTargetEventCounts();

		for (int iKeyframe = 0; iKeyframe < Keyframes.Num(); ++iKeyframe)
		{
			ADD_LATENT_AUTOMATION_COMMAND(FTestPointerKeyframeCommand(Test, *this, iKeyframe, EventCountSequence[iKeyframe]));
		}
	}

}