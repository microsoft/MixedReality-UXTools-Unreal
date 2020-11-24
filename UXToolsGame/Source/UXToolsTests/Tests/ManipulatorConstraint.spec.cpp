// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "GenericManipulatorTestComponent.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Components/SceneComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/Constraints/UxtFaceUserConstraint.h"
#include "Interactions/Constraints/UxtFixedDistanceConstraint.h"
#include "Interactions/Constraints/UxtFixedRotationToUserConstraint.h"
#include "Interactions/Constraints/UxtFixedRotationToWorldConstraint.h"
#include "Interactions/Constraints/UxtMaintainApparentSizeConstraint.h"
#include "Interactions/Constraints/UxtMoveAxisConstraint.h"
#include "Interactions/Constraints/UxtRotationAxisConstraint.h"
#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	UUxtGenericManipulatorComponent* CreateTestComponent()
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		// Box Mesh
		UStaticMeshComponent* Mesh = UxtTestUtils::CreateBoxStaticMesh(Actor);
		Actor->SetRootComponent(Mesh);
		Mesh->RegisterComponent();

		// Generic manipulator component
		UUxtGenericManipulatorComponent* Manipulator = NewObject<UUxtGenericManipulatorComponent>(Actor);
		Manipulator->OneHandRotationMode = EUxtOneHandRotationMode::RotateAboutObjectCenter;
		Manipulator->SetSmoothing(0.0f);
		Manipulator->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return Manipulator;
	}

	FRotator GetCameraRotation(UWorld* World)
	{
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);
		return CameraManager->GetCameraRotation();
	}
} // namespace

BEGIN_DEFINE_SPEC(
	ManipulatorConstraintSpec, "UXTools.GenericManipulator.Constraints",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueFixedRotationToWorldConstraintTests();
void EnqueueFixedRotationToUserConstraintTests();
void EnqueueFaceUserConstraintTests();
void EnqueueRotationAxisConstraintTests();
void EnqueueFixedDistanceConstraintTests();
void EnqueueMaintainApparentSizeConstraintTests();
void EnqueueMoveAxisConstraintTests();

UUxtGenericManipulatorComponent* Target;
UUxtTransformConstraint* Constraint;
FFrameQueue FrameQueue;

// Must be configured by Describe block if needed
EUxtInteractionMode InteractionMode;
FUxtTestHand LeftHand = FUxtTestHand(EControllerHand::Left);
FUxtTestHand RightHand = FUxtTestHand(EControllerHand::Right);

// Cache for a position to use between frames
FVector PositionCache;

END_DEFINE_SPEC(ManipulatorConstraintSpec)

void ManipulatorConstraintSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		UWorld* World = UxtTestUtils::GetTestWorld();
		FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

		UxtTestUtils::EnableTestHandTracker();

		Target = CreateTestComponent();
	});

	AfterEach([this] {
		Target->GetOwner()->Destroy();
		Target = nullptr;
		Constraint = nullptr;

		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();
	});

	Describe("Near Interaction", [this] {
		BeforeEach([this] {
			InteractionMode = EUxtInteractionMode::Near;
			LeftHand.Configure(InteractionMode, TargetLocation);
			RightHand.Configure(InteractionMode, TargetLocation);
		});

		AfterEach([this] {
			LeftHand.Reset();
			RightHand.Reset();
		});

		EnqueueFixedRotationToWorldConstraintTests();
		EnqueueFixedRotationToUserConstraintTests();
		EnqueueFaceUserConstraintTests();
		EnqueueRotationAxisConstraintTests();
		EnqueueFixedDistanceConstraintTests();
		EnqueueMaintainApparentSizeConstraintTests();
		EnqueueMoveAxisConstraintTests();
	});

	Describe("Far Interaction", [this] {
		BeforeEach([this] {
			InteractionMode = EUxtInteractionMode::Far;
			LeftHand.Configure(InteractionMode, TargetLocation);
			RightHand.Configure(InteractionMode, TargetLocation);
		});

		AfterEach([this] {
			LeftHand.Reset();
			RightHand.Reset();
		});

		EnqueueFixedRotationToWorldConstraintTests();
		EnqueueFixedRotationToUserConstraintTests();
		EnqueueFaceUserConstraintTests();
		EnqueueRotationAxisConstraintTests();
		EnqueueFixedDistanceConstraintTests();
		EnqueueMaintainApparentSizeConstraintTests();
		EnqueueMoveAxisConstraintTests();
	});
}

