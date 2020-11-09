// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "HandConstraintListener.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Behaviors/UxtPalmUpConstraintComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(
	PalmUpConstraintComponentSpec, "UXTools.PalmUpConstraintComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)
FFrameQueue FrameQueue;
UUxtPalmUpConstraintComponent* PalmUpConstraint;
UHandConstraintListener* HandConstraintListener;

const FVector PalmPosition = FVector(70, 20, -15);
const FQuat PalmFacingCameraOrientation = FRotationMatrix::MakeFromZY(FVector(73, 17, -12), FVector(0, 30, -5)).ToQuat();
const FQuat PalmFacingAwayOrientation = FRotationMatrix::MakeFromZY(FVector(-20, 50, -12), FVector(0, 30, -5)).ToQuat();
const FVector IndexTipPositionFlat = PalmPosition + PalmFacingCameraOrientation.RotateVector(FVector(5, 1, 0.5f));
const FVector RingTipPositionFlat = PalmPosition + PalmFacingCameraOrientation.RotateVector(FVector(4, -3, 0.2f));
const FVector IndexTipPositionUnflat = PalmPosition + PalmFacingCameraOrientation.RotateVector(FVector(5, 0, 2));
const FVector RingTipPositionUnflat = PalmPosition + PalmFacingCameraOrientation.RotateVector(FVector(3, -1, -3));
END_DEFINE_SPEC(PalmUpConstraintComponentSpec)

