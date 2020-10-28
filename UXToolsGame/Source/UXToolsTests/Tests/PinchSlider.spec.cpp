// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "PinchSliderTestComponent.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtPinchSliderComponent.h"
#include "Input/UxtPointerComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	UUxtPinchSliderComponent* CreateTestComponent()
	{
		AActor* Actor = UxtTestUtils::GetTestWorld()->SpawnActor<AActor>();

		UStaticMeshComponent* Thumb = UxtTestUtils::CreateBoxStaticMesh(Actor);
		Thumb->RegisterComponent();

		UUxtPinchSliderComponent* PinchSlider = NewObject<UUxtPinchSliderComponent>(Actor);
		PinchSlider->SetVisuals(Thumb);
		PinchSlider->SetTrackLength(10.0f);
		PinchSlider->SetValue(0.5f);
		PinchSlider->SetSmoothing(0.0f);
		PinchSlider->RegisterComponent();

		Actor->SetRootComponent(PinchSlider);
		Actor->SetActorLocation(TargetLocation);
		Actor->SetActorRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, 180.0f)));

		return PinchSlider;
	}

	UPinchSliderTestComponent* CreateEventCaptureComponent(UUxtPinchSliderComponent* Slider)
	{
		UPinchSliderTestComponent* EventCaptureComponent = NewObject<UPinchSliderTestComponent>(Slider->GetOwner());

		Slider->OnUpdateState.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnUpdateState);
		Slider->OnBeginFocus.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnBeginFocus);
		Slider->OnUpdateFocus.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnUpdateFocus);
		Slider->OnEndFocus.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnEndFocus);
		Slider->OnBeginGrab.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnBeginGrab);
		Slider->OnUpdateValue.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnUpdateValue);
		Slider->OnEndGrab.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnEndGrab);
		Slider->OnEnable.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnEnable);
		Slider->OnDisable.AddDynamic(EventCaptureComponent, &UPinchSliderTestComponent::OnDisable);

		EventCaptureComponent->RegisterComponent();

		return EventCaptureComponent;
	}
} // namespace

