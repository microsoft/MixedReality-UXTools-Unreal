// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/Constraints/UxtMoveAxisConstraint.h"
#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Interactions/UxtManipulationFlags.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UxToolsTests/Tests/FrameQueue.h"
#include "UxToolsTests/Tests/UxtTestHandTracker.h"
#include "UxToolsTests/Tests/UxtTestUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const static FVector Center = FVector(150, 0, 0);
	const static FVector NearPointerPosition = Center + FVector(-15, 0, 0);
	const static FVector TestDelta = FVector(200, 200, 200);
	const static FVector LeftHandOffset = FVector(0, 0, 5);
} // namespace

BEGIN_DEFINE_SPEC(
	ManipulatorMoveAxisConstraintSpec, "UXTools.GenericManipulator.Constraints",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtNearPointerComponent* NearPointer = nullptr;
UUxtFarPointerComponent* FarPointerRight = nullptr;
UUxtFarPointerComponent* FarPointerLeft = nullptr;
AActor* TargetActor = nullptr;
FFrameQueue FrameQueue;
UUxtMoveAxisConstraint* MoveAxisConstraint = nullptr;
FVector TestPositionCache;

END_DEFINE_SPEC(ManipulatorMoveAxisConstraintSpec)

void ManipulatorMoveAxisConstraintSpec::Define()
{
	Describe("Move Axis Constraint", [this] {
		BeforeEach([this] {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			UxtTestUtils::EnableTestHandTracker();
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);

			UUxtGenericManipulatorComponent* Manipulator =
				UxtTestUtils::CreateTestBoxWithComponent<UUxtGenericManipulatorComponent>(World, Center);
			Manipulator->SetSmoothing(0.0f); // disable smoothing to get accurate results when moving objects

			// Add constraint
			MoveAxisConstraint = NewObject<UUxtMoveAxisConstraint>(Manipulator->GetOwner());
			MoveAxisConstraint->RegisterComponent();
			MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::X);

			TargetActor = Manipulator->GetOwner();

			// Register all new components.
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();
			MoveAxisConstraint = nullptr;
			TargetActor->Destroy();
			TargetActor = nullptr;

			UxtTestUtils::ExitGame();
		});

		LatentIt("should restrict movement in X direction for near interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				// create near pointer
				NearPointer = UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), TEXT("GrabTestPointer"), NearPointerPosition);
				TargetActor->SetActorLocation(Center);

				// Enable grab.
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				FVector MoveToPosition = NearPointerPosition + TestDelta;
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(MoveToPosition);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure it isn't moved in X axis
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ExpectedLocation = Center + FVector(0, 200, 200);
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);
				NearPointer->GetOwner()->Destroy();
				NearPointer = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should apply constraints that got added during runtime / grab started", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				// create near pointer
				NearPointer = UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), TEXT("GrabTestPointer"), NearPointerPosition);
				TargetActor->SetActorLocation(Center);
				// destroy constraint
				MoveAxisConstraint->DestroyComponent();
				MoveAxisConstraint = nullptr;
			});

			FrameQueue.Enqueue([this] {
				// Enable grab.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				FVector MoveToPosition = NearPointerPosition + TestDelta;
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(MoveToPosition);
			});

			FrameQueue.Enqueue([this] {
				// cache new position and add constraint
				TestPositionCache = TargetActor->GetActorLocation();
				MoveAxisConstraint = NewObject<UUxtMoveAxisConstraint>(TargetActor);
				MoveAxisConstraint->RegisterComponent();
				MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::X);
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				FVector MoveToPosition = NearPointerPosition;
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(MoveToPosition);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure it isn't moved in X axis
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ExpectedLocation = Center;
				ExpectedLocation.X = TestPositionCache.X;
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);
				NearPointer->GetOwner()->Destroy();
				NearPointer = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should react on constraint component detach during runtime/ grab started", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				// create near pointer
				NearPointer = UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), TEXT("GrabTestPointer"), NearPointerPosition);
				TargetActor->SetActorLocation(Center);

				// Enable grab.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// detach constraint
				MoveAxisConstraint->DestroyComponent();
				MoveAxisConstraint = nullptr;
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				FVector MoveToPosition = NearPointerPosition + TestDelta;
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(MoveToPosition);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure we moved as expected / no constraint applied
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ConstraintLocation = Center + FVector(0, 200, 200);
				TestNotEqual(TEXT("object still had constraint applied"), NewLocation, ConstraintLocation);
				TestTrue(TEXT("object did not move as expected in x direciton"), NewLocation.X >= (Center.X + 200));

				NearPointer->GetOwner()->Destroy();
				NearPointer = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should restrict movement in X direction local space", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				// create near pointer
				NearPointer = UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), TEXT("GrabTestPointer"), NearPointerPosition);
				TargetActor->SetActorLocation(Center);
				TargetActor->SetActorRotation(FQuat(FVector::RightVector, FMath::DegreesToRadians(90)));
				MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::X);
				MoveAxisConstraint->bUseLocalSpaceForConstraint = true;

				// Enable grab.
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				FVector MoveToPosition = NearPointerPosition + TestDelta;
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(MoveToPosition);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure it isn't moved in X axis
				FVector NewLocation = TargetActor->GetActorLocation();
				// due to rotated object locking local X will be global Z
				FVector ExpectedLocation = FVector(350, 200, 0);
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation, 0.001f);
				NearPointer->GetOwner()->Destroy();
				NearPointer = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should restrict movement in X direction for far interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				FarPointerRight = UxtTestUtils::CreateFarPointer(
					UxtTestUtils::GetTestWorld(), TEXT("FarTestPointer"), FVector::ZeroVector, EControllerHand::Right);
				TargetActor->SetActorLocation(FVector(200, 0, 0));
			});

			FrameQueue.Enqueue([this] {
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(true);
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TestDelta);
			});

			FrameQueue.Enqueue([this, Done] {
				// make sure it isn't moved in X axis
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ExpectedLocation = TestDelta;
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);
				FarPointerRight->GetOwner()->Destroy();
				FarPointerRight = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should allow multiple constraints", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				FarPointerRight = UxtTestUtils::CreateFarPointer(
					UxtTestUtils::GetTestWorld(), TEXT("FarTestPointer"), FVector::ZeroVector, EControllerHand::Right);
				TargetActor->SetActorLocation(FVector(200, 0, 0));

				// adding another constraint that will lock y axis
				UUxtMoveAxisConstraint* MoveAxisSingleHandConstraint = NewObject<UUxtMoveAxisConstraint>(TargetActor);
				MoveAxisSingleHandConstraint->RegisterComponent();
				MoveAxisSingleHandConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::Y);
			});

			FrameQueue.Enqueue([this] {
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(true);
			});

			FrameQueue.Enqueue([this] {
				// Move hand and wait for update
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TestDelta, EControllerHand::Right);
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, Done] {
				// make sure it isn't moved in X and Y axis
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ExpectedLocation = FVector(200, 0, 200);
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);

				FarPointerRight->GetOwner()->Destroy();
				FarPointerRight = nullptr;
				Done.Execute();
			});
		});

		LatentIt("should restrict movement for two hand far interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				FarPointerRight = UxtTestUtils::CreateFarPointer(
					UxtTestUtils::GetTestWorld(), TEXT("FarTestPointer"), FVector::ZeroVector, EControllerHand::Right);
				FarPointerLeft = UxtTestUtils::CreateFarPointer(
					UxtTestUtils::GetTestWorld(), TEXT("FarTestPointerLeft"), LeftHandOffset, EControllerHand::Left);
				TargetActor->SetActorLocation(FVector(200, 0, 0));
			});

			FrameQueue.Enqueue([this] {
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetSelectPressed(true);
			});

			FrameQueue.Enqueue([this] {
				// Move hands and wait for update
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TestDelta, EControllerHand::Right);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TestDelta + LeftHandOffset, EControllerHand::Left);
			});

			FrameQueue.Enqueue([this] {
				// make sure it isn't moved in X axis
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ExpectedLocation = TestDelta;
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);

				// adding another constraint that applies only to one handed movement
				UUxtMoveAxisConstraint* MoveAxisSingleHandConstraint = NewObject<UUxtMoveAxisConstraint>(TargetActor);
				MoveAxisSingleHandConstraint->RegisterComponent();
				MoveAxisSingleHandConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::Y);
				MoveAxisSingleHandConstraint->HandType = static_cast<int32>(EUxtGenericManipulationMode::OneHanded);
			});

			FrameQueue.Enqueue([this, Done] {
				FVector TwoHandOffset = FVector(400, 400, 400);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TwoHandOffset, EControllerHand::Right);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(TwoHandOffset + LeftHandOffset, EControllerHand::Left);
			});

			FrameQueue.Enqueue([this, Done] {
				// only the first (x axis) constraint should be applied to this translation
				FVector NewLocation = TargetActor->GetActorLocation();
				FVector ExpectedLocation = FVector(200, 400, 400);
				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);

				FarPointerRight->GetOwner()->Destroy();
				FarPointerRight = nullptr;
				FarPointerLeft->GetOwner()->Destroy();
				FarPointerLeft = nullptr;
				Done.Execute();
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
