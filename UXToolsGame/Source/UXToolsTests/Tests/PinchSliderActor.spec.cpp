// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtPinchSliderActor.h"
#include "Controls/UxtPinchSliderComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	AUxtPinchSliderActor* CreateTestActor()
	{
		AUxtPinchSliderActor* PinchSliderActor = UxtTestUtils::GetTestWorld()->SpawnActor<AUxtPinchSliderActor>();
		PinchSliderActor->SetTrackLength(10.0f);
		PinchSliderActor->SetValue(0.5f);
		PinchSliderActor->SetActorLocation(TargetLocation);
		PinchSliderActor->SetActorRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, 180.0f)));

		UUxtPinchSliderComponent* PinchSlider = PinchSliderActor->FindComponentByClass<UUxtPinchSliderComponent>();
		PinchSlider->SetSmoothing(0.0f);

		return PinchSliderActor;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	PinchSliderActorSpec, "UXTools.PinchSliderActor", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

AUxtPinchSliderActor* PinchSliderActor;
UUxtPinchSliderComponent* PinchSliderComponent;

FFrameQueue FrameQueue;
FUxtTestHand Hand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(PinchSliderActorSpec)

void PinchSliderActorSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		UWorld* World = UxtTestUtils::GetTestWorld();
		FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

		PinchSliderActor = CreateTestActor();
		PinchSliderComponent = PinchSliderActor->FindComponentByClass<UUxtPinchSliderComponent>();

		UxtTestUtils::EnableTestHandTracker();
		Hand.Configure(EUxtInteractionMode::Near, TargetLocation);
	});

	AfterEach([this] {
		PinchSliderActor->Destroy();
		PinchSliderActor = nullptr;
		PinchSliderComponent = nullptr;

		Hand.Reset();
		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();
	});

	Describe("Basic functionality", [this] {
		It("should set initial value on slider component", [this] {
			const float NewValue = 1.0f;
			PinchSliderActor->SetValue(NewValue);

			TestEqual("Actor has new value", PinchSliderActor->GetValue(), NewValue);
			TestEqual("Component has new value", PinchSliderComponent->GetValue(), NewValue);
		});

		It("should set track length on slider component", [this] {
			const float NewTrackLength = 100.0f;
			PinchSliderActor->SetTrackLength(NewTrackLength);

			TestEqual("Actor has new track length", PinchSliderActor->GetTrackLength(), NewTrackLength);
			TestEqual("Component has new track length", PinchSliderComponent->GetTrackLength(), NewTrackLength);
		});

		It("should use tick marks as steps when using stepped movement", [this] {
			PinchSliderActor->SetStepWithTickMarks(true);
			TestEqual("Component has correct number of steps", PinchSliderComponent->GetNumSteps(), 5);

			PinchSliderActor->SetNumTickMarks(10);
			TestEqual("Component's number of steps has updated", PinchSliderComponent->GetNumSteps(), 10);

			PinchSliderActor->SetNumTickMarks(1);
			TestEqual("Minimum number of tick marks should be two", PinchSliderActor->GetNumTickMarks(), 2);
			TestEqual("Component's number of steps has updated", PinchSliderComponent->GetNumSteps(), 2);
		});

		LatentIt("should update value on move", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", PinchSliderComponent->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::RightVector * 2.5f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Actor's Value has updated", PinchSliderActor->GetValue(), 0.75f);
				TestEqual("Component's value has updated", PinchSliderComponent->GetValue(), 0.75f);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should use custom min/max values", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				PinchSliderActor->SetMinValue(5.0f);
				PinchSliderActor->SetMaxValue(10.0f);
				PinchSliderActor->SetValue(7.5f);

				TestEqual("Component has correct value", PinchSliderComponent->GetValue(), 0.5f);

				Hand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", PinchSliderComponent->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::LeftVector * 5.0f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Actor is at the min value", PinchSliderActor->GetValue(), PinchSliderActor->GetMinValue());
				TestEqual("Component is at the min value", PinchSliderComponent->GetValue(), 0.0f);

				Hand.Translate(FVector::RightVector * 10.0f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Actor is at the max value", PinchSliderActor->GetValue(), PinchSliderActor->GetMaxValue());
				TestEqual("Component is at the max value", PinchSliderComponent->GetValue(), 1.0f);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
