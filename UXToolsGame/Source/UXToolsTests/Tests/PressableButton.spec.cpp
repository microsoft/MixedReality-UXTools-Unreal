// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"
#include "UxtTestHandTracker.h"

#include "Controls/UxtPressableButtonComponent.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UxtTestUtils.h"
#include "Input/UxtNearPointerComponent.h"
#include "PressableButtonTestComponent.h"
#include "FrameQueue.h"

#include "Templates/SharedPointer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtPressableButtonComponent* CreateTestComponent(UWorld* World, const FVector& Location)
	{
		AActor* actor = World->SpawnActor<AActor>();

		USceneComponent* root = NewObject<USceneComponent>(actor);
		actor->SetRootComponent(root);
		root->SetWorldLocation(Location);
		root->RegisterComponent();

		UUxtPressableButtonComponent* TestTarget = NewObject<UUxtPressableButtonComponent>(actor);
		TestTarget->SetWorldLocation(Location);
		TestTarget->RegisterComponent();

		FString meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
		float meshScale = 0.1f;
		if (!meshFilename.IsEmpty())
		{
			UStaticMeshComponent* mesh = NewObject<UStaticMeshComponent>(actor);
			mesh->SetupAttachment(actor->GetRootComponent());
			mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			mesh->SetCollisionProfileName(TEXT("OverlapAll"));
			mesh->SetGenerateOverlapEvents(true);

			UStaticMesh* meshAsset = LoadObject<UStaticMesh>(actor, *meshFilename);
			mesh->SetStaticMesh(meshAsset);
			mesh->SetRelativeScale3D(FVector::OneVector * meshScale);

			mesh->RegisterComponent();

			TestTarget->SetVisuals(mesh);
		}

		TestTarget->RecoverySpeed = BIG_NUMBER;
		TestTarget->MaxPushDistance = 5;

		return TestTarget;
	}
}

