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
		PinchSliderActor->SetInitialValue(0.5f);
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
			PinchSliderActor->SetInitialValue(NewValue);

			TestEqual("Actor has new initial value", PinchSliderActor->GetInitialValue(), NewValue);
			TestEqual("Pinch slider component has new value", PinchSliderComponent->GetValue(), NewValue);
		});

		It("should set track length on slider component", [this] {
			const float NewTrackLength = 100.0f;
			PinchSliderActor->SetTrackLength(NewTrackLength);

			TestEqual("Actor has new track length", PinchSliderActor->GetTrackLength(), NewTrackLength);
			TestEqual("Pinch slider component has new track length", PinchSliderComponent->GetTrackLength(), NewTrackLength);
		});

		LatentIt("should update value on move", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", PinchSliderComponent->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::RightVector * 2.5f);
			});

			FrameQueue.Enqueue([this] { TestEqual("Value has updated", PinchSliderComponent->GetValue(), 0.75f); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
