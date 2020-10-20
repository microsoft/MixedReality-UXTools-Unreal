// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "FarPointerListenerComponent.h"
#include "FarTargetTestComponent.h"
#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtFarBeamComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Input/UxtFarPointerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFarPointerSpec, "UXTools.FarPointer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
FUxtTestHandTracker* HandTracker;
EControllerHand Hand = EControllerHand::Right;
UUxtFarPointerComponent* Pointer;
UFarPointerListenerComponent* PointerListener;
UFarTargetTestComponent* FarTarget;
FFrameQueue FrameQueue;
UPrimitiveComponent* HitPrimitive = nullptr;
FVector TargetLocation = FVector(200, 0, 0);
END_DEFINE_SPEC(FFarPointerSpec)

void FFarPointerSpec::Define()
{
	LatentBeforeEach([this](const FDoneDelegate& Done) {
		// Load map
		UWorld* World = UxtTestUtils::LoadMap("/Game/UXToolsGame/Tests/Maps/TestEmpty");
		TestNotNull("Load map result", World);

		// Frame queue
		FrameQueue.Init(World->GetGameInstance()->TimerManager);

		// Hand tracker
		HandTracker = &UxtTestUtils::EnableTestHandTracker();
		HandTracker->SetAllJointPositions(FVector::ZeroVector);
		HandTracker->SetAllJointOrientations(FQuat::Identity);

		// Pointer actor
		{
			AActor* PointerActor = World->SpawnActor<AActor>();

			// Root
			USceneComponent* Root = NewObject<USceneComponent>(PointerActor);
			Root->RegisterComponent();
			PointerActor->SetRootComponent(Root);

			// Pointer
			Pointer = NewObject<UUxtFarPointerComponent>(PointerActor);
			Pointer->RegisterComponent();
			Pointer->Hand = Hand;

			// Subscribe to enable/disable events
			PointerListener = NewObject<UFarPointerListenerComponent>(PointerActor);
			Pointer->OnFarPointerEnabled.AddDynamic(PointerListener, &UFarPointerListenerComponent::OnFarPointerEnabled);
			Pointer->OnFarPointerDisabled.AddDynamic(PointerListener, &UFarPointerListenerComponent::OnFarPointerDisabled);
			PointerListener->RegisterComponent();

			// Beam
			UUxtFarBeamComponent* Beam = NewObject<UUxtFarBeamComponent>(PointerActor);
			Beam->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			Beam->RegisterComponent();

			// Cursor
			UUxtFarCursorComponent* Cursor = NewObject<UUxtFarCursorComponent>(PointerActor);
			Cursor->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
			Cursor->RegisterComponent();
		}

		// Target actor
		{
			AActor* TargetActor = World->SpawnActor<AActor>();

			// Far target
			FarTarget = NewObject<UFarTargetTestComponent>(TargetActor);
			FarTarget->RegisterComponent();

			// Mesh
			UStaticMeshComponent* MeshComponent = UxtTestUtils::CreateBoxStaticMesh(TargetActor);
			TargetActor->SetRootComponent(MeshComponent);
			MeshComponent->RegisterComponent();
			HitPrimitive = MeshComponent;

			TargetActor->SetActorLocation(TargetLocation);
		}

		// Tick once so the far pointer gets to read the hand position
		FrameQueue.Enqueue([Done]() { Done.Execute(); });
	});

	AfterEach([this]() {
		FrameQueue.Reset();
		UxtTestUtils::DisableTestHandTracker();
		UxtTestUtils::ExitGame();
	});

	It("should have correct focus target", [this]() {
		UFarTargetTestComponent* FocusTarget = CastChecked<UFarTargetTestComponent>(Pointer->GetFocusTarget());
		TestEqual("Object is targeted", FocusTarget, FarTarget);
	});

	LatentIt("should raise the correct events on far targets", [this](const FDoneDelegate& Done) {
		// We expect the pointer to be focusing the target from the first frame
		TestEqual("EnterFarFocus", FarTarget->NumEnter, 1);
		TestEqual("UpdatedFarFocus", FarTarget->NumUpdated, 0);
		TestEqual("ExitFarFocus", FarTarget->NumExit, 0);
		TestEqual("Pressed", FarTarget->NumPressed, 0);
		TestEqual("Dragged", FarTarget->NumDragged, 0);
		TestEqual("Released", FarTarget->NumReleased, 0);

		FrameQueue.Enqueue([this]() {
			TestEqual("UpdatedFarFocus", FarTarget->NumUpdated, 1);
			HandTracker->SetSelectPressed(true);
		});

		FrameQueue.Enqueue([this]() {
			TestEqual("Pressed", FarTarget->NumPressed, 1);
			TestTrue("IsPressed", Pointer->IsPressed());
		});

		FrameQueue.Enqueue([this]() {
			TestEqual("Dragged", FarTarget->NumDragged, 1);
			HandTracker->SetSelectPressed(false);
		});

		FrameQueue.Enqueue([this]() {
			TestEqual("Released", FarTarget->NumReleased, 1);
			TestFalse("IsPressed", Pointer->IsPressed());
			HandTracker->SetAllJointPositions(FVector(-500, 0, 0));
		});

		FrameQueue.Enqueue([this, Done]() {
			TestEqual("EnterFarFocus", FarTarget->NumEnter, 1);
			TestEqual("UpdatedFarFocus", FarTarget->NumUpdated, 4);
			TestEqual("ExitFarFocus", FarTarget->NumExit, 1);
			TestEqual("Pressed", FarTarget->NumPressed, 1);

			// A drag event is raised on the same frame the pointer is released because pointer pose update is
			// processed before press state update.
			TestEqual("Dragged", FarTarget->NumDragged, 2);

			TestEqual("Released", FarTarget->NumReleased, 1);
			Done.Execute();
		});
	});

	It("should use hand transform", [this]() {
		FQuat Orientation;
		FVector Position;
		HandTracker->GetPointerPose(Hand, Orientation, Position);

		TestEqual("Origin", Pointer->GetPointerOrigin(), Position);
		TestEqual("Orientation", Pointer->GetPointerOrientation(), Orientation);

		Pointer->RayStartOffset = 10;
		FVector RayStartExpected = Position + Orientation.GetForwardVector() * Pointer->RayStartOffset;
		TestEqual("Ray start", Pointer->GetRayStart(), RayStartExpected);
	});

	LatentIt("should become disabled/enabled on tracking loss/recovery", [this](const FDoneDelegate& Done) {
		TestEqual("NumEnabled", PointerListener->NumEnabled, 1);
		TestEqual("NumDisabled", PointerListener->NumDisabled, 0);
		TestTrue("IsEnabled", Pointer->IsEnabled());

		HandTracker->SetTracked(false);

		FrameQueue.Enqueue([this]() {
			TestEqual("NumEnabled", PointerListener->NumEnabled, 1);
			TestEqual("NumDisabled", PointerListener->NumDisabled, 1);
			TestFalse("IsEnabled", Pointer->IsEnabled());

			HandTracker->SetTracked(true);
		});

		FrameQueue.Enqueue([this, Done]() {
			TestEqual("NumEnabled", PointerListener->NumEnabled, 2);
			TestEqual("NumDisabled", PointerListener->NumDisabled, 1);
			TestTrue("IsEnabled", Pointer->IsEnabled());

			Done.Execute();
		});
	});

	It("should report correct hit info", [this]() {
		TestEqual("Hit Primitive", Pointer->GetHitPrimitive(), HitPrimitive);
		TestEqual("Hit Normal", Pointer->GetHitNormal(), FVector(-1, 0, 0));
		TestEqual("Hit Point", Pointer->GetHitPoint(), TargetLocation - FVector(50, 0, 0));
	});

	LatentIt("should lock to current target when requested", [this](const FDoneDelegate& Done) {
		Pointer->SetFocusLocked(true);

		AActor* TargetActor = FarTarget->GetOwner();
		FVector NewTargetLocation = TargetLocation + FVector(0, 400, 0);
		TargetActor->SetActorLocation(NewTargetLocation);

		FrameQueue.Enqueue([this, NewTargetLocation, TargetActor]() {
			TestTrue("FocusLocked", Pointer->GetFocusLocked());
			TestEqual("Hit Primitive", Pointer->GetHitPrimitive(), HitPrimitive);
			TestEqual("Hit Normal", Pointer->GetHitNormal(), FVector(-1, 0, 0));
			TestEqual("Hit Point", Pointer->GetHitPoint(), NewTargetLocation - FVector(50, 0, 0));
			TargetActor->Destroy();
		});

		FrameQueue.Enqueue([this, Done]() {
			TestFalse("FocusLocked", Pointer->GetFocusLocked());
			TestNull("Hit Primitive", Pointer->GetHitPrimitive());
			Done.Execute();
		});
	});

	LatentIt("should release lock on tracking loss", [this](const FDoneDelegate& Done) {
		Pointer->SetFocusLocked(true);
		HandTracker->SetTracked(false);

		FrameQueue.Enqueue([this, Done]() {
			TestFalse("FocusLocked", Pointer->GetFocusLocked());
			Done.Execute();
		});
	});
}

#endif // #if WITH_DEV_AUTOMATION_TESTS
