// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "FrameQueue.h"
#include "PressableButtonTestComponent.h"
#include "PressableButtonTestUtils.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Behaviors/UxtFollowComponent.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtFollowComponent* CreateTestComponent(UWorld* World, const FVector& Location)
	{
		AActor* Actor = World->SpawnActor<AActor>();

		UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Actor);
		Mesh->RegisterComponent();

		Actor->SetRootComponent(Mesh);
		Actor->SetActorLocation(Location);
		Actor->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 180)));

		UUxtFollowComponent* FollowComponent = NewObject<UUxtFollowComponent>(Actor);
		FollowComponent->bInterpolatePose = false;
		FollowComponent->LerpTime = 0;
		FollowComponent->RegisterComponent();

		return FollowComponent;
	}

	float SimplifyAngle(float Angle)
	{
		while (Angle > PI)
		{
			Angle -= 2 * PI;
		}

		while (Angle < -PI)
		{
			Angle += 2 * PI;
		}

		return Angle;
	}

	float AngleBetweenOnPlane(FVector From, FVector To, FVector Normal)
	{
		From.Normalize();
		To.Normalize();
		Normal.Normalize();

		FVector Right = FVector::CrossProduct(Normal, From);
		FVector Forward = FVector::CrossProduct(Right, Normal);

		float Angle = FMath::Atan2(FVector::DotProduct(To, Right), FVector::DotProduct(To, Forward));

		return SimplifyAngle(Angle);
	}
} // namespace

