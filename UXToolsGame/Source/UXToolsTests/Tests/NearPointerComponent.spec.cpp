// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "UxtTestHandTracker.h"
#include "UxtTestTargetComponent.h"
#include "UxtTestUtils.h"

#include "Containers/Union.h"
#include "GameFramework/Actor.h"
#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Misc/AutomationTest.h"
#include "Templates/AreTypesEqual.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	const FString DefaultResource = TEXT("/Engine/BasicShapes/Cube.Cube");

	enum class ETestTargetKind
	{
		Grab,
		Poke
	};

	struct PointerTargetState
	{
	public:
		UTestGrabTarget* GetGrabTarget() const
		{
			return Target.HasSubtype<UTestGrabTarget*>() ? Target.GetSubtype<UTestGrabTarget*>() : nullptr;
		}

		UTestPokeTarget* GetPokeTarget() const
		{
			return Target.HasSubtype<UTestPokeTarget*>() ? Target.GetSubtype<UTestPokeTarget*>() : nullptr;
		}

		template <typename T>
		void SetTarget(T* NewTarget)
		{
			check((ARE_TYPES_EQUAL(T, UTestGrabTarget) || ARE_TYPES_EQUAL(T, UTestPokeTarget)));
			Target.SetSubtype<T*>(NewTarget);
		}

		AActor* GetOwner()
		{
			if (UTestGrabTarget* GrabTarget = GetGrabTarget())
			{
				return GrabTarget->GetOwner();
			}
			if (UTestPokeTarget* PokeTarget = GetPokeTarget())
			{
				return PokeTarget->GetOwner();
			}
			return nullptr;
		}

		const int32 GetBeginFocusCount() const
		{
			if (UTestGrabTarget* GrabTarget = GetGrabTarget())
			{
				return GrabTarget->BeginFocusCount;
			}
			if (UTestPokeTarget* PokeTarget = GetPokeTarget())
			{
				return PokeTarget->BeginFocusCount;
			}
			return 0;
		}

		const int32 GetEndFocusCount() const
		{
			if (UTestGrabTarget* GrabTarget = GetGrabTarget())
			{
				return GrabTarget->EndFocusCount;
			}
			if (UTestPokeTarget* PokeTarget = GetPokeTarget())
			{
				return PokeTarget->EndFocusCount;
			}
			return 0;
		}

		/** Expected event counts */
		int BeginFocusCount = 0;
		int EndFocusCount = 0;
		int BeginGrabCount = 0;
		int EndGrabCount = 0;
		int BeginPokeCount = 0;
		int EndPokeCount = 0;

	private:
		TUnion<UTestGrabTarget*, UTestPokeTarget*> Target;
	};

	bool IsThereCollisionWithActor(const FVector& Start, const FVector& End, const AActor* const& Actor)
	{
		FHitResult HitResult;
		UxtTestUtils::GetTestWorld()->SweepSingleByChannel(
			HitResult, Start, End, FQuat::Identity, ECollisionChannel::ECC_Visibility, FCollisionShape::MakeSphere(1.f));
		return HitResult.Actor == Actor;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	NearPointerPokeSpec, "UXTools.NearPointer",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

PointerTargetState& AddTarget(
	const FVector& Location, const ETestTargetKind = ETestTargetKind::Grab, const FString& ResourceLocation = DefaultResource,
	const float Scale = 0.3f);

void TestKeyframe();
void TestFocusTargetObject();

/**
 * Move pointer to new location and test for focus changes in the next frame.
 * NewFocusTargetIndex is the expected focus target.
 * Changed focus target is expected to increment event counters accordingly.
 * Target index -1 means no target is focused.
 */
void AddMovementKeyframe(const FVector& PointerLocation);
void ExpectFocusTargetIndex(int NewFocusTargetIndex);
void ExpectFocusTargetNone();
void ExpectGrabTargetNone();
void ExpectPokeTargetIndex(int TargetIndex);
void ExpectPokeTargetNone();
void AddGrabKeyframe(bool bEnableGrab);

FFrameQueue FrameQueue;
TArray<UUxtNearPointerComponent*> Pointers;
TArray<PointerTargetState> Targets;
int CurrentFocusTargetIndex = -1;
int CurrentPokeTargetIndex = -1;
bool bCurrentGrabbing = false;

const int NumPointers = 2;
const FVector TargetLocation = FVector(120, -20, -5);
const FVector InsideTargetLocation = FVector(113, -24, -8);
const FVector OutsideTargetLocation = FVector(150, 40, -40);
const FVector FocusStartLocation = FVector(40, -50, 30);
const FVector FocusEndLocation = FVector(150, 40, -40);
const FVector PokeInitialPosition = FVector(180, 0, 0);
const FVector PokeFinalPosition = FVector(186, 0, 0);

END_DEFINE_SPEC(NearPointerPokeSpec)

PointerTargetState& NearPointerPokeSpec::AddTarget(
	const FVector& Location, const ETestTargetKind TargetKind, const FString& ResourceLocation, const float Scale)
{
	UWorld* World = UxtTestUtils::GetTestWorld();
	PointerTargetState TargetState;
	switch (TargetKind)
	{
	case ETestTargetKind::Grab:
		TargetState.SetTarget(UxtTestUtils::CreateNearPointerGrabTarget(World, Location, ResourceLocation, Scale));
		break;
	case ETestTargetKind::Poke:
		TargetState.SetTarget(UxtTestUtils::CreateNearPointerPokeTarget(World, Location, ResourceLocation, Scale));
		break;
	}
	int32 Index = Targets.Add(TargetState);
	return Targets[Index];
}

/** Test event counts for all targets. */
void NearPointerPokeSpec::TestKeyframe()
{
	for (int TargetIndex = 0; TargetIndex < Targets.Num(); ++TargetIndex)
	{
		const PointerTargetState& TargetState = Targets[TargetIndex];

		FString whatFocusStarted;
		whatFocusStarted.Appendf(TEXT("Target %d EnterFocus count"), TargetIndex);
		FString whatFocusEnded;
		whatFocusEnded.Appendf(TEXT("Target %d ExitFocus count"), TargetIndex);
		FString whatGraspStarted;
		whatGraspStarted.Appendf(TEXT("Target %d BeginGrab count"), TargetIndex);
		FString whatGraspEnded;
		whatGraspEnded.Appendf(TEXT("Target %d EndGrab count"), TargetIndex);
		FString whatPokeStarted;
		whatPokeStarted.Appendf(TEXT("Target %d BeginPoke count"), TargetIndex);
		FString whatPokeEnded;
		whatPokeEnded.Appendf(TEXT("Target %d EndPoke count"), TargetIndex);
		TestEqual(whatFocusStarted, TargetState.GetBeginFocusCount(), TargetState.BeginFocusCount);
		TestEqual(whatFocusEnded, TargetState.GetEndFocusCount(), TargetState.EndFocusCount);
		if (UTestGrabTarget* GrabTarget = TargetState.GetGrabTarget())
		{
			TestEqual(whatGraspStarted, GrabTarget->BeginGrabCount, TargetState.BeginGrabCount);
			TestEqual(whatGraspEnded, GrabTarget->EndGrabCount, TargetState.EndGrabCount);
		}
		else if (UTestPokeTarget* PokeTarget = TargetState.GetPokeTarget())
		{
			TestEqual(whatPokeStarted, PokeTarget->BeginPokeCount, TargetState.BeginPokeCount);
			TestEqual(whatPokeEnded, PokeTarget->EndPokeCount, TargetState.EndPokeCount);
		}
	}
}

/** Test for the pointer having the correct focus target object. */
void NearPointerPokeSpec::TestFocusTargetObject()
{
	FrameQueue.Enqueue([this]() {
		const bool bValidTarget = CurrentFocusTargetIndex != -1 && CurrentFocusTargetIndex < Targets.Num();
		TestTrue("Focus target is valid", bValidTarget);

		const UTestGrabTarget* GrabTarget = Targets[CurrentFocusTargetIndex].GetGrabTarget();
		TestTrue("Target kind is Grab", GrabTarget != nullptr);
		const bool bTargetFocused = Pointers[0]->GetFocusTarget() == GrabTarget;
		TestTrue("Target is focused", bTargetFocused);
	});
}

void NearPointerPokeSpec::AddMovementKeyframe(const FVector& PointerLocation)
{
	FrameQueue.Enqueue([PointerLocation] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(PointerLocation); });
}

