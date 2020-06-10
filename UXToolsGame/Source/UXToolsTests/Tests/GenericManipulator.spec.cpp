// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "GenericManipulatorTestComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Tests/AutomationCommon.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Interactions/UxtGenericManipulatorComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	AActor* CreateTestActor(UWorld* World, const FVector& Location)
	{
		AActor* Actor = World->SpawnActor<AActor>();

		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocation(Location);
		Root->RegisterComponent();

		UStaticMeshComponent* Mesh = UxtTestUtils::CreateBoxStaticMesh(Actor, FVector(0.3f));
		Mesh->SetupAttachment(Actor->GetRootComponent());
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionProfileName(TEXT("OverlapAll"));
		Mesh->SetGenerateOverlapEvents(true);
		Mesh->RegisterComponent();

		return Actor;
	}

	UUxtGenericManipulatorComponent* CreateTestComponent(UWorld* World, const FVector& Location, const FComponentReference* TargetComponent = nullptr)
	{
		AActor* Actor = CreateTestActor(World, Location);

		UUxtGenericManipulatorComponent* TestTarget = NewObject<UUxtGenericManipulatorComponent>(Actor);
		if (TargetComponent)
		{
			TestTarget->TargetComponent = *TargetComponent;
		}
		TestTarget->RegisterComponent();

		return TestTarget;
	}

	UGenericManipulatorTestComponent* CreateEventCaptureComponent(UUxtGenericManipulatorComponent* Manipulator)
	{
		UGenericManipulatorTestComponent* EventCaptureComponent = NewObject<UGenericManipulatorTestComponent>(Manipulator->GetOwner());
		EventCaptureComponent->RegisterComponent();

		Manipulator->OnUpdateTransform.AddDynamic(EventCaptureComponent, &UGenericManipulatorTestComponent::UpdateTransform);

		return EventCaptureComponent;
	}
}

BEGIN_DEFINE_SPEC(GenericManipulatorSpec, "UXTools.GenericManipulatorTest", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueCorrectTargetTest();

UWorld* World;
FVector Center;
UUxtNearPointerComponent* Pointer;

UUxtGenericManipulatorComponent* Manipulator;
USceneComponent* TargetComponent;
UGenericManipulatorTestComponent* EventCaptureComponent;

FFrameQueue FrameQueue;

END_DEFINE_SPEC(GenericManipulatorSpec)

void GenericManipulatorSpec::Define()
{
	Describe("GenericManipulator", [this]
		{
			BeforeEach([this]
				{
					// Load the empty test map to run the test in.
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

					UxtTestUtils::EnableTestHandTracker();

					Center = FVector(150, 0, 0);
					Pointer = UxtTestUtils::CreateNearPointer(World, "TestPointer", Center + FVector(-15, 0, 0));
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();

					FrameQueue.Reset();

					if (Manipulator)
					{
						Manipulator->GetOwner()->Destroy();
						Manipulator = nullptr;
					}
					Pointer->GetOwner()->Destroy();
					Pointer = nullptr;
				});

			LatentIt("should default to parent actor if no user defined target is set", [this](const FDoneDelegate& Done)
				{
					Manipulator = CreateTestComponent(World, Center);
					TargetComponent = Manipulator->GetOwner()->GetRootComponent();

					EnqueueCorrectTargetTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should use user defined component on same actor", [this](const FDoneDelegate& Done)
				{
					FComponentReference TargetComponentReference;
					TargetComponentReference.ComponentProperty = "StaticMeshComponent_0";

					Manipulator = CreateTestComponent(World, Center, &TargetComponentReference);
					TargetComponent = Manipulator->GetOwner()->FindComponentByClass<UStaticMeshComponent>();

					EnqueueCorrectTargetTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should use user defined component on different actor", [this](const FDoneDelegate& Done)
				{
					AActor* TargetActor = CreateTestActor(World, Center);
					FComponentReference TargetComponentReference;
					TargetComponentReference.OtherActor = TargetActor;
					TargetComponentReference.ComponentProperty = "StaticMeshComponent_0";

					Manipulator = CreateTestComponent(World, Center, &TargetComponentReference);
					TargetComponent = TargetActor->FindComponentByClass<UStaticMeshComponent>();

					EnqueueCorrectTargetTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });

					TargetActor->Destroy();
				});

			LatentIt("should trigger transform update events", [this](const FDoneDelegate& Done)
				{
					Manipulator = CreateTestComponent(World, Center);
					EventCaptureComponent = CreateEventCaptureComponent(Manipulator);

					FrameQueue.Enqueue([this]()
						{
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Enqueue([this]()
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Center + (FVector::RightVector * 10));
						});
					FrameQueue.Enqueue([this]()
						{
							TestTrue("Transform update event triggered", EventCaptureComponent->TransformUpdateCount > 0);
						});

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

void GenericManipulatorSpec::EnqueueCorrectTargetTest()
{
	FrameQueue.Enqueue([this]()
		{
			UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
		});

	FrameQueue.Enqueue([this]()
		{
			TestTrue("Target grabbed", Manipulator->GetGrabPointers().Num() > 0);
			TestTrue("Correct component targeted", Manipulator->TransformTarget == TargetComponent);
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
