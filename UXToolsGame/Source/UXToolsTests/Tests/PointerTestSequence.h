#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Misc/AutomationTest.h"

#include "Interactions/UxtGrabTarget.h"

#include "PointerTestSequence.generated.h"

class UUxtNearPointerComponent;
class FUxtTestHandTracker;

/**
 * Target for touch pointer tests that counts touch events.
 */
UCLASS()
class UXTOOLSTESTS_API UTestTouchPointerTarget : public UActorComponent, public IUxtGrabTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface

	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual bool GetClosestGrabPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const override;

	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	int BeginFocusCount;
	int EndFocusCount;

	int BeginGrabCount;
	int EndGrabCount;

};

namespace UxtPointerTests
{

	struct TargetEventCount
	{
		int BeginFocusCount = 0;
		int EndFocusCount = 0;
		int BeginGrabCount = 0;
		int EndGrabCount = 0;
	};

	/** Contains event counts for each target index */
	typedef TArray<TargetEventCount> TargetEventCountMap;

	/** Position in space for moving the pointer, with a list of expected enter/exit event counts for each target. */
	struct PointerKeyframe
	{
		/** Location of the pointer at this keyframe. */
		FVector Location;

		/** Grab state of the pointer at this keyframe. */
		bool bIsGrabbing = false;

		/** The expected focus target after moving the pointer to the keyframe location. */
		int ExpectedFocusTargetIndex = -1;
		
		/** If true then a target change in this keyframe is expected to trigger focus/grab events on the target. */
		bool bExpectEvents = true;
	};

	struct PointerTestSequence
	{
		const TArray<UUxtNearPointerComponent*>& GetPointers() const { return Pointers; }
		const TArray<UTestTouchPointerTarget*>& GetTargets() const { return Targets; }
		const TArray<PointerKeyframe>& GetKeyframes() const { return Keyframes; }

		void CreatePointers(UWorld* world, int Count);

		void AddTarget(UWorld* world, const FVector& pos);

		void AddMovementKeyframe(const FVector& pos);
		void AddGrabKeyframe(bool bEnableGrabbing);

		void ExpectFocusTargetIndex(int TargetIndex, bool bExpectEvents = true);
		void ExpectFocusTargetNone(bool bExpectEvents = true);

		/** Compute a keyframe sequence with event counts for each target. */
		TArray<TargetEventCountMap> ComputeTargetEventCounts() const;

		void TestKeyframe(FAutomationTestBase* Test, const TargetEventCountMap& EventCounts, int KeyframeIndex) const;

		void EnqueueTestSequence(FAutomationTestBase* Test) const;

	private:

		PointerKeyframe& CreateKeyframe();

	private:

		TArray<UUxtNearPointerComponent*> Pointers;
		TArray<UTestTouchPointerTarget*> Targets;
		TArray<PointerKeyframe> Keyframes;
	};

}
