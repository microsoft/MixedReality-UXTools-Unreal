// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "FrameQueue.h"
#include "TouchableVolumeTestComponent.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtTouchableVolumeComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);

	UUxtTouchableVolumeComponent* CreateTestComponent()
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		// Box Mesh
		UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Actor);
		Actor->SetRootComponent(Mesh);
		Mesh->RegisterComponent();

		// Generic manipulator component
		UUxtTouchableVolumeComponent* Volume = NewObject<UUxtTouchableVolumeComponent>(Actor);
		Volume->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return Volume;
	}

	UTouchableVolumeTestComponent* AddEventCaptureComponent(UUxtTouchableVolumeComponent* Target)
	{
		// Generic manipulator component
		UTouchableVolumeTestComponent* EventCaptureComponent = NewObject<UTouchableVolumeTestComponent>(Target->GetOwner());
		Target->OnBeginFocus.AddDynamic(EventCaptureComponent, &UTouchableVolumeTestComponent::OnBeginFocus);
		Target->OnEndFocus.AddDynamic(EventCaptureComponent, &UTouchableVolumeTestComponent::OnEndFocus);
		Target->OnBeginPoke.AddDynamic(EventCaptureComponent, &UTouchableVolumeTestComponent::OnBeginPoke);
		Target->OnEndPoke.AddDynamic(EventCaptureComponent, &UTouchableVolumeTestComponent::OnEndPoke);
		Target->OnVolumeDisabled.AddDynamic(EventCaptureComponent, &UTouchableVolumeTestComponent::OnDisable);
		Target->OnVolumeEnabled.AddDynamic(EventCaptureComponent, &UTouchableVolumeTestComponent::OnEnable);
		EventCaptureComponent->RegisterComponent();

		return EventCaptureComponent;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	TouchableVolumeSpec, "UXTools.TouchableVolume", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtTouchableVolumeComponent* Target;
UTouchableVolumeTestComponent* EventCaptureComponent;
FFrameQueue FrameQueue;

// Must be configured by Describe block if needed
FUxtTestHand Hand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(TouchableVolumeSpec)

void TouchableVolumeSpec::Define()
{
	BeforeEach(
		[this]
		{
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			UxtTestUtils::EnableTestInputSystem();

			Target = CreateTestComponent();
			EventCaptureComponent = AddEventCaptureComponent(Target);
		});

	AfterEach(
		[this]
		{
			Target->GetOwner()->Destroy();
			Target = nullptr;

			UxtTestUtils::DisableTestInputSystem();

			FrameQueue.Reset();
		});

	Describe(
		"Near Interaction",
		[this]
		{
			BeforeEach([this] { Hand.Configure(EUxtInteractionMode::Near, TargetLocation + FVector(-100, 0, 0)); });

			AfterEach([this] { Hand.Reset(); });

			LatentIt(
				"should trigger focus events",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this] { Hand.Translate(FVector(50, 0, 0)); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin focus triggered", EventCaptureComponent->BeginFocusCount, 1);

							Hand.Translate(FVector(-50, 0, 0));
						});

					FrameQueue.Enqueue([this] { TestEqual("End focus triggered", EventCaptureComponent->EndFocusCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should trigger poke events",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this] { Hand.Translate(FVector(100, 0, 0)); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke triggered", EventCaptureComponent->BeginPokeCount, 1);

							Hand.Translate(FVector(-100, 0, 0));
						});

					FrameQueue.Enqueue([this] { TestEqual("End poke triggered", EventCaptureComponent->EndPokeCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should not trigger events when disabled",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue(
						[this]
						{
							Target->SetEnabled(false);
							Hand.Translate(FVector(100, 0, 0));
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Disable event triggered", EventCaptureComponent->DisableCount, 1);
							TestEqual("Begin poke not triggered", EventCaptureComponent->BeginPokeCount, 0);

							Hand.Translate(FVector(-100, 0, 0));
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("End poke not triggered", EventCaptureComponent->EndPokeCount, 0);

							Target->SetEnabled(true);
						});

					FrameQueue.Enqueue([this] { TestEqual("Enable event triggered", EventCaptureComponent->EnableCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should not trigger events when near interaction is disabled",
				[this](const FDoneDelegate& Done)
				{
					Target->InteractionMode = static_cast<int32>(EUxtInteractionMode::Far);

					FrameQueue.Enqueue([this] { Hand.Translate(FVector(100, 0, 0)); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke not triggered", EventCaptureComponent->BeginPokeCount, 0);

							Hand.Translate(FVector(-100, 0, 0));
						});

					FrameQueue.Enqueue([this] { TestEqual("End poke not triggered", EventCaptureComponent->EndPokeCount, 0); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should only trigger events on selected primitives",
				[this](const FDoneDelegate& Done)
				{
					UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Target->GetOwner());
					Mesh->SetWorldLocation(TargetLocation + FVector(0, 0, 100));
					Mesh->RegisterComponent();

					Target->TouchablePrimitives.Add(Mesh);

					FrameQueue.Enqueue([this] { Hand.Translate(FVector(100, 0, 0)); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke not triggered", EventCaptureComponent->BeginPokeCount, 0);

							Hand.Translate(FVector(-100, 0, 0));
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("End poke not triggered", EventCaptureComponent->EndPokeCount, 0);

							Hand.Translate(FVector(100, 0, 100));
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke triggered", EventCaptureComponent->BeginPokeCount, 1);

							Hand.Translate(FVector(-100, 0, 0));
						});

					FrameQueue.Enqueue([this] { TestEqual("End poke triggered", EventCaptureComponent->EndPokeCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});

	Describe(
		"Far Interaction",
		[this]
		{
			BeforeEach([this] { Hand.Configure(EUxtInteractionMode::Far, TargetLocation); });

			AfterEach([this] { Hand.Reset(); });

			LatentIt(
				"should trigger focus events",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Starts with focus", EventCaptureComponent->BeginFocusCount, 1);

							Hand.Translate(FVector(0, 100, 0));
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("End focus triggered", EventCaptureComponent->EndFocusCount, 1);

							Hand.Translate(FVector(0, -100, 0));
						});

					FrameQueue.Enqueue([this] { TestEqual("Begin focus triggered", EventCaptureComponent->BeginFocusCount, 2); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should trigger poke events",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke triggered", EventCaptureComponent->BeginPokeCount, 1);

							Hand.SetGrabbing(false);
						});

					FrameQueue.Enqueue([this] { TestEqual("End poke triggered", EventCaptureComponent->EndPokeCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should not trigger events when disabled",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue(
						[this]
						{
							Target->SetEnabled(false);
							Hand.SetGrabbing(true);
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Disable event triggered", EventCaptureComponent->DisableCount, 1);
							TestEqual("Begin poke not triggered", EventCaptureComponent->BeginPokeCount, 0);

							Hand.SetGrabbing(false);
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("End poke not triggered", EventCaptureComponent->EndPokeCount, 0);

							Target->SetEnabled(true);
						});

					FrameQueue.Enqueue([this] { TestEqual("Enable event triggered", EventCaptureComponent->EnableCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should not trigger events when far interaction is disabled",
				[this](const FDoneDelegate& Done)
				{
					Target->InteractionMode = static_cast<int32>(EUxtInteractionMode::Near);

					FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke triggered", EventCaptureComponent->BeginPokeCount, 0);

							Hand.SetGrabbing(false);
						});

					FrameQueue.Enqueue([this] { TestEqual("End poke triggered", EventCaptureComponent->EndPokeCount, 0); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should only trigger events on selected primitives",
				[this](const FDoneDelegate& Done)
				{
					UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Target->GetOwner());
					Mesh->SetWorldLocation(TargetLocation + FVector(0, 0, 100));
					Mesh->RegisterComponent();

					Target->TouchablePrimitives.Add(Mesh);

					FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke not triggered", EventCaptureComponent->BeginPokeCount, 0);

							Hand.SetGrabbing(false);
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("End poke not triggered", EventCaptureComponent->EndPokeCount, 0);

							Hand.Translate(FVector(0, 0, 100));
							Hand.SetGrabbing(true);
						});

					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Begin poke triggered", EventCaptureComponent->BeginPokeCount, 1);

							Hand.SetGrabbing(false);
						});

					FrameQueue.Enqueue([this] { TestEqual("End poke triggered", EventCaptureComponent->EndPokeCount, 1); });

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
