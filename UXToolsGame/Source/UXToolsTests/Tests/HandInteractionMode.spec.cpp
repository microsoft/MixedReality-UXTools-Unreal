// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Interactions/UxtManipulationFlags.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const static FVector Center = FVector(150, 0, 0);
	const FVector NearLocation = Center - FVector(15, 0, 0);
	const FVector FarLocation = FVector::ZeroVector;

	struct InteractionResult
	{
		bool IsValidGrab = false;
		FUxtGrabPointerData PointerData;
	};
} // namespace

BEGIN_DEFINE_SPEC(
	HandInteractionModeSpec, "UXTools.HandInteraction", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueSetHandTrackerLocation(const FVector&);
void EnqueueGrabStatusChange(bool);
void EnqueueSelectStatusChange(bool);
void EnqueueInteractionModeChange(int32);
void EnqueueNoInteractionTest();
void EnqueueNearInteractionTest();
void EnqueueFarInteractionTest();

InteractionResult RetrieveInteractionResult();

AUxtHandInteractionActor* HandActor;
UUxtNearPointerComponent* NearPointer;
UUxtFarPointerComponent* FarPointer;
UUxtGrabTargetComponent* Target;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(HandInteractionModeSpec)

void HandInteractionModeSpec::Define()
{
	Describe("Hand interaction actor", [this] {
		BeforeEach([this] {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UxtTestUtils::EnableTestHandTracker();

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			HandActor = World->SpawnActor<AUxtHandInteractionActor>();
			HandActor->SetHand(EControllerHand::Left);

			NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
			FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();

			Target = UxtTestUtils::CreateTestBoxWithComponent<UUxtGrabTargetComponent>(World, Center);

			// Register all new components.
			World->UpdateWorldComponents(false, false);

			// Skip one frame as pointers take a frame to start ticking when activated by the hand interaction actor
			FrameQueue.Skip();
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();
			FrameQueue.Reset();
			HandActor->Destroy();
			Target->GetOwner()->Destroy();
		});

		LatentIt("should not grab when near mode is disabled", [this](const FDoneDelegate& Done) {
			EnqueueSetHandTrackerLocation(NearLocation);
			EnqueueGrabStatusChange(true);
			EnqueueNearInteractionTest();
			EnqueueGrabStatusChange(false);
			EnqueueInteractionModeChange(static_cast<int32>(EUxtInteractionMode::Far));
			EnqueueGrabStatusChange(true);
			EnqueueNoInteractionTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should release grab when near mode is disabled", [this](const FDoneDelegate& Done) {
			EnqueueSetHandTrackerLocation(NearLocation);
			EnqueueGrabStatusChange(true);
			EnqueueNearInteractionTest();
			EnqueueInteractionModeChange(static_cast<int32>(EUxtInteractionMode::Far));
			EnqueueNoInteractionTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should not select when far mode is disabled", [this](const FDoneDelegate& Done) {
			EnqueueSetHandTrackerLocation(FarLocation);
			EnqueueSelectStatusChange(true);
			EnqueueFarInteractionTest();
			EnqueueSelectStatusChange(false);
			EnqueueInteractionModeChange(static_cast<int32>(EUxtInteractionMode::Near));
			EnqueueSelectStatusChange(true);
			EnqueueNoInteractionTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should release selection when far mode is disabled", [this](const FDoneDelegate& Done) {
			EnqueueSetHandTrackerLocation(FarLocation);
			EnqueueSelectStatusChange(true);
			EnqueueFarInteractionTest();
			EnqueueInteractionModeChange(static_cast<int32>(EUxtInteractionMode::Near));
			EnqueueNoInteractionTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void HandInteractionModeSpec::EnqueueSetHandTrackerLocation(const FVector& Position)
{
	FrameQueue.Enqueue([this, Position] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Position); });
}

void HandInteractionModeSpec::EnqueueGrabStatusChange(bool bGrabStatus)
{
	FrameQueue.Enqueue([this, bGrabStatus] { UxtTestUtils::GetTestHandTracker().SetGrabbing(bGrabStatus); });
}

void HandInteractionModeSpec::EnqueueSelectStatusChange(bool bSelectStatus)
{
	FrameQueue.Enqueue([this, bSelectStatus] { UxtTestUtils::GetTestHandTracker().SetSelectPressed(bSelectStatus); });
}

void HandInteractionModeSpec::EnqueueInteractionModeChange(int32 Flags)
{
	FrameQueue.Enqueue([this, Flags] { HandActor->InteractionMode = Flags; });
}

void HandInteractionModeSpec::EnqueueNoInteractionTest()
{
	FrameQueue.Enqueue([this] {
		InteractionResult Result = RetrieveInteractionResult();
		TestFalse("Has primary grab pointer", Result.IsValidGrab);
	});
}

void HandInteractionModeSpec::EnqueueNearInteractionTest()
{
	FrameQueue.Enqueue([this] {
		InteractionResult Result = RetrieveInteractionResult();
		TestTrue("Has primary grab pointer", Result.IsValidGrab);
		TestEqual("Primary grab pointer is the near pointer", Result.PointerData.NearPointer, NearPointer);
	});
}

void HandInteractionModeSpec::EnqueueFarInteractionTest()
{
	FrameQueue.Enqueue([this] {
		InteractionResult Result = RetrieveInteractionResult();
		TestTrue("Has primary grab pointer", Result.IsValidGrab);
		TestEqual("Primary grab pointer is the far pointer", Result.PointerData.FarPointer, FarPointer);
	});
}

InteractionResult HandInteractionModeSpec::RetrieveInteractionResult()
{
	InteractionResult Result;
	Target->GetPrimaryGrabPointer(Result.IsValidGrab, Result.PointerData);
	return Result;
}

#endif // WITH_DEV_AUTOMATION_TESTS
