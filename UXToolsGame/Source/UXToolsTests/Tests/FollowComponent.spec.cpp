// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "UxtTestUtils.h"

#include "Behaviors/UxtFollowComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtFollowComponent* CreateTestComponent(UWorld* World, const FVector& Location)
	{
		AActor* actor = World->SpawnActor<AActor>();

		USceneComponent* root = NewObject<USceneComponent>(actor);
		actor->SetRootComponent(root);
		root->SetWorldLocation(Location);
		root->RegisterComponent();

		UUxtFollowComponent* testTarget = NewObject<UUxtFollowComponent>(actor);
		testTarget->SetAutoActivate(true);
		testTarget->RegisterComponent();

		UStaticMeshComponent* mesh = UxtTestUtils::CreateBoxStaticMesh(actor, FVector(0.3f));
		mesh->SetupAttachment(actor->GetRootComponent());
		mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		mesh->SetCollisionProfileName(TEXT("OverlapAll"));
		mesh->SetGenerateOverlapEvents(true);
		mesh->RegisterComponent();

		AActor* actorToFollow = World->SpawnActor<AActor>();
		USceneComponent* rootToFollow = NewObject<USceneComponent>(actorToFollow);
		actorToFollow->SetRootComponent(rootToFollow);
		rootToFollow->SetWorldLocation(Location + FVector::BackwardVector * testTarget->DefaultDistance);
		rootToFollow->RegisterComponent();

		testTarget->ActorToFollow = actorToFollow;

		return testTarget;
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

UUxtFollowComponent* Follow;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(FollowComponentSpec)

void FollowComponentSpec::Define()
{
	Describe("Follow component", [this] {
		BeforeEach([this] {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			FVector Center(50, 0, 0);
			Follow = CreateTestComponent(World, Center);
			Follow->LerpTime = 0;
			Follow->bInterpolatePose = false;
		});

		AfterEach([this] {
			FrameQueue.Reset();

			Follow->GetOwner()->Destroy();
			Follow = nullptr;
		});

		LatentIt("stays within distance and angle limits", [this](const FDoneDelegate& Done) {
			Follow->bIgnoreDistanceClamp = false;
			Follow->bIgnoreAngleClamp = false;

			EnqueueDistanceTest();
			EnqueueAngleTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("stays within angle limits only", [this](const FDoneDelegate& Done) {
			Follow->bIgnoreDistanceClamp = true;
			Follow->bIgnoreAngleClamp = false;

			EnqueueDistanceTest();
			EnqueueAngleTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("stays within distance limits only", [this](const FDoneDelegate& Done) {
			Follow->bIgnoreDistanceClamp = false;
			Follow->bIgnoreAngleClamp = true;

			EnqueueDistanceTest();
			EnqueueAngleTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("orientation is world locked", [this](const FDoneDelegate& Done) {
			Follow->OrientationType = EUxtFollowOrientBehavior::WorldLock;

			EnqueueOrientationTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("orientation is facing camera", [this](const FDoneDelegate& Done) {
			Follow->OrientationType = EUxtFollowOrientBehavior::FaceCamera;

			EnqueueOrientationTest();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void FollowComponentSpec::EnqueueDistanceTest()
{
	const bool bIgnoreDistance = Follow->bIgnoreDistanceClamp;

	// Move the target past min distance extents
	FrameQueue.Enqueue([this] {
		const FVector ActorLocation = Follow->ActorToFollow->GetActorLocation();
		float MinFollowDist = Follow->MinimumDistance;

		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		FVector TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		const float Distance = TargetToComponent.Size();
		TargetToComponent.Normalize();

		Follow->ActorToFollow->SetActorLocation(ActorLocation + TargetToComponent * (Distance - (MinFollowDist * 0.5f)));
	});
	// Check behavior is expected for min bounds
	FrameQueue.Enqueue([this, bIgnoreDistance] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		const float Distance = FVector::Distance(FollowTransform.GetLocation(), TargetTransform.GetLocation());

		TestEqual("Follow component does not subceed minimum bounds", Distance < Follow->MinimumDistance, bIgnoreDistance);
	});
	// Move the target past max distance extents
	FrameQueue.Enqueue([this] {
		const FVector ActorLocation = Follow->ActorToFollow->GetActorLocation();
		float MaxFollowDist = Follow->MaximumDistance;

		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		FVector TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		const float Distance = TargetToComponent.Size();
		TargetToComponent.Normalize();

		Follow->ActorToFollow->SetActorLocation(ActorLocation - TargetToComponent * ((MaxFollowDist * 1.5f) - Distance));
	});
	// Check behavior is expected for max bounds
	FrameQueue.Enqueue([this, bIgnoreDistance] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		const float Distance = FVector::Distance(FollowTransform.GetLocation(), TargetTransform.GetLocation());

		TestEqual("Follow component does not exceed maximum bounds", Distance > Follow->MaximumDistance, bIgnoreDistance);
	});
}

void FollowComponentSpec::EnqueueAngleTest()
{
	const bool bIgnoreAngular = Follow->bIgnoreAngleClamp;

	// Move the target past horizontal angle extents
	FrameQueue.Enqueue([this] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		const FVector TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		const FVector TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

		float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, FVector::UpVector);
		float MaxHorizontal = FMath::DegreesToRadians(Follow->MaxViewHorizontalDegrees);

		const FQuat NewTargetRot(FVector::UpVector, (MaxHorizontal * 1.5f) - CurrAngle);

		Follow->ActorToFollow->SetActorRotation(NewTargetRot.Rotator());
	});
	// Check behavior is expected for horizontal bounds
	FrameQueue.Enqueue([this, bIgnoreAngular] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		const FVector TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		const FVector TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

		float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, FVector::UpVector);

		TestEqual("Follow component ignore angular option matches behavior", FMath::IsNearlyZero(CurrAngle, 1.0e-7f), bIgnoreAngular);

		TestTrue("Follow component does not exceed horizontal bounds", CurrAngle <= Follow->MaxViewHorizontalDegrees);
	});
	// Move the target past vertical angle extents
	FrameQueue.Enqueue([this] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		const FVector TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		const FVector TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

		float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, TargetTransform.GetUnitAxis(EAxis::Y));
		float MaxVertical = FMath::DegreesToRadians(Follow->MaxViewVerticalDegrees);

		const FQuat NewTargetRot(FVector::RightVector, (MaxVertical * 1.5f) - CurrAngle);

		Follow->ActorToFollow->SetActorRotation(NewTargetRot.Rotator());
	});
	// Check behavior is expected for vertical bounds
	FrameQueue.Enqueue([this, bIgnoreAngular] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();
		const FVector TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		const FVector TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

		float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, TargetTransform.GetUnitAxis(EAxis::Y));

		TestEqual("Follow component ignore angular option matches behavior", FMath::IsNearlyZero(CurrAngle, 1.0e-7f), bIgnoreAngular);

		TestTrue("Follow component does not exceed vertical bounds", CurrAngle <= Follow->MaxViewVerticalDegrees);
	});
}

void FollowComponentSpec::EnqueueOrientationTest()
{
	const FQuat InitialRotation = Follow->GetOwner()->GetActorRotation().Quaternion();
	const bool bFacing = (Follow->OrientationType == EUxtFollowOrientBehavior::FaceCamera);

	// Move the target halfway to the dead zone degrees
	FrameQueue.Enqueue([this] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();

		float DeadzoneAngle = FMath::DegreesToRadians(Follow->OrientToCameraDeadzoneDegrees);
		const FQuat Rotation(FVector::UpVector, DeadzoneAngle * 0.5f);

		const FVector ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();

		const FVector NewTargetPosition = FollowTransform.GetLocation() + Rotation * ComponentToTarget;

		Follow->ActorToFollow->SetActorLocation(NewTargetPosition);
		Follow->ActorToFollow->SetActorRotation(Rotation.Rotator());
	});
	// Check behavior is expected for orientation type
	FrameQueue.Enqueue([this, InitialRotation, bFacing] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();

		const FVector ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();
		const FVector Cross = FVector::CrossProduct(FollowTransform.GetUnitAxis(EAxis::X), ComponentToTarget);
		const float Dot = FVector::DotProduct(FollowTransform.GetUnitAxis(EAxis::X), ComponentToTarget);

		TestEqual(
			"Follow component orientation has changed to match orientation type", InitialRotation != FollowTransform.GetRotation(),
			bFacing);

		TestEqual("Follow component orientation type matches behavior", FMath::IsNearlyZero(Cross.Size(), 0.001f), bFacing);

		if (bFacing)
		{
			TestTrue("Following actor's +X pointing towards ActorToFollow", Dot > 0.f);
		}
	});
	// Move the target past the dead zone degrees
	FrameQueue.Enqueue([this] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();

		float DeadzoneAngle = FMath::DegreesToRadians(Follow->OrientToCameraDeadzoneDegrees);
		const FQuat Rotation(FVector::UpVector, DeadzoneAngle);

		const FVector ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();

		const FVector NewTargetPosition = FollowTransform.GetLocation() + Rotation * ComponentToTarget;

		Follow->ActorToFollow->SetActorLocation(NewTargetPosition);
		Follow->ActorToFollow->SetActorRotation(Rotation.Rotator());
	});
	// Check behavior is expected for orientation type
	FrameQueue.Enqueue([this, InitialRotation] {
		const FTransform& TargetTransform = Follow->ActorToFollow->GetTransform();
		const FTransform& FollowTransform = Follow->GetOwner()->GetTransform();

		const FVector ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();
		const FVector Cross = FVector::CrossProduct(FollowTransform.GetUnitAxis(EAxis::X), ComponentToTarget);
		const float Dot = FVector::DotProduct(FollowTransform.GetUnitAxis(EAxis::X), ComponentToTarget);

		TestNotEqual("Follow component orientation has changed", InitialRotation, FollowTransform.GetRotation());

		TestTrue("Follow component orientation type matches behavior", FMath::IsNearlyZero(Cross.Size(), 0.001f));

		TestTrue("Following actor's +X pointing towards ActorToFollow", Dot > 0.f);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
