// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "GenericManipulatorTestComponent.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Components/ArrowComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Interactions/UxtInteractionMode.h"
#include "Tests/AutomationCommon.h"

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

		// Scene component for testing re-targeting
		UArrowComponent* Scene = NewObject<UArrowComponent>(Actor);
		Scene->RegisterComponent();

		// Generic manipulator component
		UUxtGenericManipulatorComponent* Manipulator = NewObject<UUxtGenericManipulatorComponent>(Actor);
		Manipulator->TargetComponent = TargetComponent;
		Manipulator->ReleaseBehavior = static_cast<int32>(EUxtReleaseBehavior::None);
		Manipulator->SetSmoothing(0.0f);
		Manipulator->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return Manipulator;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	GenericManipulatorSpec, "UXTools.GenericManipulator",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueInteractionTests();

UUxtGenericManipulatorComponent* Target;
FFrameQueue FrameQueue;

// Must be configured by Describe block if needed
EUxtInteractionMode InteractionMode;
FUxtTestHand LeftHand = FUxtTestHand(EControllerHand::Left);
FUxtTestHand RightHand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(GenericManipulatorSpec)

void GenericManipulatorSpec::Define()
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

		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();
	});

	Describe("Component Targeting", [this] {
		It("should default to parent actor", [this] {
			USceneComponent* ExpectedTarget = Target->GetOwner()->GetRootComponent();
			TestEqual("Correct component targeted", Target->TransformTarget, ExpectedTarget);
		});

		It("should target user defined component (same actor)", [this] {
			FComponentReference TargetComponentReference;
			TargetComponentReference.ComponentProperty = "ArrowComponent_0";

			UUxtGenericManipulatorComponent* ManipulatorComponent = CreateTestComponent(TargetComponentReference);
			USceneComponent* ExpectedTarget = ManipulatorComponent->GetOwner()->FindComponentByClass<UArrowComponent>();
			TestEqual("Correct component targeted", ManipulatorComponent->TransformTarget, ExpectedTarget);

			ManipulatorComponent->GetOwner()->Destroy();
		});

		It("should target user defined component (different actor)", [this] {
			FComponentReference TargetComponentReference;
			TargetComponentReference.OtherActor = Target->GetOwner();
			TargetComponentReference.ComponentProperty = "ArrowComponent_0";

			UUxtGenericManipulatorComponent* ManipulatorComponent = CreateTestComponent(TargetComponentReference);
			USceneComponent* ExpectedTarget = Target->GetOwner()->FindComponentByClass<UArrowComponent>();
			TestEqual("Correct component targeted", ManipulatorComponent->TransformTarget, ExpectedTarget);

			ManipulatorComponent->GetOwner()->Destroy();
		});
	});

	Describe("Manipulation Events", [this] {
		BeforeEach([this] { RightHand.Configure(EUxtInteractionMode::Near, TargetLocation); });

		AfterEach([this] { RightHand.Reset(); });

		LatentIt("should raise transform update events", [this](const FDoneDelegate& Done) {
			UGenericManipulatorTestComponent* EventCaptureComponent = NewObject<UGenericManipulatorTestComponent>(Target->GetOwner());
			Target->OnUpdateTransform.AddDynamic(EventCaptureComponent, &UGenericManipulatorTestComponent::UpdateTransform);
			EventCaptureComponent->RegisterComponent();

			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
				RightHand.Translate(FVector::RightVector * 10);
			});

			FrameQueue.Enqueue([this, EventCaptureComponent] {
				TestTrue("Transform update events triggered", EventCaptureComponent->TransformUpdateCount > 0);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
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

		Describe("Uniformly Scaled Object", [this] { EnqueueInteractionTests(); });

		Describe("Non-uniformly Scaled Object", [this] {
			BeforeEach([this] { Target->TransformTarget->SetRelativeScale3D(FVector(1, 2, 3)); });

			EnqueueInteractionTests();
		});
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

		Describe("Uniformly Scaled Object", [this] { EnqueueInteractionTests(); });

		Describe("Non-uniformly Scaled Object", [this] {
			BeforeEach([this] { Target->TransformTarget->SetRelativeScale3D(FVector(1, 2, 3)); });

			EnqueueInteractionTests();
		});
	});
}

void GenericManipulatorSpec::EnqueueInteractionTests()
{
	LatentIt("should move with one hand", [this](const FDoneDelegate& Done) {
		const float DistanceScaling = InteractionMode == EUxtInteractionMode::Far ? 6.40312433 : 1;
		const FVector TranslationDelta = FVector(200, 200, 200);
		const FVector ExpectedLocation = TargetLocation * DistanceScaling + TranslationDelta;

		FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

		FrameQueue.Enqueue([this, TranslationDelta] {
			TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
			RightHand.Translate(TranslationDelta);
		});

		FrameQueue.Skip();

		FrameQueue.Enqueue([this, ExpectedLocation] {
			const FVector Result = Target->TransformTarget->GetRelativeLocation();
			TestEqual("Object has moved", Result, ExpectedLocation);
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});

	LatentIt("should move with two hands", [this](const FDoneDelegate& Done) {
		const float DistanceScaling = InteractionMode == EUxtInteractionMode::Far ? 6.40312433 : 1;
		const FVector TranslationDelta = FVector(200, 200, 200);
		const FVector ExpectedLocation = TargetLocation * DistanceScaling + TranslationDelta;

		FrameQueue.Enqueue([this] {
			LeftHand.Translate(FVector(0, -50, 0));
			RightHand.Translate(FVector(0, 50, 0));

			RightHand.SetGrabbing(true);
			LeftHand.SetGrabbing(true);
		});

		FrameQueue.Enqueue([this, TranslationDelta] {
			TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
			RightHand.Translate(TranslationDelta);
			LeftHand.Translate(TranslationDelta);
		});

		FrameQueue.Skip();

		FrameQueue.Enqueue([this, ExpectedLocation] {
			const FVector Result = Target->TransformTarget->GetRelativeLocation();
			TestEqual("Object has moved", Result, ExpectedLocation);
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});

	LatentIt("should rotate around center with one hand", [this](const FDoneDelegate& Done) {
		const FQuat ExpectedRotation(FVector::ForwardVector, FMath::DegreesToRadians(90));
		const FTransform ExpectedTransform(ExpectedRotation, TargetLocation, Target->TransformTarget->GetRelativeScale3D());

		FrameQueue.Enqueue([this] {
			Target->OneHandRotationMode = EUxtOneHandRotationMode::RotateAboutObjectCenter;
			RightHand.SetGrabbing(true);
		});

		FrameQueue.Enqueue([this, ExpectedRotation] {
			TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
			RightHand.Rotate(ExpectedRotation);
		});

		FrameQueue.Skip();

		FrameQueue.Enqueue([this, ExpectedTransform] {
			const FTransform Result = Target->TransformTarget->GetRelativeTransform();
			TestTrue("Object is rotated", Result.Equals(ExpectedTransform));
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});

	LatentIt("should rotate around grab point with one hand", [this](const FDoneDelegate& Done) {
		const FQuat ExpectedRotation(FVector::ForwardVector, FMath::DegreesToRadians(90));
		const FVector ExpectedLocation(TargetLocation + FVector(0, 50, -50));
		const FTransform ExpectedTransform(ExpectedRotation, ExpectedLocation, Target->TransformTarget->GetRelativeScale3D());

		FrameQueue.Enqueue([this] {
			Target->OneHandRotationMode = EUxtOneHandRotationMode::RotateAboutGrabPoint;
			RightHand.Translate(FVector(0, 50, 0));
			RightHand.SetGrabbing(true);
		});

		FrameQueue.Enqueue([this, ExpectedRotation] {
			TestTrue("Component is grabbed", Target->GetGrabPointers().Num() > 0);
			RightHand.Rotate(ExpectedRotation);
		});

		FrameQueue.Skip();

		FrameQueue.Enqueue([this, ExpectedTransform] {
			const FTransform Result = Target->TransformTarget->GetRelativeTransform();
			TestTrue("Object is rotated", Result.Equals(ExpectedTransform));
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});

	LatentIt("should rotate with two hands", [this](const FDoneDelegate& Done) {
		const FQuat ExpectedRotation(FVector::ForwardVector, FMath::DegreesToRadians(90));
		const FTransform ExpectedTransform(ExpectedRotation, TargetLocation, Target->TransformTarget->GetRelativeScale3D());

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
			const FTransform Result = Target->TransformTarget->GetRelativeTransform();
			TestTrue("Object is rotated", Result.Equals(ExpectedTransform));
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});

	LatentIt("should scale with two hands", [this](const FDoneDelegate& Done) {
		const FVector ExpectedScale = Target->TransformTarget->GetRelativeScale3D() * 2;

		FrameQueue.Enqueue([this] {
			LeftHand.Translate(FVector(0, -50, 0));
			RightHand.Translate(FVector(0, 50, 0));

			LeftHand.SetGrabbing(true);
			RightHand.SetGrabbing(true);
		});

		FrameQueue.Enqueue([this] {
			TestTrue("Component is grabbed", Target->GetGrabPointers().Num() == 2);
			LeftHand.Translate(FVector(0, -50, 0));
			RightHand.Translate(FVector(0, 50, 0));
		});

		FrameQueue.Skip();

		FrameQueue.Enqueue([this, ExpectedScale] {
			const FVector Result = Target->TransformTarget->GetRelativeScale3D();
			TestTrue("Object is scaled", Result.Equals(ExpectedScale));
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});

	LatentIt("should disable physics on grab", [this](const FDoneDelegate& Done) {
		UStaticMeshComponent* StaticMesh = Target->GetOwner()->FindComponentByClass<UStaticMeshComponent>();

		FrameQueue.Enqueue([this, StaticMesh] {
			StaticMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
			StaticMesh->SetEnableGravity(false);
			StaticMesh->SetSimulatePhysics(true);
		});

		FrameQueue.Enqueue([this, StaticMesh] { RightHand.SetGrabbing(true); });

		FrameQueue.Enqueue([this, StaticMesh] {
			TestTrue("Object is grabbed", Target->GetGrabPointers().Num() > 0);
			TestFalse("Physics is disabled", StaticMesh->IsSimulatingPhysics());
			RightHand.SetGrabbing(false);
		});

		FrameQueue.Enqueue([this, StaticMesh] {
			TestEqual("Object is not grabbed", Target->GetGrabPointers().Num(), 0);
			TestTrue("Physics is enabled", StaticMesh->IsSimulatingPhysics());
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
