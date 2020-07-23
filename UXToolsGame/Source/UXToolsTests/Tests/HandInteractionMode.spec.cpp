// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"
#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Interactions/UxtManipulationFlags.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const static FVector Center = FVector(150, 0, 0);
	const FVector NearLocation = Center - FVector(15, 0, 0);
	const FVector FarLocation = FVector::ZeroVector;

	struct InteractionResult {
		bool IsValidGrab = false;
		FUxtGrabPointerData PointerData;
	};
}

BEGIN_DEFINE_SPEC(HandInteractionModeSpec, "UXTools.HandInteraction", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueSetHandTrackerLocation(const FVector&);
void EnqueueGrabStatusChange(bool);
void EnqueueSelectStatusChange(bool);
void EnqueueInteractionModeChange(uint8);
void EnqueueInteractionTest(bool = false, EUxtInteractionMode = EUxtInteractionMode::None);

InteractionResult RetrieveInteractionResult();

AUxtHandInteractionActor* HandActor;
UUxtNearPointerComponent* NearPointer;
UUxtFarPointerComponent* FarPointer;
UUxtGrabTargetComponent* Target;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(HandInteractionModeSpec)

void HandInteractionModeSpec::Define()
{
	Describe("Interaction Mode", [this]
		{
			BeforeEach([this]
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UxtTestUtils::EnableTestHandTracker();

					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(World->GetGameInstance()->TimerManager);

					HandActor = World->SpawnActor<AUxtHandInteractionActor>();
					HandActor->SetHand(EControllerHand::Left);
					HandActor->InteractionMode = static_cast<uint8>(EUxtInteractionMode::Near | EUxtInteractionMode::Far);

					NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
					FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();

					Target = UxtTestUtils::CreateTestBoxWithComponent<UUxtGrabTargetComponent>(World, Center);

					// Register all new components.
					World->UpdateWorldComponents(false, false);

					// Skip one frame as pointers take a frame to start ticking when activated by the hand interaction actor
					FrameQueue.Skip();
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();
					FrameQueue.Reset();
					HandActor->Destroy();
					Target->GetOwner()->Destroy();
				});

			LatentIt("should not grab when near mode is disabled", [this](const FDoneDelegate& Done)
				{
					EnqueueSetHandTrackerLocation(NearLocation);
					EnqueueGrabStatusChange(true);
					EnqueueInteractionTest(true, EUxtInteractionMode::Near);
					EnqueueGrabStatusChange(false);
					EnqueueInteractionModeChange(static_cast<uint8>(EUxtInteractionMode::Far));
					EnqueueGrabStatusChange(true);
					EnqueueInteractionTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should release grab when near mode is disabled", [this](const FDoneDelegate& Done)
				{
					EnqueueSetHandTrackerLocation(NearLocation);
					EnqueueGrabStatusChange(true);
					EnqueueInteractionTest(true, EUxtInteractionMode::Near);
					EnqueueInteractionModeChange(static_cast<uint8>(EUxtInteractionMode::Far));
					EnqueueInteractionTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should not select when far mode is disabled", [this](const FDoneDelegate& Done)
				{
					EnqueueSetHandTrackerLocation(FarLocation);
					EnqueueSelectStatusChange(true);
					EnqueueInteractionTest(true, EUxtInteractionMode::Far);
					EnqueueSelectStatusChange(false);
					EnqueueInteractionModeChange(static_cast<uint8>(EUxtInteractionMode::Near));
					EnqueueSelectStatusChange(true);
					EnqueueInteractionTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should release selection when far mode is disabled", [this](const FDoneDelegate& Done)
				{
					EnqueueSetHandTrackerLocation(FarLocation);
					EnqueueSelectStatusChange(true);
					EnqueueInteractionTest(true, EUxtInteractionMode::Far);
					EnqueueInteractionModeChange(static_cast<uint8>(EUxtInteractionMode::Near));
					EnqueueInteractionTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

void HandInteractionModeSpec::EnqueueSetHandTrackerLocation(const FVector& Position)
{
	FrameQueue.Enqueue([this, Position]
		{
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Position);
		});
}

void HandInteractionModeSpec::EnqueueGrabStatusChange(bool bGrabStatus)
{
	FrameQueue.Enqueue([this, bGrabStatus]
		{
			UxtTestUtils::GetTestHandTracker().SetGrabbing(bGrabStatus);
		});
}

void HandInteractionModeSpec::EnqueueSelectStatusChange(bool bSelectStatus)
{
	FrameQueue.Enqueue([this, bSelectStatus]
		{
			UxtTestUtils::GetTestHandTracker().SetSelectPressed(bSelectStatus);
		});
}

void HandInteractionModeSpec::EnqueueInteractionModeChange(uint8 Flags)
{
	FrameQueue.Enqueue([this, Flags]
		{
			HandActor->InteractionMode = Flags;
		});
}

void HandInteractionModeSpec::EnqueueInteractionTest(bool bExpectValidInteraction, EUxtInteractionMode Mode)
{
	FrameQueue.Enqueue([this, bExpectValidInteraction, Mode]
		{
			InteractionResult Result = RetrieveInteractionResult();
			TestEqual("Has primary grab pointer", Result.IsValidGrab, bExpectValidInteraction);
			if (bExpectValidInteraction)
			{
				switch (Mode)
				{
				case EUxtInteractionMode::Near:
					TestEqual("Primary grab pointer is the near pointer", Result.PointerData.NearPointer, NearPointer);
					break;
				case EUxtInteractionMode::Far:
					TestEqual("Primary grab pointer is the far pointer", Result.PointerData.FarPointer, FarPointer);
					break;
				default:
					TestTrue("Expect valid interaction of None type", false);
					break;
				}
			}
		});
}

InteractionResult HandInteractionModeSpec::RetrieveInteractionResult()
{
	InteractionResult Result;
	Target->GetPrimaryGrabPointer(Result.IsValidGrab, Result.PointerData);
	return Result;
}

#endif // WITH_DEV_AUTOMATION_TESTS