void PalmUpConstraintComponentSpec::Define()
{
	Describe("PalmUp constraint", [this] {
		BeforeEach([this]() {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			UxtTestUtils::EnableTestHandTracker();

			AActor* Actor = World->SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(Actor);
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();

			PalmUpConstraint = NewObject<UUxtPalmUpConstraintComponent>(Actor);
			// Create listener before registering so all events are recorded
			HandConstraintListener = NewObject<UHandConstraintListener>(PalmUpConstraint);
			PalmUpConstraint->OnConstraintActivated.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnConstraintActivated);
			PalmUpConstraint->OnConstraintDeactivated.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnConstraintDeactivated);
			PalmUpConstraint->OnBeginTracking.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnBeginTracking);
			PalmUpConstraint->OnEndTracking.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnEndTracking);
			PalmUpConstraint->RegisterComponent();

			// Register all new components.
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();

			PalmUpConstraint->GetOwner()->Destroy();
			PalmUpConstraint = nullptr;
		});

		LatentIt("should only be active when facing camera", [this](const FDoneDelegate& Done) {
			UxtTestUtils::GetTestHandTracker().SetTracked(true);
			UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(PalmPosition);
			UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingCameraOrientation);

			PalmUpConstraint->Hand = EControllerHand::Left;
			PalmUpConstraint->OffsetMode = EUxtHandConstraintOffsetMode::HandRotation;
			PalmUpConstraint->MaxPalmAngle = 80.0f;
			PalmUpConstraint->bRequireFlatHand = false;

			FrameQueue.Enqueue([this]() {
				TestTrue("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 0);

				// Rotate away from camera
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingAwayOrientation);
			});
			FrameQueue.Enqueue([this]() {
				// Hand bounds are only computed when the hand is accepted
				TestFalse("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 1);

				// Rotate back towards camera
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingCameraOrientation);
			});
			FrameQueue.Enqueue([this, Done]() {
				TestTrue("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 2);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 1);

				Done.Execute();
			});
		});

		LatentIt("should only be active when facing the camera and hand is flat", [this](const FDoneDelegate& Done) {
			UxtTestUtils::GetTestHandTracker().SetTracked(true);
			UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(PalmPosition);
			UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingCameraOrientation);

			PalmUpConstraint->Hand = EControllerHand::Left;
			PalmUpConstraint->OffsetMode = EUxtHandConstraintOffsetMode::HandRotation;
			PalmUpConstraint->MaxPalmAngle = 80.0f;
			PalmUpConstraint->bRequireFlatHand = true;
			PalmUpConstraint->MaxFlatHandAngle = 45.0f;

			// Setup finger tips to form a triangle aligned with the palm, used as the measure of flatness
			UxtTestUtils::GetTestHandTracker().SetJointPosition(IndexTipPositionFlat, EControllerHand::AnyHand, EUxtHandJoint::IndexTip);
			UxtTestUtils::GetTestHandTracker().SetJointPosition(RingTipPositionFlat, EControllerHand::AnyHand, EUxtHandJoint::RingTip);

			FrameQueue.Enqueue([this]() {
				TestTrue("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 0);

				// Rotate away from camera
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingAwayOrientation);
			});
			FrameQueue.Enqueue([this]() {
				// Hand bounds are only computed when the hand is accepted
				TestFalse("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 1);

				// Rotate back towards camera
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingCameraOrientation);
			});
			FrameQueue.Enqueue([this, Done]() {
				TestTrue("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 2);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 1);

				Done.Execute();
			});
		});

		LatentIt("should never be active when hand is not flat", [this](const FDoneDelegate& Done) {
			UxtTestUtils::GetTestHandTracker().SetTracked(true);
			UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(PalmPosition);
			UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingCameraOrientation);

			PalmUpConstraint->Hand = EControllerHand::Left;
			PalmUpConstraint->OffsetMode = EUxtHandConstraintOffsetMode::HandRotation;
			PalmUpConstraint->MaxPalmAngle = 80.0f;
			PalmUpConstraint->bRequireFlatHand = true;
			PalmUpConstraint->MaxFlatHandAngle = 45.0f;

			// Setup finger tips to form a triangle angled against the palm, failing the flatness test
			UxtTestUtils::GetTestHandTracker().SetJointPosition(IndexTipPositionUnflat, EControllerHand::AnyHand, EUxtHandJoint::IndexTip);
			UxtTestUtils::GetTestHandTracker().SetJointPosition(RingTipPositionUnflat, EControllerHand::AnyHand, EUxtHandJoint::RingTip);

			FrameQueue.Enqueue([this]() {
				// Hand bounds are only computed when the hand is accepted
				TestFalse("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 0);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 0);

				// Rotate away from camera
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingAwayOrientation);
			});
			FrameQueue.Enqueue([this]() {
				TestFalse("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 0);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 0);

				// Rotate back towards camera
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(PalmFacingCameraOrientation);
			});
			FrameQueue.Enqueue([this, Done]() {
				TestFalse("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", PalmUpConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 0);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 0);

				Done.Execute();
			});
		});

		LatentIt("should require gaze to activate", [this](const FDoneDelegate& Done) {
			UxtTestUtils::GetTestHandTracker().SetTracked(true);
			UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
			UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(FVector::DownVector.ToOrientationQuat());

			PalmUpConstraint->Zone = EUxtHandConstraintZone::AboveFingerTips; // To avoid conflicts with the joints used for the hand plane.
			PalmUpConstraint->Hand = EControllerHand::Left;
			PalmUpConstraint->OffsetMode = EUxtHandConstraintOffsetMode::HandRotation;
			PalmUpConstraint->MaxPalmAngle = 80.0f;
			PalmUpConstraint->bRequireGaze = true;
			PalmUpConstraint->HeadGazeProximityThreshold = 1.0f;

			const FVector GazeLocation(100.0f, 0.0f, 0.0f);
			const FVector NoGazeLocation(100.0f, 10.0f, 0.0f);

			// Start with no gaze
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NoGazeLocation);
			UxtTestUtils::GetTestHandTracker().SetJointPosition(
				NoGazeLocation + FVector(0, -1, 1), EControllerHand::AnyHand, EUxtHandJoint::IndexMetacarpal);
			UxtTestUtils::GetTestHandTracker().SetJointPosition(
				NoGazeLocation + FVector(0, 1, 1), EControllerHand::AnyHand, EUxtHandJoint::LittleMetacarpal);

			FrameQueue.Enqueue([this, GazeLocation]() {
				// Not active with no gaze
				TestFalse("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", PalmUpConstraint->IsConstraintActive());

				// Move into head gaze
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(GazeLocation);
				UxtTestUtils::GetTestHandTracker().SetJointPosition(
					GazeLocation + FVector(0, -1, 1), EControllerHand::AnyHand, EUxtHandJoint::IndexMetacarpal);
				UxtTestUtils::GetTestHandTracker().SetJointPosition(
					GazeLocation + FVector(0, 1, 1), EControllerHand::AnyHand, EUxtHandJoint::LittleMetacarpal);
			});
			FrameQueue.Enqueue([this, NoGazeLocation]() {
				// Active with gaze
				TestTrue("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", PalmUpConstraint->IsConstraintActive());

				// Move out of head gaze
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NoGazeLocation);
				UxtTestUtils::GetTestHandTracker().SetJointPosition(
					NoGazeLocation + FVector(0, -1, 1), EControllerHand::AnyHand, EUxtHandJoint::IndexMetacarpal);
				UxtTestUtils::GetTestHandTracker().SetJointPosition(
					NoGazeLocation + FVector(0, 1, 1), EControllerHand::AnyHand, EUxtHandJoint::LittleMetacarpal);
			});
			FrameQueue.Enqueue([this, Done]() {
				// Remains active after moving out of gaze
				TestTrue("Hand bounds are valid", (bool)PalmUpConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", PalmUpConstraint->IsConstraintActive());

				Done.Execute();
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
