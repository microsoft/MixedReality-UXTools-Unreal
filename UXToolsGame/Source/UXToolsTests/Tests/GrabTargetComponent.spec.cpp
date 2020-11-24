// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "GrabTargetTestComponent.h"
#include "UxtTestHand.h"
#include "UxtTestUtils.h"

#include "Interactions/UxtGrabTargetComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	UUxtGrabTargetComponent* CreateTestComponent()
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		// Box mesh
		UStaticMeshComponent* Mesh = UxtTestUtils::CreateBoxStaticMesh(Actor);
		Actor->SetRootComponent(Mesh);
		Mesh->RegisterComponent();

		// Grab target component
		UUxtGrabTargetComponent* GrabTarget = NewObject<UUxtGrabTargetComponent>(Actor);
		GrabTarget->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return GrabTarget;
	}

	UGrabTargetTestComponent* AddEventCaptureComponent(UUxtGrabTargetComponent* GrabTargetComponent)
	{
		UGrabTargetTestComponent* EventCaptureComponent = NewObject<UGrabTargetTestComponent>(GrabTargetComponent->GetOwner());

		GrabTargetComponent->OnEnterFarFocus.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnEnterFarFocus);
		GrabTargetComponent->OnUpdateFarFocus.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnUpdateFarFocus);
		GrabTargetComponent->OnExitFarFocus.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnExitFarFocus);

		GrabTargetComponent->OnEnterGrabFocus.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnEnterGrabFocus);
		GrabTargetComponent->OnUpdateGrabFocus.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnUpdateGrabFocus);
		GrabTargetComponent->OnExitGrabFocus.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnExitGrabFocus);

		GrabTargetComponent->OnBeginGrab.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnBeginGrab);
		GrabTargetComponent->OnUpdateGrab.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnUpdateGrab);
		GrabTargetComponent->OnEndGrab.AddDynamic(EventCaptureComponent, &UGrabTargetTestComponent::OnEndGrab);

		return EventCaptureComponent;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	GrabTargetComponentSpec, "UXTools.GrabTargetComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueInteractionTests();

UUxtGrabTargetComponent* Target;
UGrabTargetTestComponent* EventCaptureComponent;
FFrameQueue FrameQueue;

// Must be configured by describe block if needed
EUxtInteractionMode InteractionMode;
FUxtTestHand LeftHand = FUxtTestHand(EControllerHand::Left);
FUxtTestHand RightHand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(GrabTargetComponentSpec)

void GrabTargetComponentSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		UWorld* World = UxtTestUtils::GetTestWorld();
		FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

		UxtTestUtils::EnableTestHandTracker();

		Target = CreateTestComponent();
		EventCaptureComponent = AddEventCaptureComponent(Target);
	});

	AfterEach([this] {
		Target->GetOwner()->Destroy();
		Target = nullptr;
		EventCaptureComponent = nullptr;

		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();
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

		EnqueueInteractionTests();
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

		EnqueueInteractionTests();
	});
}

