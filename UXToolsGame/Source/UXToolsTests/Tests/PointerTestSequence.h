// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Misc/AutomationTest.h"

#include "FrameQueue.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeTarget.h"

#include "PointerTestSequence.generated.h"

class UUxtNearPointerComponent;
class FUxtTestHandTracker;

/**
 * Target for grab tests that counts grab events.
 */
UCLASS()
class UXTOOLSTESTS_API UTestGrabTarget : public UActorComponent, public IUxtGrabTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface

	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	int BeginFocusCount;
	int EndFocusCount;

	int BeginGrabCount;
	int EndGrabCount;

	// If the target should enable focus lock on the pointer while grabbed.
	bool bUseFocusLock = false;

};

/**
 * Target for poke tests that counts poke events.
 */
UCLASS()
class UXTOOLSTESTS_API UTestPokeTarget : public UActorComponent, public IUxtPokeTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface

	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual EUxtPokeBehaviour GetPokeBehaviour() const;

	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;

	int BeginFocusCount;
	int EndFocusCount;

	int BeginPokeCount;
	int EndPokeCount;

	// If the target should enable focus lock on the pointer while poked.
	bool bUseFocusLock = false;

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
		const TArray<UTestGrabTarget*>& GetTargets() const { return Targets; }
		const TArray<PointerKeyframe>& GetKeyframes() const { return Keyframes; }

		void Init(UWorld* World, int NumPointers);

		void AddTarget(UWorld* World, const FVector& Location);

		void AddMovementKeyframe(const FVector& PointerLocation);
		void AddGrabKeyframe(bool bEnableGrabbing);

		void ExpectFocusTargetIndex(int TargetIndex, bool bExpectEvents = true);
		void ExpectFocusTargetNone(bool bExpectEvents = true);

		void Reset();

		/** Compute a keyframe sequence with event counts for each target. */
		TArray<TargetEventCountMap> ComputeTargetEventCounts() const;

		void TestKeyframe(FAutomationTestBase* Test, const TargetEventCountMap& EventCounts, int KeyframeIndex) const;

		void EnqueueFrames(FAutomationTestBase* Test, const FDoneDelegate& Done);

	private:

		PointerKeyframe& CreateKeyframe();

	private:

		TArray<UUxtNearPointerComponent*> Pointers;
		TArray<UTestGrabTarget*> Targets;
		TArray<PointerKeyframe> Keyframes;
		FFrameQueue FrameQueue;
	};

}

