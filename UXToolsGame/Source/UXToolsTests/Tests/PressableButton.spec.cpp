// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "PressableButtonTestComponent.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtPressableButtonComponent.h"
#include "GameFramework/Actor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Templates/SharedPointer.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	USceneComponent* SetTestVisuals(UUxtPressableButtonComponent* Button, EUxtPushBehavior PushBehavior = EUxtPushBehavior::Translate)
	{
		const FString MeshFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
		const float MeshScale = 0.1f;

		AActor* Actor = Button->GetOwner();
		USceneComponent* VisualsSet = nullptr;

		if (!MeshFilename.IsEmpty())
		{
			USceneComponent* Visuals = nullptr;

			// Create a pivot parent component for the compressible visuals
			if (PushBehavior == EUxtPushBehavior::Compress)
			{
				Visuals = NewObject<USceneComponent>(Actor);
				Visuals->SetupAttachment(Actor->GetRootComponent());
				Visuals->RegisterComponent();
			}

			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
			Mesh->SetupAttachment((Visuals != nullptr) ? Visuals : Button);
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Mesh->SetCollisionProfileName(TEXT("OverlapAll"));
			Mesh->SetGenerateOverlapEvents(true);

			UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(Actor, *MeshFilename);
			Mesh->SetStaticMesh(MeshAsset);
			Mesh->SetRelativeLocation((Visuals != nullptr) ? FVector(-5, 0, 0) : FVector::ZeroVector);
			Mesh->SetRelativeScale3D(
				(Visuals != nullptr) ? FVector(MeshScale * 0.5f, MeshScale, MeshScale) : FVector::OneVector * MeshScale);
			Mesh->RegisterComponent();

			VisualsSet = (Visuals != nullptr) ? Visuals : Mesh;

			Button->SetVisuals(VisualsSet);
		}

		return VisualsSet;
	}

	UUxtPressableButtonComponent* CreateTestComponent(
		UWorld* World, const FVector& Location, EUxtPushBehavior PushBehavior = EUxtPushBehavior::Translate)
	{
		AActor* Actor = World->SpawnActor<AActor>();

		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocation(Location);
		Root->RegisterComponent();

		UUxtPressableButtonComponent* TestTarget = NewObject<UUxtPressableButtonComponent>(Actor);
		TestTarget->SetupAttachment(Root);
		TestTarget->SetPushBehavior(PushBehavior);
		TestTarget->SetWorldRotation(FRotator(0, 180, 0));
		TestTarget->SetWorldLocation(Location);
		TestTarget->RecoverySpeed = BIG_NUMBER;
		TestTarget->SetMaxPushDistance(5);
		SetTestVisuals(TestTarget, PushBehavior);
		TestTarget->RegisterComponent();

		return TestTarget;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	PressableButtonSpec, "UXTools.PressableButtonTest", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueDestroyTest(const TTuple<FVector, FVector> FramePositions);
void EnqueueDisableTest(const TTuple<FVector, FVector> FramePositions);
void EnqueuePressReleaseTest(const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease);
void EnqueueMoveButtonTest(const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease);
void EnqueueTwoButtonsTest(const FVector StartingPos);
void EnqueueAbsoluteDistancesTest(const FVector StartingPos);

UUxtPressableButtonComponent* Button;
UUxtPressableButtonComponent* SecondButton;
UPressableButtonTestComponent* EventCaptureObj;
UUxtNearPointerComponent* Pointer;
FVector Center;
FFrameQueue FrameQueue;

FVector TestPos;

const float MoveBy = 10;

END_DEFINE_SPEC(PressableButtonSpec)

void PressableButtonSpec::Define()
{
	Describe("Button", [this] {
		BeforeEach([this] {
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

		AfterEach([this] {
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
		});

		LatentIt("should raise press and release when pointer moves forward and back", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::BackwardVector * MoveBy));

			EnqueuePressReleaseTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should raise press and release when pointer moves forward and left", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::LeftVector * MoveBy));

			EnqueuePressReleaseTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should raise press and release when pointer moves forward past the poke depth", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::ForwardVector * MoveBy));

			EnqueuePressReleaseTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt(
			"shouldn't raise press or release when pointer moves forward and back outside of the button bounds",
			[this](const FDoneDelegate& Done) {
				TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
					Center + ((FVector::BackwardVector + FVector::LeftVector) * MoveBy), Center + (FVector::LeftVector * MoveBy),
					Center + ((FVector::BackwardVector + FVector::LeftVector) * MoveBy));

				EnqueuePressReleaseTest(Sequence, false, false);
				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});

		LatentIt("shouldn't raise press or release when the button is disabled", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::BackwardVector * MoveBy));

			Button->SetEnabled(false);

			EnqueuePressReleaseTest(Sequence, false, false);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("shouldn't raise release when the button is disabled mid press", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector> Sequence = MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center);

			EnqueueDisableTest(Sequence);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should raise press but not release when the button is destroyed during a press", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector> Sequence = MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center);

			EnqueueDestroyTest(Sequence);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt(
			"should raise press and release when the button moves forward and back over the pointer", [this](const FDoneDelegate& Done) {
				TTuple<FVector, FVector, FVector> Sequence =
					MakeTuple(FVector::ForwardVector * MoveBy, FVector::ZeroVector, FVector::ForwardVector * MoveBy);

				EnqueueMoveButtonTest(Sequence, true, true);
				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});

		LatentIt(
			"should raise press and release when the button moves forward and left over the pointer", [this](const FDoneDelegate& Done) {
				TTuple<FVector, FVector, FVector> Sequence =
					MakeTuple(FVector::ForwardVector * MoveBy, FVector::ZeroVector, FVector::LeftVector * MoveBy);

				EnqueueMoveButtonTest(Sequence, true, true);
				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});

		LatentIt("should raise press and release when the button moves forward over the pointer", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(FVector::ForwardVector * MoveBy, FVector::ZeroVector, FVector::BackwardVector * MoveBy);

			EnqueueMoveButtonTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt(
			"shouldn't raise press or release when the button moves forward and back without intersecting with the pointer",
			[this](const FDoneDelegate& Done) {
				TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
					(FVector::ForwardVector + FVector::LeftVector) * MoveBy, FVector::LeftVector * MoveBy,
					(FVector::ForwardVector + FVector::LeftVector) * MoveBy);

				EnqueueMoveButtonTest(Sequence, false, false);
				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});

		LatentIt(
			"should raise press or release for both buttons when the pointer moves forward and back through both in sequence",
			[this](const FDoneDelegate& Done) {
				SecondButton = CreateTestComponent(UxtTestUtils::GetTestWorld(), Center + FVector(0, 10, 0));

				SecondButton->OnButtonPressed.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementPressed);
				SecondButton->OnButtonReleased.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementReleased);

				FVector StartPos = Center + FVector(-MoveBy, 5, 0);

				EnqueueTwoButtonsTest(StartPos);

				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});

		LatentIt("should be in default state", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				TestTrue("button state is default", Button->GetState() == EUxtButtonState::Default);
				Done.Execute();
			});
		});

		LatentIt("should be in disabled state", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this, Done] {
				Button->SetEnabled(false);
				TestTrue("button state is disabled", Button->GetState() == EUxtButtonState::Disabled);
				Done.Execute();
			});
		});

		LatentIt("should be in focused state", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue(
				[this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Center + (FVector::BackwardVector * MoveBy)); });
			FrameQueue.Enqueue([this, Done] {
				TestTrue("button state is focused", Button->GetState() == EUxtButtonState::Focused);
				Done.Execute();
			});
		});

		LatentIt("should be in contacted state", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue(
				[this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Center + (FVector::BackwardVector * MoveBy)); });
			FrameQueue.Enqueue([this] {
				const float MoveAmount = Button->GetMaxPushDistance() * (Button->PressedFraction * 2.0f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Center + (FVector::BackwardVector * MoveAmount));
			});
			FrameQueue.Enqueue([this, Done] {
				TestTrue("button state is contacted", Button->GetState() == EUxtButtonState::Contacted);
				Done.Execute();
			});
		});

		LatentIt("should be in pressed state", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue(
				[this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Center + (FVector::BackwardVector * MoveBy)); });

			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Center); });
			FrameQueue.Enqueue([this, Done] {
				TestTrue("button state is pressed", Button->GetState() == EUxtButtonState::Pressed);
				Done.Execute();
			});
		});

		LatentIt(
			"should move the button to the same location when using either local or world scale for distances",
			[this](const FDoneDelegate& Done) {
				Button->SetWorldScale3D(FVector(0.5f, 0.5f, 0.5f));

				FVector StartPos = Center + FVector(-MoveBy, 5, 0);

				EnqueueAbsoluteDistancesTest(StartPos);

				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});

		LatentIt("should update visuals after component is created", [this](const FDoneDelegate& Done) {
			USceneComponent* NewVisuals = SetTestVisuals(Button);
			TestEqual("Has new visuals", Button->GetVisuals(), NewVisuals);

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});

	// Perform a subset of the above tests with the button visuals set to compress rather than translate
	Describe("Button (Compress Push Behavior)", [this] {
		BeforeEach([this] {
			// Load the empty test map to run the test in.
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			Center = FVector(50, 0, 0);
			Button = CreateTestComponent(World, Center, EUxtPushBehavior::Compress);

			EventCaptureObj = NewObject<UPressableButtonTestComponent>(Button->GetOwner());
			EventCaptureObj->RegisterComponent();
			Button->OnButtonPressed.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementPressed);
			Button->OnButtonReleased.AddDynamic(EventCaptureObj, &UPressableButtonTestComponent::IncrementReleased);

			UxtTestUtils::EnableTestHandTracker();
			Pointer = UxtTestUtils::CreateNearPointer(World, "TestPointer", FVector::ZeroVector);
			Pointer->PokeDepth = 5;
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();

			Button->GetOwner()->Destroy();
			Button = nullptr;
			Pointer->GetOwner()->Destroy();
			Pointer = nullptr;
		});

		LatentIt("should raise press and release when pointer moves forward and back", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::BackwardVector * MoveBy));

			EnqueuePressReleaseTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should raise press and release when pointer moves forward and left", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::LeftVector * MoveBy));

			EnqueuePressReleaseTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should raise press and release when pointer moves forward past the poke depth", [this](const FDoneDelegate& Done) {
			TTuple<FVector, FVector, FVector> Sequence =
				MakeTuple(Center + (FVector::BackwardVector * MoveBy), Center, Center + (FVector::ForwardVector * MoveBy));

			EnqueuePressReleaseTest(Sequence, true, true);
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt(
			"shouldn't raise press or release when pointer moves forward and back outside of the button bounds",
			[this](const FDoneDelegate& Done) {
				TTuple<FVector, FVector, FVector> Sequence = MakeTuple(
					Center + ((FVector::BackwardVector + FVector::LeftVector) * MoveBy), Center + (FVector::LeftVector * MoveBy),
					Center + ((FVector::BackwardVector + FVector::LeftVector) * MoveBy));

				EnqueuePressReleaseTest(Sequence, false, false);
				FrameQueue.Enqueue([Done] { Done.Execute(); });
			});
	});
}