void GrabTargetComponentSpec::EnqueueInteractionTests()
{
	Describe("Interaction Events", [this] {
		LatentIt("should trigger focus events", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					TestEqual("Entered near focus", EventCaptureComponent->EnterGrabFocusCount, 2);
					break;

				case EUxtInteractionMode::Far:
					TestEqual("Entered far focus", EventCaptureComponent->EnterFarFocusCount, 2);
					break;

				default:
					checkNoEntry();
					break;
				}

				RightHand.Translate(FVector::RightVector);
				LeftHand.Translate(FVector::LeftVector);
			});

			FrameQueue.Enqueue([this] {
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					TestTrue("Updated near focus", EventCaptureComponent->UpdateGrabFocusCount > 1);
					break;

				case EUxtInteractionMode::Far:
					TestTrue("Updated far focus", EventCaptureComponent->UpdateFarFocusCount > 1);
					break;

				default:
					checkNoEntry();
					break;
				}

				RightHand.Translate(FVector::RightVector * 100);
			});

			FrameQueue.Enqueue([this] {
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					TestEqual("Exited near focus", EventCaptureComponent->ExitGrabFocusCount, 1);
					break;

				case EUxtInteractionMode::Far:
					TestEqual("Exited far focus", EventCaptureComponent->ExitFarFocusCount, 1);
					break;

				default:
					checkNoEntry();
					break;
				}
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should trigger grab events", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Begin grab", EventCaptureComponent->BeginGrabCount, 1);

				RightHand.Translate(FVector::RightVector);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Update grab", EventCaptureComponent->UpdateGrabCount, 1);

				RightHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] { TestEqual("End grab", EventCaptureComponent->EndGrabCount, 1); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});

	Describe("Interaction Mode", [this] {
		LatentIt("should not be grabbed when interaction mode is disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Target is grabbed", Target->GetGrabPointers().Num(), 1);

				RightHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				Target->InteractionMode =
					static_cast<int32>(InteractionMode == EUxtInteractionMode::Near ? EUxtInteractionMode::Far : EUxtInteractionMode::Near);

				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] { TestEqual("Target is not grabbed", Target->GetGrabPointers().Num(), 0); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should release when interaction mode is disabled during interaction", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { RightHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Target is grabbed", Target->GetGrabPointers().Num(), 1);

				Target->InteractionMode =
					static_cast<int32>(InteractionMode == EUxtInteractionMode::Near ? EUxtInteractionMode::Far : EUxtInteractionMode::Near);
			});

			FrameQueue.Enqueue([this] { TestEqual("Target is not grabbed", Target->GetGrabPointers().Num(), 0); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});

	Describe("Grab Mode", [this] {
		LatentIt("should be grabbed by one or two hands", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { LeftHand.SetGrabbing(true); });

			FrameQueue.Enqueue([this] {
				TestEqual("Target has first grab pointer", Target->GetGrabPointers().Num(), 1);
				TestEqual("First grab event triggered", EventCaptureComponent->BeginGrabCount, 1);

				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target has second grab pointer", Target->GetGrabPointers().Num(), 2);
				TestEqual("Second grab event triggered", EventCaptureComponent->BeginGrabCount, 2);

				LeftHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target released first grab pointer", Target->GetGrabPointers().Num(), 1);
				TestEqual("First release event triggered", EventCaptureComponent->EndGrabCount, 1);

				RightHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target released second grab pointer", Target->GetGrabPointers().Num(), 0);
				TestEqual("Second release event triggered", EventCaptureComponent->EndGrabCount, 2);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should only be grabbed by one hand", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Target->GrabModes = static_cast<int32>(EUxtGrabMode::OneHanded);

				LeftHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target has first grab pointer", Target->GetGrabPointers().Num(), 1);
				TestEqual("Grab event triggered", EventCaptureComponent->BeginGrabCount, 1);

				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target ignored second grab pointer", Target->GetGrabPointers().Num(), 1);
				TestEqual("No second grab event", EventCaptureComponent->BeginGrabCount, 1);

				LeftHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target released first grab pointer", Target->GetGrabPointers().Num(), 0);
				TestEqual("Release event triggered", EventCaptureComponent->EndGrabCount, 1);

				RightHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target ignored release of second grab pointer", Target->GetGrabPointers().Num(), 0);
				TestEqual("No second release event", EventCaptureComponent->EndGrabCount, 1);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should only be grabbed by two hands", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Target->GrabModes = static_cast<int32>(EUxtGrabMode::TwoHanded);

				LeftHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target has first grab pointer", Target->GetGrabPointers().Num(), 1);
				TestEqual("No grab events", EventCaptureComponent->BeginGrabCount, 0);

				RightHand.SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target has second grab pointer", Target->GetGrabPointers().Num(), 2);
				TestEqual("Grab event triggered", EventCaptureComponent->BeginGrabCount, 1);

				LeftHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target released first grab pointer", Target->GetGrabPointers().Num(), 1);
				TestEqual("Release event triggered", EventCaptureComponent->EndGrabCount, 1);

				RightHand.SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] {
				TestEqual("Target released second grab pointer", Target->GetGrabPointers().Num(), 0);
				TestEqual("No second release event", EventCaptureComponent->EndGrabCount, 1);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
