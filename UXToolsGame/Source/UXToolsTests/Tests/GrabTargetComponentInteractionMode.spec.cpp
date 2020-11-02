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
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Interactions/UxtInteractionMode.h"
#include "Interactions/UxtManipulationFlags.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const static FVector Center = FVector(150, 0, 0);
}

BEGIN_DEFINE_SPEC(
	GrabTargetComponentInteractionModeSpec, "UXTools.GrabTargetComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtNearPointerComponent* NearPointer;
UUxtFarPointerComponent* FarPointer;
UUxtGrabTargetComponent* Target;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(GrabTargetComponentInteractionModeSpec)

void GrabTargetComponentInteractionModeSpec::Define()
{
	Describe("Interaction Mode", [this] {
		BeforeEach([this] {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			UxtTestUtils::EnableTestHandTracker();

			Target = UxtTestUtils::CreateTestBoxWithComponent<UUxtGrabTargetComponent>(World, Center);

			// Register all new components.
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();

			Target->GetOwner()->Destroy();
			Target = nullptr;
		});

		LatentIt("should not be grabbed by near interaction when near mode is disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				NearPointer =
					UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), TEXT("GrabTestPointer"), Center + FVector(-15, 0, 0));

				// Enable grab.
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// Check that the object is grabbed
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestTrue("Has primary grab pointer", IsValidGrab);
				TestEqual("Primary grab pointer", PointerData.NearPointer, NearPointer);

				// release grab
				UxtTestUtils::GetTestHandTracker().SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				// set interaction mode to not include near interaction
				Target->InteractionMode = static_cast<int32>(EUxtInteractionMode::Far);
				// enable grab again
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this, Done] {
				// now make sure the near pointer didn't grab the target
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestFalse(TEXT("target was grabbed even though near interaction was disabled"), IsValidGrab);

				NearPointer->GetOwner()->Destroy();
				NearPointer = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should stop near interaction when near mode is disabled during interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				NearPointer =
					UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), TEXT("GrabTestPointer"), Center + FVector(-15, 0, 0));

				// Enable grab.
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// Check that the object is grabbed
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestTrue("Has primary grab pointer", IsValidGrab);
				TestEqual("Primary grab pointer", PointerData.NearPointer, NearPointer);

				// change interaction mode to far only while interacting
				Target->InteractionMode = static_cast<int32>(EUxtInteractionMode::Far);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure target got released
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestFalse(TEXT("target is still grabbed after disabling near interaction"), IsValidGrab);

				NearPointer->GetOwner()->Destroy();
				NearPointer = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should not be grabbed by far interaction when far mode is disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				FarPointer = UxtTestUtils::CreateFarPointer(
					UxtTestUtils::GetTestWorld(), TEXT("FarTestPointer"), FVector::ZeroVector, EControllerHand::Right);

				AActor* TargetActor = Target->GetOwner();
				TargetActor->SetActorLocation(FVector(200, 0, 0));
			});

			FrameQueue.Enqueue([this] {
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(true);
			});

			FrameQueue.Enqueue([this] {
				// Check that the object is grabbed
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestTrue("Has primary grab pointer", IsValidGrab);
				TestEqual("Primary grab pointer", PointerData.FarPointer, FarPointer);

				// release grab
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(false);

				// set interaction mode to not include far interaction
				Target->InteractionMode = static_cast<int32>(EUxtInteractionMode::Near);
			});

			FrameQueue.Enqueue([this] {
				// enable grab again
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(true);
			});

			FrameQueue.Enqueue([this, Done] {
				// now make sure the near pointer didn't grab the target
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestFalse(TEXT("target was grabbed even though far interaction was disabled"), IsValidGrab);

				FarPointer->GetOwner()->Destroy();
				FarPointer = nullptr;

				Done.Execute();
			});
		});

		LatentIt("should stop far interaction when far mode is disabled during interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				FarPointer = UxtTestUtils::CreateFarPointer(
					UxtTestUtils::GetTestWorld(), TEXT("FarTestPointer"), FVector::ZeroVector, EControllerHand::Right);

				AActor* TargetActor = Target->GetOwner();
				TargetActor->SetActorLocation(FVector(200, 0, 0));
			});

			FrameQueue.Enqueue([this] {
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(true);
			});

			FrameQueue.Enqueue([this] {
				// Check that the object is grabbed
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestTrue("Has primary grab pointer", IsValidGrab);
				TestEqual("Primary grab pointer", PointerData.FarPointer, FarPointer);

				// set interaction mode to not include far interaction
				Target->InteractionMode = static_cast<int32>(EUxtInteractionMode::Near);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure far pointer got released
				bool IsValidGrab;
				FUxtGrabPointerData PointerData;
				Target->GetPrimaryGrabPointer(IsValidGrab, PointerData);
				TestFalse(TEXT("target still grabbed after disabling far interaction"), IsValidGrab);

				FarPointer->GetOwner()->Destroy();
				FarPointer = nullptr;

				Done.Execute();
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