void PressableButtonSpec::EnqueueDestroyTest(const TTuple<FVector, FVector> FramePositions)
{
	FrameQueue.Enqueue([this, FramePositions] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<0>()); });
	FrameQueue.Enqueue([this, FramePositions] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<1>()); });
	FrameQueue.Enqueue([this] {
		TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
		TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);

		Button->DestroyComponent();
	});
	FrameQueue.Enqueue([this] {
		TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
		TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);
	});
}

void PressableButtonSpec::EnqueueDisableTest(const TTuple<FVector, FVector> FramePositions)
{
	FrameQueue.Enqueue([this, FramePositions] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<0>()); });
	FrameQueue.Enqueue([this, FramePositions] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<1>()); });
	FrameQueue.Enqueue([this] {
		TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
		TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);

		Button->SetEnabled(false);
	});
	// verify no released
	FrameQueue.Enqueue([this] {
		TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);
		TestTrue("Button was not released", EventCaptureObj->ReleasedCount == 0);
	});
}

void PressableButtonSpec::EnqueuePressReleaseTest(
	const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease)
{
	FrameQueue.Enqueue([this, FramePositions] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<0>()); });
	FrameQueue.Enqueue([this, FramePositions] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<1>()); });
	FrameQueue.Enqueue([this, bExpectingPress, FramePositions] {
		TestEqual("Button press as expected", EventCaptureObj->PressedCount == 1, bExpectingPress);

		UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FramePositions.Get<2>());
	});
	FrameQueue.Enqueue(
		[this, bExpectingRelease] { TestEqual("Button release as expected", EventCaptureObj->ReleasedCount == 1, bExpectingRelease); });
}