void NearPointerPokeSpec::ExpectFocusTargetIndex(int NewFocusTargetIndex)
{
	FrameQueue.Enqueue([this, NewFocusTargetIndex] {
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

void NearPointerPokeSpec::ExpectGrabTargetNone()
{
	FrameQueue.Enqueue([this]() {
		for (const PointerTargetState& Target : Targets)
		{
			const UTestGrabTarget* GrabTarget = Target.GetGrabTarget();
			TestTrue("Target must be of Grab kind", GrabTarget != nullptr);
			if (GrabTarget)
			{
				const bool bIsGrabbed = GrabTarget->BeginGrabCount > GrabTarget->EndGrabCount;
				TestFalse("Target should not be grabbed", bIsGrabbed);
			}
		}
	});
}

void NearPointerPokeSpec::ExpectPokeTargetIndex(int TargetIndex)
{
	FrameQueue.Enqueue([this, TargetIndex] {
		check(TargetIndex >= 0 && TargetIndex < Targets.Num());
		if (TargetIndex != CurrentPokeTargetIndex)
		{
			Targets[TargetIndex].BeginPokeCount += Pointers.Num();
			if (CurrentPokeTargetIndex != -1)
			{
				Targets[CurrentPokeTargetIndex].EndPokeCount += Pointers.Num();
			}
			CurrentPokeTargetIndex = TargetIndex;
		}
		TestKeyframe();
	});
}

void NearPointerPokeSpec::ExpectPokeTargetNone()
{
	FrameQueue.Enqueue([this]() {
		for (const PointerTargetState& Target : Targets)
		{
			const UTestPokeTarget* PokeTarget = Target.GetPokeTarget();
			if (PokeTarget == nullptr)
			{
				continue; // Ignore any non-poke target
			}
			const bool bIsPoked = PokeTarget->BeginPokeCount - PokeTarget->EndPokeCount != 0;
			TestFalse("Target should not be poked", bIsPoked);
		}
	});
}

void NearPointerPokeSpec::AddGrabKeyframe(bool bEnableGrab)
{
	FrameQueue.Enqueue([bEnableGrab] { UxtTestUtils::GetTestHandTracker().SetGrabbing(bEnableGrab); });

	FrameQueue.Enqueue([this, bEnableGrab] {
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
	Describe("Near pointer", [this] {
		BeforeEach([this] {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));
			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);
			UxtTestUtils::EnableTestHandTracker();

			Pointers.SetNum(NumPointers);
			for (int i = 0; i < NumPointers; ++i)
			{
				Pointers[i] = UxtTestUtils::CreateNearPointer(World, *FString::Printf(TEXT("TestPointer%d"), i), FVector::ZeroVector);
				TestTrue("Valid near pointer", Pointers[i] != nullptr);
			}

			// Register all new components.
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			for (UUxtNearPointerComponent* Pointer : Pointers)
			{
				Pointer->GetOwner()->Destroy();
			}
			Pointers.Empty();

			for (PointerTargetState& TargetState : Targets)
			{
				TargetState.GetOwner()->Destroy();
			}
			Targets.Empty();
			CurrentFocusTargetIndex = -1;
			CurrentPokeTargetIndex = -1;
			bCurrentGrabbing = false;

			FrameQueue.Reset();
		});

		LatentIt("should have correct focus target object", [this](const FDoneDelegate& Done) {
			AddTarget(TargetLocation);

			AddMovementKeyframe(InsideTargetLocation);
			ExpectFocusTargetIndex(0);
			TestFocusTargetObject();

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should focus poke target when overlapping initially", [this](const FDoneDelegate& Done) {
			AddTarget(TargetLocation);

			AddMovementKeyframe(InsideTargetLocation);
			ExpectFocusTargetIndex(0);

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should focus poke target when entering", [this](const FDoneDelegate& Done) {
			AddTarget(TargetLocation);

			AddMovementKeyframe(OutsideTargetLocation);
			ExpectFocusTargetNone();

			AddMovementKeyframe(InsideTargetLocation);
			ExpectFocusTargetIndex(0);

			AddMovementKeyframe(OutsideTargetLocation);
			ExpectFocusTargetNone();

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should have no focus without targets", [this](const FDoneDelegate& Done) {
			AddMovementKeyframe(FocusStartLocation);
			ExpectFocusTargetNone();

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should focus single target", [this](const FDoneDelegate& Done) {
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

		LatentIt("should focus two separate targets", [this](const FDoneDelegate& Done) {
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

		LatentIt("should focus two overlapping targets", [this](const FDoneDelegate& Done) {
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

		LatentIt("should focus grab target when overlapping initially", [this](const FDoneDelegate& Done) {
			AddTarget(TargetLocation);

			AddMovementKeyframe(InsideTargetLocation);
			ExpectFocusTargetIndex(0);

			AddGrabKeyframe(true);

			AddGrabKeyframe(false);

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should focus grab target when entering", [this](const FDoneDelegate& Done) {
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

		LatentIt("should not grab target when entering while already grabbing", [this](const FDoneDelegate& Done) {
			AddTarget(TargetLocation);

			AddMovementKeyframe(OutsideTargetLocation);
			AddGrabKeyframe(true);
			ExpectGrabTargetNone();

			AddMovementKeyframe(InsideTargetLocation);
			ExpectGrabTargetNone();

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
		LatentIt("should start poking box target", [this](const FDoneDelegate& Done) {
			PointerTargetState& Target = AddTarget(FVector(200, 0, 0), ETestTargetKind::Poke);

			bool Colliding = IsThereCollisionWithActor(PokeInitialPosition, PokeFinalPosition, Target.GetPokeTarget()->GetOwner());
			TestTrue("There's a collision", Colliding);

			ExpectFocusTargetNone();
			AddMovementKeyframe(PokeInitialPosition);
			ExpectFocusTargetIndex(0);
			ExpectPokeTargetNone();
			AddMovementKeyframe(PokeFinalPosition);
			ExpectPokeTargetIndex(0);

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
		LatentIt("should not start poking non-box target", [this](const FDoneDelegate& Done) {
			PointerTargetState& Target = AddTarget(FVector(200, 0, 0), ETestTargetKind::Poke, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

			bool Colliding = IsThereCollisionWithActor(PokeInitialPosition, PokeFinalPosition, Target.GetPokeTarget()->GetOwner());
			TestTrue("There's a collision", Colliding);

			ExpectFocusTargetNone();
			AddMovementKeyframe(PokeInitialPosition);
			ExpectPokeTargetNone();
			AddMovementKeyframe(PokeFinalPosition);
			ExpectPokeTargetNone();

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
		LatentIt("should not start poking when facing backwards", [this](const FDoneDelegate& Done) {
			PointerTargetState& Target = AddTarget(FVector(200, 0, 0), ETestTargetKind::Poke);
			AActor* const TargetActor = Target.GetPokeTarget()->GetOwner();
			TargetActor->AddActorLocalRotation(FQuat::MakeFromEuler(FVector(0, 0, 180)));

			bool Colliding = IsThereCollisionWithActor(PokeInitialPosition, PokeFinalPosition, Target.GetPokeTarget()->GetOwner());
			TestTrue("There's a collision", Colliding);

			ExpectFocusTargetNone();
			AddMovementKeyframe(PokeInitialPosition);
			ExpectFocusTargetIndex(0);
			ExpectPokeTargetNone();
			AddMovementKeyframe(PokeFinalPosition);
			ExpectPokeTargetNone();

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
