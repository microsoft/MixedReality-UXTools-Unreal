// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "UxtTestHandTracker.h"
#include "UxtTestTargetComponent.h"
#include "UxtTestUtils.h"

#include "Controls/UxtFarBeamComponent.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(
	HandInteractionSpec, "UXTools.HandInteraction",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

AUxtHandInteractionActor* HandActor;
UUxtNearPointerComponent* NearPointer;
UUxtFarPointerComponent* FarPointer;
UUxtFarBeamComponent* FarBeam;
UTestGrabTarget* Target;
FFrameQueue FrameQueue;

const FString TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
const float TargetScale = 0.3f;
const FVector TargetLocation = FVector(150, 0, 0);
const FVector NearPoint = FVector(135, 0, 0);
const FVector FarPoint = FVector(40, 0, 0);
FTransform InitialFarBeamTransform;

END_DEFINE_SPEC(HandInteractionSpec)

void HandInteractionSpec::Define()
{
	LatentBeforeEach([this](const FDoneDelegate& Done) {
		UWorld* World = UxtTestUtils::LoadMap("/Game/UXToolsGame/Tests/Maps/TestEmpty");
		TestNotNull("World", World);

		UxtTestUtils::EnableTestHandTracker();
		FrameQueue.Init(&World->GetTimerManager());

		HandActor = World->SpawnActor<AUxtHandInteractionActor>();
		HandActor->SetHand(EControllerHand::Left);

		NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
		FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();
		FarBeam = HandActor->FindComponentByClass<UUxtFarBeamComponent>();
		TestTrue("Components found", NearPointer && FarPointer && FarBeam);

		Target = UxtTestUtils::CreateNearPointerGrabTarget(World, TargetLocation, TargetFilename, TargetScale);

		FrameQueue.Enqueue([Done]() { Done.Execute(); });
	});

	AfterEach([this] {
		FrameQueue.Reset();
		UxtTestUtils::DisableTestHandTracker();
		UxtTestUtils::ExitGame();
	});

	LatentIt("should activate near pointer if close to a target", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint); });

		// Skip one frame as pointers take a frame to start ticking when activated by the hand interaction actor
		FrameQueue.Skip();

		FrameQueue.Enqueue([this, Done] {
			FVector ClosestPoint;
			FVector Normal;
			TestEqual(TEXT("Near pointer focusing target"), NearPointer->GetFocusedGrabTarget(ClosestPoint, Normal), (UObject*)Target);
			TestTrue(TEXT("Near pointer active"), NearPointer->IsActive());
			TestFalse(TEXT("Far pointer active"), FarPointer->IsActive());

			Done.Execute();
		});
	});

	LatentIt("should activate far pointer if not close to a target", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FarPoint); });

		FrameQueue.Enqueue([this, Done] {
			FVector ClosestPoint;
			FVector Normal;
			TestNull(TEXT("Near pointer not focused"), NearPointer->GetFocusedGrabTarget(ClosestPoint, Normal));
			TestFalse(TEXT("Near pointer active"), NearPointer->IsActive());
			TestTrue(TEXT("Far pointer active"), FarPointer->IsActive());

			Done.Execute();
		});
	});

	LatentIt("should deactivate pointer when hand is not in the pointing pose", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue(
			[this] { UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(FVector::DownVector.Rotation().Quaternion()); });

		// Skip one frame as pointers take a frame to start ticking when activated by the hand interaction actor
		FrameQueue.Skip();

		FrameQueue.Enqueue([this, Done] {
			TestFalse("Near pointer activated", NearPointer->IsActive());
			TestFalse("Far pointer active", FarPointer->IsActive());
			Done.Execute();
		});
	});

	LatentIt("should deactivate pointers when tracking is lost", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue([this] {
			// Enable focus lock to ensure it is released when tracking is lost.
			// Have to activate the pointers as well to test that focus lock is released during deactivation.
			NearPointer->Activate();
			NearPointer->SetFocusLocked(true);

			// Far pointer doesn't expect focus requests while not enabled (i.e. having ticked with tracking) so we just check for
			// deactivation.
			FarPointer->Activate();

			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint);
			UxtTestUtils::GetTestHandTracker().SetTracked(false);
		});

		FrameQueue.Enqueue([this, Done] {
			FVector ClosestPoint;
			FVector Normal;
			TestNull(TEXT("Near pointer not focused"), NearPointer->GetFocusedGrabTarget(ClosestPoint, Normal));
			TestFalse(TEXT("Near pointer active"), NearPointer->IsActive());
			TestFalse(TEXT("Far pointer active"), FarPointer->IsActive());

			TestFalse(TEXT("Near pointer focus locked"), NearPointer->GetFocusLocked());

			Done.Execute();
		});
	});

	LatentIt("should reset proximity detection when tracking is lost", [this](const FDoneDelegate& Done) {
		UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint);

		FrameQueue.Enqueue([this] {
			TestTrue("Near pointer active", NearPointer->IsActive());
			TestFalse("Far pointer active", FarPointer->IsActive());

			UxtTestUtils::GetTestHandTracker().SetTracked(false);
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FarPoint);
		});

		FrameQueue.Enqueue([this, Done] {
			// Moving away from the near target while not tracking
			// should immediately remove the target from proximity detection.
			TestFalse("Near pointer active", NearPointer->IsActive());
			TestFalse("Far pointer active", FarPointer->IsActive());

			UxtTestUtils::GetTestHandTracker().SetTracked(true);
		});

		FrameQueue.Enqueue([this, Done] {
			// Starting tracking away from the near target should enable the far pointer.
			TestFalse("Near pointer active", NearPointer->IsActive());
			TestTrue("Far pointer active", FarPointer->IsActive());

			Done.Execute();
		});
	});

	LatentIt("should not transition between near and far interaction modes when a pointer is locked", [this](const FDoneDelegate& Done) {
		UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint);

		FrameQueue.Enqueue([this] {
			TestTrue("Near pointer active when close to target", NearPointer->IsActive());
			TestFalse("Far pointer active", FarPointer->IsActive());
			NearPointer->SetFocusLocked(true);
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FarPoint);
		});

		FrameQueue.Enqueue([this] {
			TestTrue("Near pointer active when locked and far from target", NearPointer->IsActive());
			TestFalse("Far pointer active", FarPointer->IsActive());
			NearPointer->SetFocusLocked(false);
		});

		// Skip one frame as pointers take a frame to start ticking when activated by the hand interaction actor
		FrameQueue.Skip();

		FrameQueue.Enqueue([this] {
			TestFalse("Near pointer active", NearPointer->IsActive());
			TestTrue("Far pointer active when far from target", FarPointer->IsActive());
			FarPointer->SetFocusLocked(true);
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint);
		});

		FrameQueue.Enqueue([this] {
			TestFalse("Near pointer active", NearPointer->IsActive());
			TestTrue("Far pointer active when locked and close to target", FarPointer->IsActive());
			FarPointer->SetFocusLocked(false);
		});

		FrameQueue.Enqueue([this, Done] {
			TestTrue("Near pointer active", NearPointer->IsActive());
			TestFalse("Far pointer active", FarPointer->IsActive());
			Done.Execute();
		});
	});
	LatentIt("should not affect its far beam's transform", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue([this] {
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);
			InitialFarBeamTransform = FarBeam->GetComponentTransform();
		});
		FrameQueue.Enqueue([this] {
			HandActor->SetActorTransform(FTransform(FQuat::MakeFromEuler(FVector(0, 90, 0)), FVector(100, 200, 300), FVector(2)));
		});
		FrameQueue.Enqueue([this] {
			TestTrue(
				TEXT("HandInteractionActor transformation did not affect the far beam"),
				FarBeam->GetComponentTransform().Equals(InitialFarBeamTransform));
		});
		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