BEGIN_DEFINE_SPEC(
	FollowComponentSpec, "UXTools.FollowComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

void EnqueueDistanceTest();
void EnqueueAngleTest();
void EnqueueOrientationTest();
void EnqueueButtonTest();

UWorld* World;
UUxtFollowComponent* Target;
FFrameQueue FrameQueue;

// The following components are used for the button test
UPressableButtonTestComponent* ButtonEventCaptureObj = nullptr;
FUxtTestHand Hand = FUxtTestHand(EControllerHand::Right);

END_DEFINE_SPEC(FollowComponentSpec)

void FollowComponentSpec::Define()
{
	BeforeEach(
		[this]
		{
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			FVector TargetLocation(75, 0, 0);

			World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			UxtTestUtils::EnableTestInputSystem();
			Hand.Configure(EUxtInteractionMode::Near, TargetLocation);

			Target = CreateTestComponent(World, TargetLocation);
		});

	AfterEach(
		[this]
		{
			Target->GetOwner()->Destroy();
			Target = nullptr;

			FrameQueue.Reset();

			Hand.Reset();
			UxtTestUtils::DisableTestInputSystem();
		});

	Describe(
		"basic functionality",
		[this]
		{
			LatentIt(
				"stays within distance and angle limits",
				[this](const FDoneDelegate& Done)
				{
					Target->bIgnoreDistanceClamp = false;
					Target->bIgnoreAngleClamp = false;

					EnqueueDistanceTest();
					EnqueueAngleTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"stays within angle limits only",
				[this](const FDoneDelegate& Done)
				{
					Target->bIgnoreDistanceClamp = true;
					Target->bIgnoreAngleClamp = false;

					EnqueueDistanceTest();
					EnqueueAngleTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"stays within distance limits only",
				[this](const FDoneDelegate& Done)
				{
					Target->bIgnoreDistanceClamp = false;
					Target->bIgnoreAngleClamp = true;

					EnqueueDistanceTest();
					EnqueueAngleTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"maintains correct orientation when world locked",
				[this](const FDoneDelegate& Done)
				{
					Target->OrientationType = EUxtFollowOrientBehavior::WorldLock;

					EnqueueOrientationTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"maintains correct orientation when facing camera",
				[this](const FDoneDelegate& Done)
				{
					Target->OrientationType = EUxtFollowOrientBehavior::FaceCamera;

					EnqueueOrientationTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});

	Describe(
		"with a button",
		[this]
		{
			BeforeEach(
				[this]
				{
					AActor* TargetActor = Target->GetOwner();
					UUxtPressableButtonComponent* ButtonComponent = CreateTestButtonComponent(TargetActor, TargetActor->GetActorLocation());

					ButtonEventCaptureObj = NewObject<UPressableButtonTestComponent>(TargetActor);
					ButtonEventCaptureObj->RegisterComponent();

					ButtonComponent->OnButtonPressed.AddDynamic(ButtonEventCaptureObj, &UPressableButtonTestComponent::IncrementPressed);
					ButtonComponent->OnButtonReleased.AddDynamic(ButtonEventCaptureObj, &UPressableButtonTestComponent::IncrementReleased);
				});

			LatentIt(
				"should work after location/orientation changed",
				[this](const FDoneDelegate& Done)
				{
					EnqueueDistanceTest();
					EnqueueAngleTest();
					EnqueueButtonTest();
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

void FollowComponentSpec::EnqueueDistanceTest()
{
	// Move the camera to within the minimum bounds
	FrameQueue.Enqueue(
		[this]
		{
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			FVector HeadPosition = TargetPosition - FVector(Target->MinimumDistance / 2, 0, 0);
			UxtTestUtils::SetTestHeadLocation(HeadPosition);
		});
	// Verify the target has moved away from the camera unless bIgnoreDistanceClamp is true.
	FrameQueue.Enqueue(
		[this]
		{
			FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(World).GetLocation();
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			double Distance = FVector::Distance(HeadPosition, TargetPosition);
			if (Target->bIgnoreDistanceClamp)
			{
				TestTrue("Follow component ignores distance clamp", Distance < Target->MinimumDistance);
			}
			else
			{
				TestTrue("Follow component stays outside minimum bounds", Distance >= Target->MinimumDistance);
			}
		});
	// Move the camera to outside the maximum bounds
	FrameQueue.Enqueue(
		[this]
		{
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			FVector HeadPosition = TargetPosition - FVector(Target->MaximumDistance * 2, 0, 0);
			UxtTestUtils::SetTestHeadLocation(HeadPosition);
		});
	// Verify the target has moved towards the camera unless bIgnoreDistanceClamp is true.
	FrameQueue.Enqueue(
		[this]
		{
			FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(World).GetLocation();
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			double Distance = FVector::Distance(HeadPosition, TargetPosition);
			if (Target->bIgnoreDistanceClamp)
			{
				TestTrue("Follow component ignores distance clamp", Distance > Target->MinimumDistance);
			}
			else
			{
				TestTrue("Follow component stays inside the maximum bounds", Distance <= Target->MaximumDistance);
			}
		});
}

void FollowComponentSpec::EnqueueAngleTest()
{
	// Move the camera past the horizontal threshold
	FrameQueue.Enqueue(
		[this]
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(World);
			FTransform TargetTransform = Target->GetOwner()->GetActorTransform();
			FVector HeadForward = HeadTransform.GetUnitAxis(EAxis::X);
			FVector HeadUp = HeadTransform.GetUnitAxis(EAxis::Z);

			FVector HeadToTarget = TargetTransform.GetLocation() - HeadTransform.GetLocation();
			float AngleToTarget = AngleBetweenOnPlane(HeadForward, HeadToTarget, HeadUp);

			float MaxRotation = FMath::DegreesToRadians(Target->MaxViewHorizontalDegrees);
			FQuat HeadRotation(FVector::UpVector, (MaxRotation * 2) - AngleToTarget);
			UxtTestUtils::SetTestHeadRotation(HeadRotation.Rotator());
		});
	// Verify the target rotated towards the camera
	FrameQueue.Enqueue(
		[this]
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(World);
			FTransform TargetTransform = Target->GetOwner()->GetActorTransform();
			FVector HeadForward = HeadTransform.GetUnitAxis(EAxis::X);
			FVector HeadUp = HeadTransform.GetUnitAxis(EAxis::Z);

			FVector HeadToTarget = TargetTransform.GetLocation() - HeadTransform.GetLocation();
			float AngleToTarget = FMath::RadiansToDegrees(FMath::Abs(AngleBetweenOnPlane(HeadForward, HeadToTarget, HeadUp)));
			if (Target->bIgnoreAngleClamp)
			{
				TestTrue("Follow component ignores angle clamp", FMath::IsNearlyZero(AngleToTarget, 1.0e-7f));
			}
			else
			{
				TestTrue("Follow component does not exceed maximum angle", AngleToTarget <= Target->MaxViewHorizontalDegrees);
			}
		});
	// Move the camera past the vertical threshold
	FrameQueue.Enqueue(
		[this]
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(World);
			FTransform TargetTransform = Target->GetOwner()->GetActorTransform();
			FVector HeadForward = HeadTransform.GetUnitAxis(EAxis::X);
			FVector HeadRight = HeadTransform.GetUnitAxis(EAxis::Y);

			FVector HeadToTarget = TargetTransform.GetLocation() - HeadTransform.GetLocation();
			float AngleToTarget = AngleBetweenOnPlane(HeadForward, HeadToTarget, HeadRight);

			float MaxRotation = FMath::DegreesToRadians(Target->MaxViewVerticalDegrees);
			FQuat HeadRotation(FVector::RightVector, (MaxRotation * 2) - AngleToTarget);
			UxtTestUtils::SetTestHeadRotation(HeadRotation.Rotator());
		});
	// Verify the target rotated towards the camera
	FrameQueue.Enqueue(
		[this]
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(World);
			FTransform TargetTransform = Target->GetOwner()->GetActorTransform();
			FVector HeadForward = HeadTransform.GetUnitAxis(EAxis::X);
			FVector HeadRight = HeadTransform.GetUnitAxis(EAxis::Y);

			FVector HeadToTarget = TargetTransform.GetLocation() - HeadTransform.GetLocation();
			float AngleToTarget = FMath::RadiansToDegrees(FMath::Abs(AngleBetweenOnPlane(HeadForward, HeadToTarget, HeadRight)));
			if (Target->bIgnoreAngleClamp)
			{
				TestTrue("Follow component ignores angle clamp", FMath::IsNearlyZero(AngleToTarget, 1.0e-7f));
			}
			else
			{
				TestTrue("Follow component does not exceed maximum angle", AngleToTarget <= Target->MaxViewVerticalDegrees);
			}
		});
}

void FollowComponentSpec::EnqueueOrientationTest()
{
	const FQuat InitialRotation = Target->GetOwner()->GetActorRotation().Quaternion();

	// Move the camera halfway to the deadzone edge
	FrameQueue.Enqueue(
		[this]
		{
			float Deadzone = FMath::DegreesToRadians(Target->OrientToCameraDeadzoneDegrees);
			FQuat HeadRotation(FVector::UpVector, Deadzone / 2);
			UxtTestUtils::SetTestHeadRotation(HeadRotation.Rotator());

			FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(World).GetLocation();
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			FVector TargetToHead = HeadPosition - TargetPosition;

			FVector NewHeadPosition = TargetPosition + HeadRotation * TargetToHead;
			UxtTestUtils::SetTestHeadLocation(NewHeadPosition);
		});
	// Verify the behaviour matches the OrientationType setting
	FrameQueue.Enqueue(
		[this, InitialRotation]
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(World);
			FTransform TargetTransform = Target->GetOwner()->GetActorTransform();
			FVector TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

			FVector TargetToHead = HeadTransform.GetLocation() - TargetTransform.GetLocation();
			FVector Cross = FVector::CrossProduct(TargetForward, TargetToHead);
			float Dot = FVector::DotProduct(TargetForward, TargetToHead);

			if (Target->OrientationType == EUxtFollowOrientBehavior::WorldLock)
			{
				TestEqual("Follow component is world locked", TargetTransform.GetRotation(), InitialRotation);
				TestFalse("Follow component orientation is correct", FMath::IsNearlyZero(Cross.Size(), 0.001f));
			}
			else
			{
				TestNotEqual("Follow component is not world locked", TargetTransform.GetRotation(), InitialRotation);
				TestTrue("Follow component orientation is correct", FMath::IsNearlyZero(Cross.Size(), 0.001f));
				TestTrue("Follow component is facing the camera", Dot > 0);
			}
		});
	// Move the camera past the deadzone edge
	FrameQueue.Enqueue(
		[this]
		{
			float Deadzone = FMath::DegreesToRadians(Target->OrientToCameraDeadzoneDegrees);
			FQuat HeadRotation(FVector::UpVector, Deadzone);
			UxtTestUtils::SetTestHeadRotation(HeadRotation.Rotator());

			FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(World).GetLocation();
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			FVector TargetToHead = HeadPosition - TargetPosition;

			FVector NewHeadPosition = TargetPosition + HeadRotation * TargetToHead;
			UxtTestUtils::SetTestHeadLocation(NewHeadPosition);
		});
	// Verify the behaviour matches the OrientationType setting
	FrameQueue.Enqueue(
		[this, InitialRotation]
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(World);
			FTransform TargetTransform = Target->GetOwner()->GetActorTransform();
			FVector TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

			FVector TargetToHead = HeadTransform.GetLocation() - TargetTransform.GetLocation();
			FVector Cross = FVector::CrossProduct(TargetForward, TargetToHead);
			float Dot = FVector::DotProduct(TargetForward, TargetToHead);

			TestNotEqual("Follow component is not world locked", TargetTransform.GetRotation(), InitialRotation);
			TestTrue("Follow component orientation is correct", FMath::IsNearlyZero(Cross.Size(), 0.001f));
			TestTrue("Follow component is facing the camera", Dot > 0);
		});
}

void FollowComponentSpec::EnqueueButtonTest()
{
	// Move the hand to be in-front of the button
	FrameQueue.Enqueue(
		[this]
		{
			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(World).GetLocation();

			FVector HeadToTarget = TargetPosition - HeadPosition;
			HeadToTarget.Normalize();
			Hand.SetTranslation(TargetPosition - (HeadToTarget * 10));
		});
	// Move the hand towards the button to press it
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Button pressed event not generated before moving the hand", ButtonEventCaptureObj->PressedCount, 0);

			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			Hand.SetTranslation(TargetPosition);
		});
	// Verify that the button was pressed and move the hand away
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Button press as expected", ButtonEventCaptureObj->PressedCount, 1);

			FVector TargetPosition = Target->GetOwner()->GetActorLocation();
			FVector HeadPosition = UUxtFunctionLibrary::GetHeadPose(World).GetLocation();

			FVector HeadToTarget = TargetPosition - HeadPosition;
			HeadToTarget.Normalize();
			Hand.SetTranslation(TargetPosition - (HeadToTarget * 10));
		});
	// Verify that the button was released
	FrameQueue.Enqueue([this] { TestEqual("Button release as expected", ButtonEventCaptureObj->ReleasedCount, 1); });
}

#endif // WITH_DEV_AUTOMATION_TESTS
