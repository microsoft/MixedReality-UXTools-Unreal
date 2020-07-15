// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "GenericManipulatorTestComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Tests/AutomationCommon.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Interactions/Constraints/UxtFaceUserConstraint.h"
#include "Interactions/Constraints/UxtFixedRotationToWorldConstraint.h"
#include "Interactions/Constraints/UxtFixedRotationToUserConstraint.h"
#include "Interactions/Constraints/UxtRotationAxisConstraint.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Components/SceneComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	UUxtGenericManipulatorComponent* CreateTestComponent(const FComponentReference& TargetComponent = FComponentReference())
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
		Manipulator->TargetComponent = TargetComponent;
		Manipulator->SetSmoothing(0.0f);
		Manipulator->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return Manipulator;
	}

	const FRotator GetCameraRotation(UWorld* World)
	{
		APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(World, 0);
		return CameraManager->GetCameraRotation();
	}
}

BEGIN_DEFINE_SPEC(ManipulatorConstraintSpec, "UXTools.GenericManipulator.Constraints", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	void EnqueueFixedRotationToWorldConstraintTests();
	void EnqueueFixedRotationToUserConstraintTests();
	void EnqueueFaceUserConstraintTests();
	void EnqueueRotationAxisConstraintTests();
	//todo: move MoveAxisConstraint tests here

	UUxtGenericManipulatorComponent* Target;
	FFrameQueue FrameQueue;

	// Must be configured by Describe block if needed
	FUxtTestHand LeftHand = FUxtTestHand(EControllerHand::Left);
	FUxtTestHand RightHand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(ManipulatorConstraintSpec)

void ManipulatorConstraintSpec::Define()
{
	BeforeEach([this]
		{
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			UxtTestUtils::EnableTestHandTracker();

			Target = CreateTestComponent();
		});

	AfterEach([this]
		{
			Target->GetOwner()->Destroy();
			Target = nullptr;

			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();
		});

	Describe("Near Interaction", [this]
		{
			BeforeEach([this]
				{
					LeftHand.Configure(EUxtInteractionMode::Near, TargetLocation);
					RightHand.Configure(EUxtInteractionMode::Near, TargetLocation);
				});

			AfterEach([this]
				{
					LeftHand.Reset();
					RightHand.Reset();
				});

			EnqueueFixedRotationToWorldConstraintTests();
			EnqueueFixedRotationToUserConstraintTests();
			EnqueueFaceUserConstraintTests();
			EnqueueRotationAxisConstraintTests();
		});

	Describe("Far Interaction", [this]
		{
			BeforeEach([this]
				{
					LeftHand.Configure(EUxtInteractionMode::Far, TargetLocation);
					RightHand.Configure(EUxtInteractionMode::Far, TargetLocation);
				});

			AfterEach([this]
				{
					LeftHand.Reset();
					RightHand.Reset();
				});

			EnqueueFixedRotationToWorldConstraintTests();
			EnqueueFixedRotationToUserConstraintTests();
			EnqueueFaceUserConstraintTests();
			EnqueueRotationAxisConstraintTests();
		});
}

void ManipulatorConstraintSpec::EnqueueFixedRotationToWorldConstraintTests()
{
	LatentIt("Should maintain fixed rotation to world with one hand", [this](const FDoneDelegate& Done)
		{
			UUxtFixedRotationToWorldConstraint* Constraint = NewObject<UUxtFixedRotationToWorldConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();
			const FTransform ExpectedTransform = Target->GetOwner()->GetTransform();

			FrameQueue.Enqueue([this]
				{
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
					RightHand.Rotate(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90)));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransform]
				{
					const FTransform Result = Target->GetOwner()->GetTransform();
					TestTrue("Objects rotation didn't change with hand rotation", Result.Equals(ExpectedTransform));
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

	LatentIt("Should maintain fixed rotation to world with two hands", [this](const FDoneDelegate& Done)
		{
			UUxtFixedRotationToWorldConstraint* Constraint = NewObject<UUxtFixedRotationToWorldConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();
			const FTransform ExpectedTransform = Target->GetOwner()->GetTransform();

			FrameQueue.Enqueue([this]
				{
					LeftHand.Translate(FVector(0, -50, 0));
					RightHand.Translate(FVector(0, 50, 0));

					LeftHand.SetGrabbing(true);
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
					LeftHand.Translate(FVector(0, 50, -50));
					RightHand.Translate(FVector(0, -50, 50));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransform]
				{
					const FTransform Result = Target->GetOwner()->GetTransform();
					TestTrue("Objects rotation didn't change with hands rotation", Result.Equals(ExpectedTransform));
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
}

void ManipulatorConstraintSpec::EnqueueFixedRotationToUserConstraintTests()
{
	LatentIt("Should maintain fixed rotation to user with one hand", [this](const FDoneDelegate& Done)
		{
			UUxtFixedRotationToUserConstraint* Constraint = NewObject<UUxtFixedRotationToUserConstraint>(Target->GetOwner());
			Constraint->bExcludeRoll = false;
			Constraint->RegisterComponent();
			const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
			const FTransform ExpectedTransformAfterHandRotation = TransformTarget;

			// store relative rotation to camera
			const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
			const FQuat RelativeRotationToCameraStart = CameraRotation.Quaternion().Inverse() * TransformTarget.GetRotation();
			const FQuat HeadTilt = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90));

			USceneComponent* CameraController = UxtTestUtils::CreateTestCamera(Constraint->GetWorld());

			FrameQueue.Enqueue([this]
				{
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this, HeadTilt]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
					RightHand.Rotate(HeadTilt);
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransformAfterHandRotation, CameraController, HeadTilt]
				{
					const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
					TestTrue("Objects rotation didn't change with hand rotation", TransformTarget.GetRotation().Rotator().Equals(ExpectedTransformAfterHandRotation.Rotator()));

					// tilt head
					CameraController->SetRelativeRotation(HeadTilt);
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, Constraint, RelativeRotationToCameraStart, HeadTilt, CameraController]
				{
					// check if our object rotated and if the rotation relative to head / camera is still the same
					const FTransform Result = Target->TransformTarget->GetComponentTransform();
					TestTrue("Objects rotation changed with head rotation", HeadTilt.Rotator().Equals(Result.Rotator()));

					const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
					const FQuat RelativeRotationToCamera = CameraRotation.Quaternion().Inverse() * Result.GetRotation();
					TestTrue("Objects rotation relative to camera / head stayed the same", RelativeRotationToCameraStart.Rotator().Equals(RelativeRotationToCamera.Rotator()));
				
					CameraController->GetOwner()->Destroy();
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

	LatentIt("Should maintain fixed rotation to user with two hands", [this](const FDoneDelegate& Done)
		{
			const FQuat ExpectedRotation(FVector::ForwardVector, FMath::DegreesToRadians(90));
			UUxtFixedRotationToUserConstraint* Constraint = NewObject<UUxtFixedRotationToUserConstraint>(Target->GetOwner());
			Constraint->bExcludeRoll = false;
			Constraint->RegisterComponent();

			const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
			const FTransform ExpectedTransformAfterHandRotation = TransformTarget;

			// store relative rotation to camera
			const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
			const FQuat RelativeRotationToCameraStart = CameraRotation.Quaternion().Inverse() * TransformTarget.GetRotation();
			const FQuat HeadTilt = FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90));

			USceneComponent* CameraController = UxtTestUtils::CreateTestCamera(Constraint->GetWorld());

			FrameQueue.Enqueue([this]
				{
					LeftHand.Translate(FVector(0, -50, 0));
					RightHand.Translate(FVector(0, 50, 0));

					LeftHand.SetGrabbing(true);
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
					LeftHand.Translate(FVector(0, 50, -50));
					RightHand.Translate(FVector(0, -50, 50));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedTransformAfterHandRotation, CameraController, HeadTilt]
				{

					const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
					TestTrue("Objects rotation didn't change with hand rotation", TransformTarget.GetRotation().Rotator().Equals(ExpectedTransformAfterHandRotation.Rotator()));

					// tilt head
					CameraController->SetRelativeRotation(HeadTilt);
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, Constraint, RelativeRotationToCameraStart, HeadTilt, CameraController]
				{
					// check if our object rotated and if the rotation relative to head / camera is still the same
					const FTransform Result = Target->TransformTarget->GetComponentTransform();
					FQuat targetRot = Result.GetRotation();
					TestTrue("Objects rotation changed with head rotation", HeadTilt.Rotator().Equals(Result.Rotator()));

					const FRotator CameraRotation = GetCameraRotation(Constraint->GetWorld());
					FQuat CameraRot = CameraRotation.Quaternion();
					const FQuat RelativeRotationToCamera = CameraRotation.Quaternion().Inverse() * Result.GetRotation();
					TestTrue("Objects rotation relative to camera / head stayed the same", RelativeRotationToCameraStart.Rotator().Equals(RelativeRotationToCamera.Rotator()));
				
					CameraController->GetOwner()->Destroy();
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
}

void ManipulatorConstraintSpec::EnqueueFaceUserConstraintTests()
{
	LatentIt("Should face user with one hand", [this](const FDoneDelegate& Done)
		{
			UUxtFaceUserConstraint* Constraint = NewObject<UUxtFaceUserConstraint>(Target->GetOwner());
			Constraint->RegisterComponent();

			FrameQueue.Enqueue([this]
				{
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
					RightHand.Rotate(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90)));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, Constraint]
				{
					const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
					FVector DirectionToTarget = TransformTarget.GetLocation() - UUxtFunctionLibrary::GetHeadPose(Constraint->GetWorld()).GetLocation();
					FQuat OrientationFacingTarget = FRotationMatrix::MakeFromXZ(-DirectionToTarget, FVector::UpVector).ToQuat();
					
					TestTrue("Object is facing user", TransformTarget.GetRotation().Rotator().Equals(OrientationFacingTarget.Rotator()));
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

	LatentIt("Should face away from user with two hands", [this](const FDoneDelegate& Done)
		{
			UUxtFaceUserConstraint* Constraint = NewObject<UUxtFaceUserConstraint>(Target->GetOwner());
			Constraint->bFaceAway = true;
			Constraint->RegisterComponent();
			
			FrameQueue.Enqueue([this]
				{
					LeftHand.Translate(FVector(0, -50, 0));
					RightHand.Translate(FVector(0, 50, 0));

					LeftHand.SetGrabbing(true);
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
					LeftHand.Translate(FVector(50, 50, -50));
					RightHand.Translate(FVector(-50, -50, 50));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, Constraint]
				{
					const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();

					FVector DirectionToTarget = TransformTarget.GetLocation() - UUxtFunctionLibrary::GetHeadPose(Constraint->GetWorld()).GetLocation();
					FQuat OrientationFacingTarget = FRotationMatrix::MakeFromXZ(DirectionToTarget, FVector::UpVector).ToQuat();

					TestTrue("Object is facing away from user", 
					TransformTarget.GetRotation().Rotator().Equals(OrientationFacingTarget.Rotator()));
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
}

void ManipulatorConstraintSpec::EnqueueRotationAxisConstraintTests()
{
	LatentIt("Should restrict rotation in X axis with one hand", [this](const FDoneDelegate& Done)
		{
			UUxtRotationAxisConstraint* Constraint = NewObject<UUxtRotationAxisConstraint>(Target->GetOwner());
			Constraint->ConstraintOnRotation = static_cast<int32>(EUxtAxisFlags::X);
			Constraint->RegisterComponent();
			const FRotator ExpectedRotation = Target->TransformTarget->GetComponentTransform().Rotator();

			FrameQueue.Enqueue([this]
				{
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
					RightHand.Rotate(FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90)));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedRotation]
				{
					const FTransform& TransformTarget = Target->TransformTarget->GetComponentTransform();
					TestTrue("Objects rotation didn't change with hand rotation", 
					TransformTarget.Rotator().Equals(ExpectedRotation));
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

	LatentIt("Should restrict rotation in X axis with two hands", [this](const FDoneDelegate& Done)
		{
			UUxtRotationAxisConstraint* Constraint = NewObject<UUxtRotationAxisConstraint>(Target->GetOwner());
			Constraint->ConstraintOnRotation = static_cast<int32>(EUxtAxisFlags::X);
			Constraint->RegisterComponent();
			const FRotator ExpectedRotation = Target->TransformTarget->GetComponentTransform().Rotator();

			FrameQueue.Enqueue([this]
				{
					LeftHand.Translate(FVector(0, -50, 0));
					RightHand.Translate(FVector(0, 50, 0));

					LeftHand.SetGrabbing(true);
					RightHand.SetGrabbing(true);
				});

			FrameQueue.Enqueue([this]
				{
					TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
					LeftHand.Translate(FVector(0, 50, -50));
					RightHand.Translate(FVector(0, -50, 50));
				});

			FrameQueue.Skip();

			FrameQueue.Enqueue([this, ExpectedRotation]
				{
					const FTransform Result = Target->GetOwner()->GetTransform();
					const FRotator ResultRotation = Result.Rotator();
					TestTrue("Objects rotation didn't change with hands rotation", ExpectedRotation.Equals(ResultRotation));
				});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