BEGIN_DEFINE_SPEC(PinchSliderSpec, "UXTools.PinchSlider", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtPinchSliderComponent* Target;
UPinchSliderTestComponent* EventCaptureComponent;

FFrameQueue FrameQueue;
FUxtTestHand Hand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(PinchSliderSpec)

void PinchSliderSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		UWorld* World = UxtTestUtils::GetTestWorld();
		FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

		Target = CreateTestComponent();
		EventCaptureComponent = CreateEventCaptureComponent(Target);

		UxtTestUtils::EnableTestHandTracker();
		Hand.Configure(EUxtInteractionMode::Near, TargetLocation);
	});

	AfterEach([this] {
		Target->GetOwner()->Destroy();
		Target = nullptr;
		EventCaptureComponent = nullptr;

		Hand.Reset();
		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();
	});

	Describe("Basic functionality", [this] {
		LatentIt("should update value on move", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::RightVector * 2.5f);
			});

			FrameQueue.Enqueue([this] { TestEqual("Value has updated", Target->GetValue(), 0.75f); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should move in steps", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Target->SetUseSteppedMovement(true);
				Hand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::RightVector * 2.0f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider has moved to the first step", Target->GetValue(), 0.75f);

				Hand.Translate(FVector::RightVector * 1.0f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is still at the first step", Target->GetValue(), 0.75f);

				Hand.Translate(FVector::RightVector * 1.0f);
			});

			FrameQueue.Enqueue([this] { TestEqual("Slider has moved to the second step", Target->GetValue(), 1.0f); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should limit value within the set bounds", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Target->SetValueLowerBound(0.2f);
				Target->SetValueUpperBound(0.8f);
				Hand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::LeftVector * 5.0f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Value has been limited by lower bound", Target->GetValue(), 0.2f);

				Hand.Translate(FVector::RightVector * 10.0f);
			});

			FrameQueue.Enqueue([this] { TestEqual("Value has been limited by upper bound", Target->GetValue(), 0.8f); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should not be grabbable when disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Target->SetEnabled(false); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is disabled", Target->GetState(), EUxtSliderState::Disabled);

				Hand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] { TestNotEqual("Slider is not grabbed", Target->GetState(), EUxtSliderState::Grabbed); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should release locked pointers on disable", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);
				TestTrue("Pointer is locked", Hand.GetPointer()->GetFocusLocked());

				Target->SetEnabled(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is disabled", Target->GetState(), EUxtSliderState::Disabled);
				TestFalse("Pointer is not locked", Hand.GetPointer()->GetFocusLocked());
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});

	Describe("State transitions", [this] {
		LatentIt("should move to default state when released without focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);

				Hand.Translate(FVector::RightVector * 100.0f);
			});

			FrameQueue.Enqueue([this] { Hand.SetGrabbing(false); });

			// We need to skip a frame here as the slider will receive an end grab event before an end focus event.
			// This is due to the grabbing pointer being focus-locked to the slider until the grab is released.
			// e.g.
			// | Frame 1              | Frame 2    | Frame 3     |
			// | Release -> EndGrab() | EndFocus() | TestEqual() |
			FrameQueue.Skip();

			FrameQueue.Enqueue([this] { TestEqual("Slider is in default state", Target->GetState(), EUxtSliderState::Default); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should move to focused state when released with focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);

				Hand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] { TestEqual("Slider is focused", Target->GetState(), EUxtSliderState::Focused); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should move to default state when enabled without focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Target->SetEnabled(false);
				Hand.Translate(FVector::RightVector * 100.0f);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is disabled", Target->GetState(), EUxtSliderState::Disabled);

				Target->SetEnabled(true);
			});

			FrameQueue.Enqueue([this] { TestEqual("Slider is in default state", Target->GetState(), EUxtSliderState::Default); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should move to focused state when enabled with focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Target->SetEnabled(false); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is disabled", Target->GetState(), EUxtSliderState::Disabled);

				Target->SetEnabled(true);
			});

			FrameQueue.Enqueue([this] { TestEqual("Slider is focused", Target->GetState(), EUxtSliderState::Focused); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});

	Describe("Interaction events", [this] {
		LatentIt("should trigger state update events", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.Translate(FVector::RightVector * 100); });

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is in default state", Target->GetState(), EUxtSliderState::Default);
				TestTrue("OnUpdateState event received", EventCaptureComponent->OnUpdateStateReceived);

				EventCaptureComponent->Reset();
				Hand.Translate(FVector::LeftVector * 100);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is in focused state", Target->GetState(), EUxtSliderState::Focused);
				TestTrue("OnUpdateState event received", EventCaptureComponent->OnUpdateStateReceived);

				EventCaptureComponent->Reset();
				Hand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is in grabbed state", Target->GetState(), EUxtSliderState::Grabbed);
				TestTrue("OnUpdateState event received", EventCaptureComponent->OnUpdateStateReceived);

				EventCaptureComponent->Reset();
				Target->SetEnabled(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is in disabled state", Target->GetState(), EUxtSliderState::Disabled);
				TestTrue("OnUpdateState event received", EventCaptureComponent->OnUpdateStateReceived);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should trigger focus events", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { Hand.Translate(FVector::RightVector * 100); });

			FrameQueue.Enqueue([this] {
				TestNotEqual("Slider is not focused", Target->GetState(), EUxtSliderState::Focused);
				EventCaptureComponent->Reset();

				Hand.Translate(FVector::LeftVector * 100);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is focused", Target->GetState(), EUxtSliderState::Focused);
				TestTrue("OnBeginFocus event received", EventCaptureComponent->OnBeginFocusReceived);

				Hand.Translate(FVector::RightVector * 100);
			});

			FrameQueue.Enqueue([this] {
				TestNotEqual("Slider is not focused", Target->GetState(), EUxtSliderState::Focused);
				TestTrue("OnUpdateFocus event received", EventCaptureComponent->OnUpdateFocusReceived);
				TestTrue("OnEndFocus event received", EventCaptureComponent->OnEndFocusReceived);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should trigger grab events", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				TestNotEqual("Slider is not grabbed", Target->GetState(), EUxtSliderState::Grabbed);

				Hand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is grabbed", Target->GetState(), EUxtSliderState::Grabbed);
				TestTrue("OnBeginGrab event received", EventCaptureComponent->OnBeginGrabReceived);

				Hand.Translate(FVector::RightVector * 10);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("OnUpdateValue event received", EventCaptureComponent->OnUpdateValueReceived);

				Hand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestNotEqual("Slider is not grabbed", Target->GetState(), EUxtSliderState::Grabbed);
				TestTrue("OnEndGrab event received", EventCaptureComponent->OnEndGrabReceived);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should trigger enable / disable events", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				TestNotEqual("Slider is not disabled", Target->GetState(), EUxtSliderState::Disabled);

				Target->SetEnabled(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Slider is disabled", Target->GetState(), EUxtSliderState::Disabled);
				TestTrue("OnDisable event received", EventCaptureComponent->OnDisableReceived);

				Target->SetEnabled(true);
			});

			FrameQueue.Enqueue([this] {
				TestNotEqual("Slider is not disabled", Target->GetState(), EUxtSliderState::Disabled);
				TestTrue("OnEnable event received", EventCaptureComponent->OnEnableReceived);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