void ManipulatorConstraintSpec::EnqueueFixedRotationToWorldConstraintTests()
{
	Describe("UxtFixedRotationToWorldConstraint", [this] {
		BeforeEach([this] {
			Constraint = NewObject<UUxtFixedRotationToWorldConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();
		});

		LatentIt("Should maintain fixed rotation to world with one hand", [this](const FDoneDelegate& Done) {
			const FTransform ExpectedTransform = Target->GetOwner()->GetTransform();

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Rotate(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90)));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransform] {
				const FTransform Result = Target->GetOwner()->GetTransform();
				TestTrue("Objects rotation didn't change with hand rotation", Result.Equals(ExpectedTransform));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Should maintain fixed rotation to world with two hands", [this](const FDoneDelegate& Done) {
			const FTransform ExpectedTransform = Target->GetOwner()->GetTransform();

			FrameQueue.Enqueue([this] {
				LeftHand.Translate(FVector(0, -50, 0));
				RightHand.Translate(FVector(0, 50, 0));

				LeftHand.SetGrabbing(true);
				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
				LeftHand.Translate(FVector(0, 50, -50));
				RightHand.Translate(FVector(0, -50, 50));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransform] {
				const FTransform Result = Target->GetOwner()->GetTransform();
				TestTrue("Objects rotation didn't change with hands rotation", Result.Equals(ExpectedTransform));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void ManipulatorConstraintSpec::EnqueueFixedRotationToUserConstraintTests()
{
	Describe("UxtFixedRotationToUserConstraint", [this] {
		BeforeEach([this] {
			UUxtFixedRotationToUserConstraint* FixedRotationToUserConstraint =
				NewObject<UUxtFixedRotationToUserConstraint>(Target->GetOwner());
			FixedRotationToUserConstraint->bExcludeRoll = false;

			Constraint = FixedRotationToUserConstraint;
			Constraint->RegisterComponent();
		});

		LatentIt("Should maintain fixed rotation to user with one hand", [this](const FDoneDelegate& Done) {
			const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
			const FTransform ExpectedTransformAfterHandRotation = TransformTarget;

			// store relative rotation to camera
			const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
			const FQuat RelativeRotationToCameraStart = CameraRotation.Quaternion().Inverse() * TransformTarget.GetRotation();
			const FQuat HeadTilt = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90));

			USceneComponent* CameraController = UxtTestUtils::CreateTestCamera(Constraint->GetWorld());

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this, HeadTilt] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Rotate(HeadTilt);
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransformAfterHandRotation, CameraController, HeadTilt] {
				const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
				TestTrue(
					"Objects rotation didn't change with hand rotation",
					TransformTarget.GetRotation().Rotator().Equals(ExpectedTransformAfterHandRotation.Rotator()));

				// tilt head
				CameraController->SetRelativeRotation(HeadTilt);
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, RelativeRotationToCameraStart, HeadTilt, CameraController] {
				// check if our object rotated and if the rotation relative to head / camera is still the same
				const FTransform Result = Target->TransformTarget->GetComponentTransform();
				TestTrue("Objects rotation changed with head rotation", HeadTilt.Rotator().Equals(Result.Rotator()));

				const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
				const FQuat RelativeRotationToCamera = CameraRotation.Quaternion().Inverse() * Result.GetRotation();
				TestTrue(
					"Objects rotation relative to camera / head stayed the same",
					RelativeRotationToCameraStart.Rotator().Equals(RelativeRotationToCamera.Rotator()));

				CameraController->GetOwner()->Destroy();
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Should maintain fixed rotation to user with two hands", [this](const FDoneDelegate& Done) {
			const FQuat ExpectedRotation(FVector::ForwardVector, FMath::DegreesToRadians(90));
			const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
			const FTransform ExpectedTransformAfterHandRotation = TransformTarget;

			// store relative rotation to camera
			const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
			const FQuat RelativeRotationToCameraStart = CameraRotation.Quaternion().Inverse() * TransformTarget.GetRotation();
			const FQuat HeadTilt = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90));

			USceneComponent* CameraController = UxtTestUtils::CreateTestCamera(Constraint->GetWorld());

			FrameQueue.Enqueue([this] {
				LeftHand.Translate(FVector(0, -50, 0));
				RightHand.Translate(FVector(0, 50, 0));

				LeftHand.SetGrabbing(true);
				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
				LeftHand.Translate(FVector(0, 50, -50));
				RightHand.Translate(FVector(0, -50, 50));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransformAfterHandRotation, CameraController, HeadTilt] {
				const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
				TestTrue(
					"Objects rotation didn't change with hand rotation",
					TransformTarget.GetRotation().Rotator().Equals(ExpectedTransformAfterHandRotation.Rotator()));

				// tilt head
				CameraController->SetRelativeRotation(HeadTilt);
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, RelativeRotationToCameraStart, HeadTilt, CameraController] {
				// check if our object rotated and if the rotation relative to head / camera is still the same
				const FTransform Result = Target->TransformTarget->GetComponentTransform();
				FQuat targetRot = Result.GetRotation();
				TestTrue("Objects rotation changed with head rotation", HeadTilt.Rotator().Equals(Result.Rotator()));

				const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
				FQuat CameraRot = CameraRotation.Quaternion();
				const FQuat RelativeRotationToCamera = CameraRotation.Quaternion().Inverse() * Result.GetRotation();
				TestTrue(
					"Objects rotation relative to camera / head stayed the same",
					RelativeRotationToCameraStart.Rotator().Equals(RelativeRotationToCamera.Rotator()));

				CameraController->GetOwner()->Destroy();
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void ManipulatorConstraintSpec::EnqueueFaceUserConstraintTests()
{
	Describe("UxtFaceUserConstraint", [this] {
		BeforeEach([this] {
			Constraint = NewObject<UUxtFaceUserConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();
		});

		LatentIt("Should face user with one hand", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Rotate(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90)));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this] {
				const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
				FVector DirectionToTarget =
					TransformTarget.GetLocation() - UUxtFunctionLibrary::GetHeadPose(Constraint->GetWorld()).GetLocation();
				FQuat OrientationFacingTarget = FRotationMatrix::MakeFromXZ(-DirectionToTarget, FVector::UpVector).ToQuat();

				TestTrue("Object is facing user", TransformTarget.GetRotation().Rotator().Equals(OrientationFacingTarget.Rotator()));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Should face away from user with two hands", [this](const FDoneDelegate& Done) {
			UUxtFaceUserConstraint* FaceUserConstraint = Cast<UUxtFaceUserConstraint>(Constraint);
			FaceUserConstraint->bFaceAway = true;

			FrameQueue.Enqueue([this] {
				LeftHand.Translate(FVector(0, -50, 0));
				RightHand.Translate(FVector(0, 50, 0));

				LeftHand.SetGrabbing(true);
				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
				LeftHand.Translate(FVector(50, 50, -50));
				RightHand.Translate(FVector(-50, -50, 50));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this] {
				const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();

				FVector DirectionToTarget =
					TransformTarget.GetLocation() - UUxtFunctionLibrary::GetHeadPose(Constraint->GetWorld()).GetLocation();
				FQuat OrientationFacingTarget = FRotationMatrix::MakeFromXZ(DirectionToTarget, FVector::UpVector).ToQuat();

				TestTrue(
					"Object is facing away from user", TransformTarget.GetRotation().Rotator().Equals(OrientationFacingTarget.Rotator()));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void ManipulatorConstraintSpec::EnqueueRotationAxisConstraintTests()
{
	Describe("UxtRotationAxisConstraint", [this] {
		BeforeEach([this] {
			UUxtRotationAxisConstraint* RotationAxisConstraint = NewObject<UUxtRotationAxisConstraint>(Target->GetOwner());
			RotationAxisConstraint->ConstraintOnRotation = static_cast<int32>(EUxtAxisFlags::X);

			Constraint = RotationAxisConstraint;
			Constraint->RegisterComponent();
		});

		LatentIt("Should restrict rotation in X axis with one hand", [this](const FDoneDelegate& Done) {
			const FRotator ExpectedRotation = Target->TransformTarget->GetComponentTransform().Rotator();

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Rotate(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90)));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedRotation] {
				const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
				TestTrue("Objects rotation didn't change with hand rotation", TransformTarget.Rotator().Equals(ExpectedRotation));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Should restrict rotation in X axis with two hands", [this](const FDoneDelegate& Done) {
			const FRotator ExpectedRotation = Target->TransformTarget->GetComponentTransform().Rotator();

			FrameQueue.Enqueue([this] {
				LeftHand.Translate(FVector(0, -50, 0));
				RightHand.Translate(FVector(0, 50, 0));

				LeftHand.SetGrabbing(true);
				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
				LeftHand.Translate(FVector(0, 50, -50));
				RightHand.Translate(FVector(0, -50, 50));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedRotation] {
				const FTransform Result = Target->GetOwner()->GetTransform();
				const FRotator ResultRotation = Result.Rotator();
				TestTrue("Objects rotation didn't change with hands rotation", ExpectedRotation.Equals(ResultRotation));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void ManipulatorConstraintSpec::EnqueueFixedDistanceConstraintTests()
{
	Describe("UxtFixedDistanceConstraint", [this] {
		BeforeEach([this] {
			Constraint = NewObject<UUxtFixedDistanceConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();
		});

		LatentIt("Should maintain fixed distance to camera", [this](const FDoneDelegate& Done) {
			const FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(UxtTestUtils::GetTestWorld());
			const float ExpectedDistance = FVector::Dist(Target->GetOwner()->GetActorLocation(), HeadPose.GetLocation());

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Translate(FVector(100, 100, 100));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, HeadPose, ExpectedDistance] {
				const float Result = FVector::Dist(Target->GetOwner()->GetActorLocation(), HeadPose.GetLocation());
				TestEqual("Distance did not change", Result, ExpectedDistance);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Should maintain fixed distance to object", [this](const FDoneDelegate& Done) {
			AActor* ConstraintObject = UxtTestUtils::GetTestWorld()->SpawnActor<AActor>();
			USceneComponent* RootComponent = NewObject<USceneComponent>(ConstraintObject);
			RootComponent->SetWorldLocation(TargetLocation + FVector(50, 50, 50));
			ConstraintObject->SetRootComponent(RootComponent);

			UUxtFixedDistanceConstraint* FixedDistanceConstraint = Cast<UUxtFixedDistanceConstraint>(Constraint);
			FixedDistanceConstraint->ConstraintComponent.OtherActor = ConstraintObject;

			const float ExpectedDistance = FVector::Dist(Target->GetOwner()->GetActorLocation(), ConstraintObject->GetActorLocation());

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Translate(FVector(100, 100, 100));
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ConstraintObject, ExpectedDistance] {
				const float Result = FVector::Dist(Target->GetOwner()->GetActorLocation(), ConstraintObject->GetActorLocation());
				TestEqual("Distance did not change", Result, ExpectedDistance);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });

			ConstraintObject->Destroy();
		});
	});
}

void ManipulatorConstraintSpec::EnqueueMaintainApparentSizeConstraintTests()
{
	Describe("UxtMaintainApparentSizeConstraint", [this] {
		BeforeEach([this] {
			Constraint = NewObject<UUxtMaintainApparentSizeConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();
		});

		LatentIt("Should maintain apparent size with one hand", [this](const FDoneDelegate& Done) {
			const FVector Translation = FVector::ForwardVector * 200;

			const FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(UxtTestUtils::GetTestWorld());
			const float InitialDistance = FVector::Dist(Target->GetOwner()->GetActorLocation(), HeadPose.GetLocation());
			const float DistanceScaling = InteractionMode == EUxtInteractionMode::Far ? 3 : 1;
			const float ExpectedDistance = InitialDistance + (Translation.Size() * DistanceScaling);
			const FVector ExpectedScale = (ExpectedDistance / InitialDistance) * Target->GetOwner()->GetActorScale();

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this, Translation] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);

				RightHand.Translate(Translation);
			});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedScale] {
				const FVector Result = Target->GetOwner()->GetActorScale();

				TestEqual("Should have scaled accordingly", Result, ExpectedScale, 0.1);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Should maintain apparent size with two hands", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				LeftHand.Translate(FVector(0, -50, 0));
				RightHand.Translate(FVector(0, 50, 0));

				LeftHand.SetGrabbing(true);
				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);

				LeftHand.Translate(FVector(0, 50, -50));
				RightHand.Translate(FVector(0, -50, 50));
			});

			FrameQueue.Skip(2);

			FrameQueue.Enqueue([this] {
				const FVector Result = Target->GetOwner()->GetActorScale();

				TestEqual("Should have scaled accordingly", Result, FVector(1, 1, 1), 0.1);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void ManipulatorConstraintSpec::EnqueueMoveAxisConstraintTests()
{
	Describe("UxtMoveAxisConstraint", [this] {
		BeforeEach([this] {
			UUxtMoveAxisConstraint* MoveAxisConstraint = NewObject<UUxtMoveAxisConstraint>(Target->GetOwner());
			MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::X);

			Constraint = MoveAxisConstraint;
			Constraint->RegisterComponent();
		});

		LatentIt("should restrict movement in X direction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] { RightHand.Translate(FVector(200, 200, 200)); });

			FrameQueue.Enqueue([this] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				const FVector ExpectedLocation = TargetLocation + FVector(0, 200, 200);

				TestEqual(TEXT("Object didn't move as expected"), NewLocation, ExpectedLocation);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should apply constraints that get added during runtime", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				Constraint->DestroyComponent();
				Constraint = nullptr;
			});

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] { RightHand.Translate(FVector(200, 200, 200)); });

			FrameQueue.Enqueue([this] {
				PositionCache = Target->GetOwner()->GetActorLocation();

				UUxtMoveAxisConstraint* MoveAxisConstraint = NewObject<UUxtMoveAxisConstraint>(Target->GetOwner());
				MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::X);
				Constraint = MoveAxisConstraint;
				Constraint->RegisterComponent();
			});

			FrameQueue.Enqueue([this] { RightHand.Translate(FVector(-200, -200, -200)); });

			FrameQueue.Enqueue([this] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				const FVector ExpectedLocation = FVector(PositionCache.X, 0, 0);

				TestEqual(TEXT("Object didn't move as expected"), NewLocation, ExpectedLocation);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should react on constraint component detach during runtime", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				Constraint->DestroyComponent();
				Constraint = nullptr;
			});

			FrameQueue.Enqueue([this] { RightHand.Translate(FVector(200, 200, 200)); });

			FrameQueue.Enqueue([this] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				const FVector ConstraintLocation = TargetLocation + FVector(0, 200, 200);

				TestNotEqual(TEXT("object still had constraint applied"), NewLocation, ConstraintLocation);
				TestTrue(TEXT("object did not move as expected in x direciton"), NewLocation.X >= (TargetLocation.X + 200));
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should restrict movement in X direction local space", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				Target->GetOwner()->SetActorRotation(FQuat(FVector::RightVector, FMath::DegreesToRadians(90)));

				UUxtMoveAxisConstraint* MoveAxisConstraint = Cast<UUxtMoveAxisConstraint>(Constraint);
				MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::X);
				MoveAxisConstraint->bUseLocalSpaceForConstraint = true;

				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] { RightHand.Translate(FVector(200, 200, 200)); });

			FrameQueue.Enqueue([this, Done] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				// due to rotated object locking local X will be global Z
				const FVector ExpectedLocation = FVector(NewLocation.X, NewLocation.Y, 0);

				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation, 0.001f);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should allow multiple constraints", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				UUxtMoveAxisConstraint* MoveAxisConstraint = NewObject<UUxtMoveAxisConstraint>(Target->GetOwner());
				MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::Y);
				MoveAxisConstraint->RegisterComponent();
			});

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] { RightHand.Translate(FVector(200, 200, 200)); });

			FrameQueue.Enqueue([this, Done] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				const FVector ExpectedLocation = TargetLocation + FVector(0, 0, 200);

				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should restrict movement for two hand interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				LeftHand.Translate(FVector(0, 0, 5));

				RightHand.SetGrabbing(true);
				LeftHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				RightHand.Translate(FVector(200, 200, 200));
				LeftHand.Translate(FVector(200, 200, 200));
			});

			FrameQueue.Enqueue([this] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				const FVector ExpectedLocation = TargetLocation + FVector(0, 200, 200);

				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);

				// Add another constraint that applies only to one handed movement
				UUxtMoveAxisConstraint* MoveAxisConstraint = NewObject<UUxtMoveAxisConstraint>(Target->GetOwner());
				MoveAxisConstraint->ConstraintOnMovement = static_cast<int32>(EUxtAxisFlags::Y);
				MoveAxisConstraint->HandType = static_cast<int32>(EUxtGrabMode::OneHanded);
				MoveAxisConstraint->RegisterComponent();
			});

			FrameQueue.Enqueue([this, Done] {
				RightHand.Translate(FVector(200, 200, 200));
				LeftHand.Translate(FVector(200, 200, 200));
			});

			FrameQueue.Enqueue([this, Done] {
				const FVector NewLocation = Target->GetOwner()->GetActorLocation();
				const FVector ExpectedLocation = TargetLocation + FVector(0, 400, 400);

				TestEqual(TEXT("object didn't move as expected"), NewLocation, ExpectedLocation);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