BEGIN_DEFINE_SPEC(PressableButtonSpec, "UXTools.PressableButtonTest", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	void EnqueueDestroyTest(const TTuple<FVector, FVector> FramePositions);
	void EnqueueDisableTest(const TTuple<FVector, FVector> FramePositions);
	void EnqueuePressReleaseTest(const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease);
	void EnqueueMoveButtonTest(const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease);
	void EnqueueTwoButtonsTest(const FVector StartingPos);

	UUxtPressableButtonComponent* Button;
	UUxtPressableButtonComponent* SecondButton;
	UPressableButtonTestComponent* EventCaptureObj;
	UUxtNearPointerComponent* Pointer;
	FVector Center;
	FFrameQueue FrameQueue;

	const float MoveBy = 10;

END_DEFINE_SPEC(PressableButtonSpec)

void PressableButtonSpec::Define()
{
	Describe("Button", [this]
		{
			BeforeEach([this]
				{
					// Load the empty test map to run the test in.
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

					Center = FVector(50, 0, 0);
					Button = CreateTestComponent(World, Center);

					EventCaptureObj = NewObject<UPressableButtonTestComponent>(Button->GetOwner());
					EventCaptureObj->RegisterComponent();
					Button->OnButtonPressed.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementPressed);
					Button->OnButtonReleased.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementReleased);

					UxtTestUtils::EnableTestHandTracker();
					Pointer = UxtTestUtils::CreateNearPointer(World, "TestPointer", FVector::ZeroVector);
					Pointer->PokeDepth = 5;
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();

					FrameQueue.Reset();

					Button->GetOwner()->Destroy();
					Button = nullptr;
					Pointer->GetOwner()->Destroy();
					Pointer = nullptr;

					if (SecondButton)
					{
						SecondButton->GetOwner()->Destroy();
						SecondButton = nullptr;
					}

					// Force GC so that destroyed actors are removed from the world.
					// Running multiple tests will otherwise cause errors when creating duplicate actors.
					GEngine->ForceGarbageCollection();
				});

			LatentIt("should raise press and release when pointer moves forward and back", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						Center + (FVector::BackwardVector * MoveBy),
						Center,
						Center + (FVector::BackwardVector * MoveBy));

					EnqueuePressReleaseTest(Sequence, true, true);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press and release when pointer moves forward and left", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						Center + (FVector::BackwardVector * MoveBy),
						Center,
						Center + (FVector::LeftVector * MoveBy));

					EnqueuePressReleaseTest(Sequence, true, true);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press and release when pointer moves forward past the poke depth", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						Center + (FVector::BackwardVector * MoveBy),
						Center,
						Center + (FVector::ForwardVector * MoveBy));

					EnqueuePressReleaseTest(Sequence, true, true);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("shouldn't raise press or release when pointer moves forward and back outside of the button bounds", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						Center + ((FVector::BackwardVector + FVector::LeftVector) * MoveBy),
						Center + (FVector::LeftVector * MoveBy),
						Center + ((FVector::BackwardVector + FVector::LeftVector) * MoveBy));

					EnqueuePressReleaseTest(Sequence, false, false);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("shouldn't raise press or release when the button is disabled", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						Center + (FVector::BackwardVector * MoveBy),
						Center,
						Center + (FVector::BackwardVector * MoveBy));

					Button->SetEnabled(false);

					EnqueuePressReleaseTest(Sequence, false, false);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("shouldn't raise release when the button is disabled mid press", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector> Sequence = MakeTuple(
						Center + (FVector::BackwardVector * MoveBy),
						Center);

					EnqueueDisableTest(Sequence);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press but not release when the button is destroyed during a press", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector> Sequence = MakeTuple(
						Center + (FVector::BackwardVector * MoveBy),
						Center);

					EnqueueDestroyTest(Sequence);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press and release when the button moves forward and back over the pointer", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						FVector::ForwardVector * MoveBy,
						FVector::ZeroVector,
						FVector::ForwardVector * MoveBy);

					EnqueueMoveButtonTest(Sequence, true, true);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press and release when the button moves forward and left over the pointer", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						FVector::ForwardVector * MoveBy,
						FVector::ZeroVector,
						FVector::LeftVector * MoveBy);

					EnqueueMoveButtonTest(Sequence, true, true);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press and release when the button moves forward over the pointer", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						FVector::ForwardVector * MoveBy,
						FVector::ZeroVector,
						FVector::BackwardVector * MoveBy);

					EnqueueMoveButtonTest(Sequence, true, true);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("shouldn't raise press or release when the button moves forward and back without intersecting with the pointer", [this](const FDoneDelegate& Done)
				{
					TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
						(FVector::ForwardVector + FVector::LeftVector) * MoveBy,
						FVector::LeftVector * MoveBy,
						(FVector::ForwardVector + FVector::LeftVector) * MoveBy);

					EnqueueMoveButtonTest(Sequence, false, false);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt("should raise press or release for both buttons when the pointer moves forward and back through both in sequence", [this](const FDoneDelegate& Done)
				{
					SecondButton = CreateTestComponent(UxtTestUtils::GetTestWorld(), Center + FVector(0, 10, 0));

					SecondButton->OnButtonPressed.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementPressed);
					SecondButton->OnButtonReleased.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementReleased);

					FVector StartPos = Center + FVector(-MoveBy, 5, 0);

					EnqueueTwoButtonsTest(StartPos);

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

void PressableButtonSpec::EnqueueDestroyTest(const TTuple<FVector, FVector> FramePositions)
{
	// The occasional frame needs to be skipped in this test because of a tick 
	// ordering issue. The problem is that UUxtNearPointerComponent::TickComponent 
	// calls after UUxtPressableButtonComponent::TickComponent so PokePointers will 
	// be empty for the first frame after the hand has been moved to the press
	// position. Waiting a frame allows for UUxtPressableButtonComponent::TickComponent 
	// to be called after PokePointers has been populated.

	// first move
	FrameQueue.Enqueue([this, FramePositions]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<0>();
		});
	// second move
	FrameQueue.Enqueue([this, FramePositions]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<1>();
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test pressed and destroy
	FrameQueue.Enqueue([this]
		{
			TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
			TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);

			Button->DestroyComponent();
		});
	// verify no released
	FrameQueue.Enqueue([this]
		{
			TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
			TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);
		});
}

void PressableButtonSpec::EnqueueDisableTest(const TTuple<FVector, FVector> FramePositions)
{
	// The occasional frame needs to be skipped in this test because of a tick 
	// ordering issue. The problem is that UUxtNearPointerComponent::TickComponent 
	// calls after UUxtPressableButtonComponent::TickComponent so PokePointers will 
	// be empty for the first frame after the hand has been moved to the press
	// position. Waiting a frame allows for UUxtPressableButtonComponent::TickComponent 
	// to be called after PokePointers has been populated.

	// first move
	FrameQueue.Enqueue([this, FramePositions]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<0>();
		});
	// second move
	FrameQueue.Enqueue([this, FramePositions]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<1>();
		});

	// skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test pressed and disable button
	FrameQueue.Enqueue([this]
		{
			TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
			TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);

			Button->SetEnabled(false);
		});
	// verify no released
	FrameQueue.Enqueue([this]
		{
			TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
			TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);
		});
}

