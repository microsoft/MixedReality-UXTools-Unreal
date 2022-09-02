// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "TapToPlaceTestComponent.h"
#include "UxtTestHand.h"
#include "UxtTestUtils.h"

#include "Behaviors/UxtTapToPlaceComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtTapToPlaceComponent* CreateTestComponent(UWorld* World, const FVector& Location, const FVector& Bounds)
	{
		AActor* Actor = World->SpawnActor<AActor>();

		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocation(Location);
		Root->RegisterComponent();

		UUxtTapToPlaceComponent* TestTarget = NewObject<UUxtTapToPlaceComponent>(Actor);
		TestTarget->RegisterComponent();

		UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Actor, Bounds);
		Mesh->SetupAttachment(Root);
		Mesh->RegisterComponent();

		TestTarget->SetTargetComponent(Mesh);

		return TestTarget;
	}

	UStaticMeshComponent* CreatePlacementSurface(UWorld* World, const FVector& Location, const FVector& Bounds)
	{
		AActor* Actor = World->SpawnActor<AActor>();

		UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Actor, Bounds);
		Actor->SetRootComponent(Mesh);
		Mesh->SetWorldLocation(Location);
		Mesh->RegisterComponent();

		return Mesh;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	TapToPlaceComponentSpec, "UXTools.TapToPlaceComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

void EnqueuePlacementTest(bool bSurfaceExists);
void EnqueueOrientationTest();
void EnqueueFarInteractionTest();

UUxtTapToPlaceComponent* TapToPlace;
UStaticMeshComponent* Surface;
UTapToPlaceTestComponent* TestComponent;
FFrameQueue FrameQueue;

const FVector StartingHeadPos = FVector::ZeroVector;
const FRotator StartingHeadRot = FRotator::ZeroRotator;

FUxtTestHand Hand = FUxtTestHand(EControllerHand::Left);

const FVector Centre{50, 0, 0};
const FVector Delta{50, 0, 0};

const FVector TargetBounds{0.3f, 0.3f, 0.3f};
const FVector SurfaceBounds{0.1f, 1, 1};

FVector TestPosition;

FVector InitialOrientation; // orientation of the TapToPlace target before the orientation test

END_DEFINE_SPEC(TapToPlaceComponentSpec)

void TapToPlaceComponentSpec::Define()
{
	Describe(
		"TapToPlace component",
		[this]
		{
			BeforeEach(
				[this]
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

					TapToPlace = CreateTestComponent(World, Centre, TargetBounds);
					TapToPlace->bInterpolatePose = false;

					UxtTestUtils::EnableTestInputSystem();
					Hand.Configure(EUxtInteractionMode::Far, Centre);

					TestComponent = NewObject<UTapToPlaceTestComponent>(TapToPlace->GetOwner());
					TestComponent->RegisterComponent();
					TapToPlace->OnBeginFocus.AddDynamic(TestComponent, &UTapToPlaceTestComponent::OnFocusEnter);
					TapToPlace->OnUpdateFocus.AddDynamic(TestComponent, &UTapToPlaceTestComponent::OnFocusUpdated);
					TapToPlace->OnEndFocus.AddDynamic(TestComponent, &UTapToPlaceTestComponent::OnFocusExit);
					TapToPlace->OnBeginPlacing.AddDynamic(TestComponent, &UTapToPlaceTestComponent::OnPlacementStarted);
					TapToPlace->OnEndPlacing.AddDynamic(TestComponent, &UTapToPlaceTestComponent::OnPlacementEnded);

					InitialOrientation = TapToPlace->GetTargetComponent()->GetComponentRotation().Vector();
				});

			AfterEach(
				[this]
				{
					FrameQueue.Reset();

					TapToPlace->GetOwner()->Destroy();
					TapToPlace = nullptr;

					UxtTestUtils::DisableTestInputSystem();
					Hand.Reset();

					if (Surface)
					{
						Surface->GetOwner()->Destroy();
						Surface = nullptr;
					}
				});

			LatentIt(
				"should be placed at default placement distance as no surface to hit",
				[this](const FDoneDelegate& Done)
				{
					EnqueuePlacementTest(false);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"should begin and end placement on far tap",
				[this](const FDoneDelegate& Done)
				{
					EnqueueFarInteractionTest();

					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			Describe(
				"should have correct orientation",
				[this]
				{
					BeforeEach([this] { Surface = CreatePlacementSurface(UxtTestUtils::GetTestWorld(), Centre + Delta, SurfaceBounds); });

					LatentIt(
						"when placed against a surface",
						[this](const FDoneDelegate& Done)
						{
							EnqueuePlacementTest(true);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"when set to always face the camera",
						[this](const FDoneDelegate& Done)
						{
							TapToPlace->OrientationType = EUxtTapToPlaceOrientBehavior::AlignToCamera;
							EnqueueOrientationTest();
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"when set to be aligned to surface normals",
						[this](const FDoneDelegate& Done)
						{
							TapToPlace->OrientationType = EUxtTapToPlaceOrientBehavior::AlignToSurface;
							EnqueueOrientationTest();
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"when set to maintain original orientation",
						[this](const FDoneDelegate& Done)
						{
							TapToPlace->OrientationType = EUxtTapToPlaceOrientBehavior::MaintainOrientation;
							EnqueueOrientationTest();
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});
				});
		});
}

void TapToPlaceComponentSpec::EnqueuePlacementTest(const bool bSurfaceExists)
{
	// Record position and begin placement
	FrameQueue.Enqueue(
		[this]
		{
			TestPosition = TapToPlace->GetTargetComponent()->GetComponentLocation();

			TapToPlace->StartPlacement();
		});
	// Ensure placement has started and then move the head
	FrameQueue.Enqueue(
		[this, bSurfaceExists]
		{
			const FVector NewPosition = TapToPlace->GetTargetComponent()->GetComponentLocation();
			const FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(UxtTestUtils::GetTestWorld()).GetLocation();

			const float DistanceToHead = FVector::Dist(NewPosition, HeadPosition);

			float ExpectedDistance = TapToPlace->DefaultPlacementDistance;
			if (bSurfaceExists)
			{
				const USceneComponent* Target = TapToPlace->GetTargetComponent();
				const float SurfaceDepth = Surface->CalcBounds(Surface->GetComponentToWorld()).BoxExtent.X;
				const float TargetDepth = Target->CalcBounds(Target->GetComponentToWorld()).BoxExtent.X;
				ExpectedDistance = Centre.X + Delta.X - SurfaceDepth - TargetDepth;
			}

			TestNotEqual("TapToPlace target has not moved after selection started", NewPosition, TestPosition);
			TestEqual("TapToPlace target at unexpected distance from head", DistanceToHead, ExpectedDistance);
			TestTrue("TapToPlace did not fire expected event", TestComponent->OnPlacementStartedReceived);

			TestPosition = NewPosition;

			UxtTestUtils::SetTestHeadLocation(StartingHeadPos + FVector(0, 5, 5));
		});
	// Ensure that target has moved with the head and then end placement
	FrameQueue.Enqueue(
		[this]
		{
			const FVector NewPosition = TapToPlace->GetTargetComponent()->GetComponentLocation();
			const FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(UxtTestUtils::GetTestWorld()).GetLocation();

			// the x distance of the target should not have changed, however the yz should have changed
			// to match the new head position
			TestEqual("TapToPlace target distance from head changed unexpectedly", NewPosition.X, TestPosition.X);
			TestEqual("TapToPlace target has not moved with the head", NewPosition.Y, HeadPosition.Y);
			TestEqual("TapToPlace target has not moved with the head", NewPosition.Z, HeadPosition.Z);

			TestPosition = NewPosition;

			TapToPlace->EndPlacement();
			UxtTestUtils::SetTestHeadLocation(StartingHeadPos + FVector::RightVector * 10);
		});
	// Ensure that the placed target has not moved
	FrameQueue.Enqueue(
		[this]
		{
			const FVector NewPosition = TapToPlace->GetTargetComponent()->GetComponentLocation();

			TestEqual("TapToPlace target distance from head changed unexpectedly", NewPosition, TestPosition);
			TestTrue("TapToPlace did not fire expected event", TestComponent->OnPlacementEndedReceived);
		});
}

void TapToPlaceComponentSpec::EnqueueOrientationTest()
{
	auto TestOrientation = [this]()
	{
		const FQuat Current = TapToPlace->GetTargetComponent()->GetComponentQuat();

		const FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(UxtTestUtils::GetTestWorld()).GetLocation();
		const FVector TargetToHead = HeadPosition - TapToPlace->GetTargetComponent()->GetComponentLocation();

		const FVector Cross = FVector::CrossProduct(TargetToHead, Current.GetForwardVector());
		const float Dot = FVector::DotProduct(TargetToHead, Current.GetForwardVector());

		switch (TapToPlace->OrientationType)
		{
		case EUxtTapToPlaceOrientBehavior::AlignToCamera:
			return FMath::IsNearlyEqual(Cross.Size(), 0, KINDA_SMALL_NUMBER) && Dot > 0;

		case EUxtTapToPlaceOrientBehavior::AlignToSurface:
			return FMath::IsNearlyEqual(Current.AngularDistance(Surface->GetComponentQuat()), PI, KINDA_SMALL_NUMBER);

		case EUxtTapToPlaceOrientBehavior::MaintainOrientation:
			return InitialOrientation.Equals(TapToPlace->GetTargetComponent()->GetComponentRotation().Vector());
		}

		return false;
	};

	// Begin placement
	FrameQueue.Enqueue([this] { TapToPlace->StartPlacement(); });
	// Test Orientation and rotate head
	FrameQueue.Enqueue(
		[this, TestOrientation]
		{
			TestTrue("TapToPlace target orientation is unexpected", TestOrientation());

			UxtTestUtils::SetTestHeadRotation(StartingHeadRot + FRotator(0, 10, 0));
		});
	// Test Orientation after rotation
	FrameQueue.Enqueue([this, TestOrientation] { TestTrue("TapToPlace target orientation is unexpected", TestOrientation()); });
}

void TapToPlaceComponentSpec::EnqueueFarInteractionTest()
{
	// Far tap to initiate placement
	FrameQueue.Enqueue([this] { Hand.SetGrabbing(true); });
	// Ensure placement has not started and then far release
	FrameQueue.Enqueue(
		[this]
		{
			TestFalse("Tap to place placement has started", TestComponent->OnPlacementStartedReceived);

			Hand.SetGrabbing(false);
		});
	// Ensure placement has started and then move the hand
	FrameQueue.Enqueue(
		[this]
		{
			TestTrue("Tap to place placement has started", TestComponent->OnPlacementStartedReceived);

			Hand.SetTranslation(Hand.GetTransform().GetLocation() + FVector::LeftVector * 2);
		});
	// Far tap while not pointing at the target
	FrameQueue.Enqueue(
		[this]
		{
			TestFalse("Tap to place placement has ended", TestComponent->OnPlacementEndedReceived);

			Hand.SetGrabbing(true);
		});
	// Far release while not pointing at the target
	FrameQueue.Enqueue(
		[this]
		{
			TestFalse("Tap to place placement has ended", TestComponent->OnPlacementEndedReceived);

			Hand.SetGrabbing(false);
		});
	// Test for placement ended
	FrameQueue.Enqueue(
		[this]
		{
			TestTrue("Tap to place placement has ended", TestComponent->OnPlacementEndedReceived);

			Hand.SetGrabbing(false);
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
