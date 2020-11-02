// Copyright (c) 2020 Microsoft Corporation.
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

bool TestQuatEqual(const FString& What, const FQuat& Actual, const FQuat& Expected, float Tolerance = KINDA_SMALL_NUMBER);

UUxtBoundsControlComponent* Target;
UUxtNearPointerComponent* NearPointer;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(BoundsControlSpec)

bool BoundsControlSpec::TestQuatEqual(const FString& What, const FQuat& Actual, const FQuat& Expected, float Tolerance)
{
	return TestEqual(What, Actual.GetAxisX(), Expected.GetAxisX(), Tolerance) &&
		   TestEqual(What, Actual.GetAxisY(), Expected.GetAxisY(), Tolerance) &&
		   TestEqual(What, Actual.GetAxisZ(), Expected.GetAxisZ(), Tolerance);
}

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

			for (const auto& Entry : Target->GetPrimitiveAffordanceMap())
			{
				const UPrimitiveComponent* AffordancePrimitive = Entry.Key;
				const FUxtAffordanceInstance& AffordanceInstance = Entry.Value;

				FVector ExpectedLocation;
				FQuat ExpectedRotation;
				AffordanceInstance.Config.GetWorldLocationAndRotation(Bounds, ActorTransform, ExpectedLocation, ExpectedRotation);

				TestEqual("Affordance location has updated", AffordancePrimitive->GetComponentLocation(), ExpectedLocation);
				TestQuatEqual("Affordance rotation has updated", AffordancePrimitive->GetComponentQuat(), ExpectedRotation);
			}
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});
}

#endif