void PressableButtonSpec::EnqueuePressReleaseTest(const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease)
{
	// The occasional frame needs to be skipped in this test because of a tick 
	// ordering issue. The problem is that UUxtNearPointerComponent::TickComponent 
	// calls after UUxtPressableButtonComponent::TickComponent so PokePointers will 
	// be empty for the first frame after the hand has been moved to the press
	// position. Waiting a frame allows for UUxtPressableButtonComponent::TickComponent 
	// to be called after PokePointers has been populated.

	// move to starting position
	FrameQueue.Enqueue([this, FramePositions]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<0>();
		});
	// move to second position
	FrameQueue.Enqueue([this, FramePositions]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<1>();
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test pressed state is expected and move to final position
	FrameQueue.Enqueue([this, bExpectingPress, FramePositions]
		{
			TestEqual("Button press as expected", EventCaptureObj->PressedCount == 1, bExpectingPress);

			UxtTestUtils::GetTestHandTracker().TestPosition = FramePositions.Get<2>();
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test released state is expected
	FrameQueue.Enqueue([this, bExpectingRelease]
		{
			TestEqual("Button release as expected", EventCaptureObj->ReleasedCount == 1, bExpectingRelease);
		});
}

void PressableButtonSpec::EnqueueMoveButtonTest(const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease)
{
	// The occasional frame needs to be skipped in this test because of a tick 
	// ordering issue. The problem is that UUxtNearPointerComponent::TickComponent 
	// calls after UUxtPressableButtonComponent::TickComponent so PokePointers will 
	// be empty for the first frame after the hand has been moved to the press
	// position. Waiting a frame allows for UUxtPressableButtonComponent::TickComponent 
	// to be called after PokePointers has been populated.

	// move to starting position
	FrameQueue.Enqueue([this, FramePositions]
		{
			Button->SetWorldLocation(FramePositions.Get<0>());
		});
	// move to second position
	FrameQueue.Enqueue([this, FramePositions]
		{
			Button->SetWorldLocation(FramePositions.Get<1>());
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test pressed state is expected and move to final position
	FrameQueue.Enqueue([this, bExpectingPress, FramePositions]
		{
			TestEqual("Button press as expected", EventCaptureObj->PressedCount == 1, bExpectingPress);

			Button->SetWorldLocation(FramePositions.Get<2>());
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test released state is expected
	FrameQueue.Enqueue([this, bExpectingRelease]
		{
			TestEqual("Button release as expected", EventCaptureObj->ReleasedCount == 1, bExpectingRelease);
		});
}

void PressableButtonSpec::EnqueueTwoButtonsTest(const FVector StartingPos)
{
	// This test is to ensure that the buttons still function when there is more than one
	// pokeable object in the poke focus volume. This was causing issues as if the focused
	// and poked primitives differ, then no poke events are fired

	// The occasional frame needs to be skipped in this test because of a tick 
	// ordering issue. The problem is that UUxtNearPointerComponent::TickComponent 
	// calls after UUxtPressableButtonComponent::TickComponent so PokePointers will 
	// be empty for the first frame after the hand has been moved to the press
	// position. Waiting a frame allows for UUxtPressableButtonComponent::TickComponent 
	// to be called after PokePointers has been populated.

	// first move
	FrameQueue.Enqueue([this, StartingPos]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = StartingPos;
		});
	// second move
	FrameQueue.Enqueue([this]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = Button->GetComponentLocation();
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// test pressed and move back to starting pos
	FrameQueue.Enqueue([this, StartingPos]
		{
			TestTrue("A button was pressed", EventCaptureObj->PressedCount == 1);
			TestTrue("No button was released", EventCaptureObj->ReleasedCount == 0);
			TestTrue("First Button is pressed", Button->IsPressed());
			TestFalse("Second Button is not pressed", SecondButton->IsPressed());
		});
	// third move
	FrameQueue.Enqueue([this, StartingPos]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = StartingPos;
		});
	// fourth move
	FrameQueue.Enqueue([this]
		{
			UxtTestUtils::GetTestHandTracker().TestPosition = SecondButton->GetComponentLocation();
		});

	// Skip a frame for poke because of tick ordering issue.
	FrameQueue.Skip();

	// second move
	FrameQueue.Enqueue([this]
		{
			TestTrue("Another button was pressed", EventCaptureObj->PressedCount == 2);
			TestTrue("A button was released", EventCaptureObj->ReleasedCount == 1);
			TestFalse("First Button is not pressed", Button->IsPressed());
			TestTrue("Second Button is pressed", SecondButton->IsPressed());
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS

