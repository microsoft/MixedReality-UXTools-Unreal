// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtBoundsControlComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	UUxtBoundsControlComponent* CreateTestComponent()
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		// Box Mesh
		UStaticMeshComponent* Mesh = UxtTestUtils::CreateBoxStaticMesh(Actor);
		Actor->SetRootComponent(Mesh);
		Mesh->RegisterComponent();

		// Generic manipulator component
		UUxtGenericManipulatorComponent* Manipulator = NewObject<UUxtGenericManipulatorComponent>(Actor);
		Manipulator->SetSmoothing(0.0f);
		Manipulator->RegisterComponent();

		// Bounds control component
		UUxtBoundsControlComponent* BoundsControl = NewObject<UUxtBoundsControlComponent>(Actor);
		BoundsControl->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return BoundsControl;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	BoundsControlSpec, "UXTools.BoundsControl", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtBoundsControlComponent* Target;
UUxtNearPointerComponent* NearPointer;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(BoundsControlSpec)

void BoundsControlSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		UWorld* World = UxtTestUtils::GetTestWorld();
		FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

		UxtTestUtils::EnableTestHandTracker();

		Target = CreateTestComponent();
		NearPointer = UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), "Near Pointer", TargetLocation + FVector(-15, 0, 0));
	});

	AfterEach([this] {
		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();

		Target->GetOwner()->Destroy();
		Target = nullptr;

		NearPointer->GetOwner()->Destroy();
		NearPointer = nullptr;
	});

	LatentIt("should update affordances when the parent actor is moved", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetGrabbing(true); });

		FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::RightVector * 10); });

		// Skip two frames as it may take up to two for the affordances to update.
		// One frame for grab update + one frame for affordance update.
		FrameQueue.Skip(2);

		FrameQueue.Enqueue([this]() {
			const FTransform ActorTransform = Target->GetOwner()->GetTransform();
			const FBox& Bounds = Target->GetBounds();

			for (const auto& Entry : Target->GetActorAffordanceMap())
			{
				const AActor* AffordanceActor = Entry.Key;
				const FUxtBoundsControlAffordanceInfo* AffordanceInfo = Entry.Value;

				const FTransform ExpectedTransform = AffordanceInfo->GetWorldTransform(Bounds, ActorTransform);
				const FTransform Result = AffordanceActor->GetTransform();

				TestTrue("Affordance has updated", Result.Equals(ExpectedTransform));
			}
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});
}

#endif