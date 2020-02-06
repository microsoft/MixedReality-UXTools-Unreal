#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Misc/AutomationTest.h"

#include <functional>
#include <vector>

#include "Interactions/UxtTouchPointerTarget.h"

#include "TouchPointerAnimUtils.generated.h"

/**
 * Target for touch pointer tests that counts touch events.
 */
UCLASS()
class UXTOOLSTESTS_API UTestTouchPointerTarget : public UActorComponent, public IUxtTouchPointerTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// ITouchPointerTarget interface

	virtual void HoverStarted_Implementation(UUxtTouchPointer* Pointer) override;
	virtual void HoverEnded_Implementation(UUxtTouchPointer* Pointer) override;

	virtual void GraspStarted_Implementation(UUxtTouchPointer* Pointer) override;
	virtual void GraspEnded_Implementation(UUxtTouchPointer* Pointer) override;

	virtual bool GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface) override;

	int HoverStartedCount;
	int HoverEndedCount;

	int GraspStartedCount;
	int GraspEndedCount;

};

namespace TouchPointerAnimUtils
{

	struct TargetEventCount
	{
		int HoverStartCount = 0;
		int HoverEndCount = 0;
		int GraspStartCount = 0;
		int GraspEndCount = 0;
	};

	/** Contains event counts for each target index */
	typedef std::vector<TargetEventCount> TargetEventCountMap;

	/** Position in space for moving the pointer, with a list of expected enter/exit event counts for each target. */
	struct PointerKeyframe
	{
		/** Location of the pointer at this keyframe. */
		FVector Location;

		/** Grasp state of the pointer at this keyframe. */
		bool bIsGrasped = false;

		/** The expected hover target after moving the pointer to the keyframe location. */
		int ExpectedHoverTargetIndex = -1;
		
		/** If true then a target change in this keyframe is expected to trigger hover/grasp events on the target. */
		bool bExpectEvents = true;
	};

	struct TouchAnimSequence
	{
		const std::vector<UUxtTouchPointer*>& GetPointers() const { return Pointers; }
		const std::vector<UTestTouchPointerTarget*>& GetTargets() const { return Targets; }
		const std::vector<PointerKeyframe>& GetKeyframes() const { return Keyframes; }

		void CreatePointers(UWorld* world, int Count);

		void AddTarget(UWorld* world, const FVector& pos);
		void AddBackgroundTarget(UWorld* world);

		void AddMovementKeyframe(const FVector& pos);
		void AddGraspKeyframe(bool bEnableGrasp);

		void ExpectHoverTargetIndex(int TargetIndex, bool bExpectEvents = true);
		void ExpectHoverTargetNone(bool bExpectEvents = true);

		/** Compute a keyframe sequence with event counts for each target. */
		std::vector<TargetEventCountMap> ComputeTargetEventCounts() const;

		void TestKeyframe(FAutomationTestBase* Test, const TargetEventCountMap& EventCounts, int KeyframeIndex) const;

		void RunInterpolatedPointersTest(FAutomationTestBase* Test, float KeyframeDuration) const;
		void RunTeleportingPointersTest(FAutomationTestBase* Test) const;

	private:

		PointerKeyframe& CreateKeyframe();

	private:

		std::vector<UUxtTouchPointer*> Pointers;
		std::vector<UTestTouchPointerTarget*> Targets;
		std::vector<PointerKeyframe> Keyframes;
	};

}