void PressableButtonSpec::EnqueueMoveButtonTest(
	const TTuple<FVector, FVector, FVector> FramePositions, bool bExpectingPress, bool bExpectingRelease)
{
	FrameQueue.Enqueue([this, FramePositions] { Button->SetWorldLocation(FramePositions.Get<0>()); });
	FrameQueue.Enqueue([this, FramePositions] { Button->SetWorldLocation(FramePositions.Get<1>()); });
	FrameQueue.Enqueue([this, bExpectingPress, FramePositions] {
		TestEqual("Button press as expected", EventCaptureObj->PressedCount == 1, bExpectingPress);

		Button->SetWorldLocation(FramePositions.Get<2>());
	});
	FrameQueue.Enqueue(
		[this, bExpectingRelease] { TestEqual("Button release as expected", EventCaptureObj->ReleasedCount == 1, bExpectingRelease); });
}

void PressableButtonSpec::EnqueueTwoButtonsTest(const FVector StartingPos)
{
	// This test is to ensure that the buttons still function when there is more than one
	// pokeable object in the poke focus volume. This was causing issues as if the focused
	// and poked primitives differ, then no poke events are fired

	FrameQueue.Enqueue([this, StartingPos] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(StartingPos); });
	FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Button->GetComponentLocation()); });
	FrameQueue.Enqueue([this, StartingPos] {
		TestTrue("A button was pressed", EventCaptureObj->PressedCount == 1);
		TestTrue("No button was released", EventCaptureObj->ReleasedCount == 0);
		TestTrue("First Button is pressed", Button->GetState() == EUxtButtonState::Pressed);
		TestFalse("Second Button is not pressed", SecondButton->GetState() == EUxtButtonState::Pressed);
	});
	FrameQueue.Enqueue([this, StartingPos] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(StartingPos); });
	FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(SecondButton->GetComponentLocation()); });
	FrameQueue.Enqueue([this] {
		TestTrue("Another button was pressed", EventCaptureObj->PressedCount == 2);
		TestTrue("A button was released", EventCaptureObj->ReleasedCount == 1);
		TestFalse("First Button is not pressed", Button->GetState() == EUxtButtonState::Pressed);
		TestTrue("Second Button is pressed", SecondButton->GetState() == EUxtButtonState::Pressed);
	});
}

void PressableButtonSpec::EnqueueAbsoluteDistancesTest(const FVector StartingPos)
{
	FrameQueue.Enqueue([this, StartingPos] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(StartingPos); });
	FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Button->GetComponentLocation()); });
	FrameQueue.Enqueue([this, StartingPos] {
		TestTrue("Button was pressed", EventCaptureObj->PressedCount == 1);

		TestPos = Button->GetVisuals()->GetComponentLocation();
	});
	FrameQueue.Enqueue([this, StartingPos] {
		UxtTestUtils::GetTestHandTracker().SetAllJointPositions(StartingPos);
		Button->SetUseAbsolutePushDistance(true);
	});
	FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Button->GetComponentLocation()); });
	FrameQueue.Enqueue([this] {
		TestTrue("Button was pressed after changing to absolute distances", EventCaptureObj->PressedCount == 2);

		TestEqual(
			"Button pressed to same position for local and world space distances", Button->GetVisuals()->GetComponentLocation(), TestPos);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
