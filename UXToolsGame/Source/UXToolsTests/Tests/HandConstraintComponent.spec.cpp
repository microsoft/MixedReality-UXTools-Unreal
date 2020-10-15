// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "HandConstraintListener.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Behaviors/UxtHandConstraintComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(
	HandConstraintComponentSpec, "UXTools.HandConstraintComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

bool TestQuatEqual(const FString& What, const FQuat& Actual, const FQuat& Expected, float Tolerance = KINDA_SMALL_NUMBER);

void TestGoal(const FVector& ExpectedLocation, const FQuat& ExpectedRotation);
void TestActorDistance(const FVector& ExpectedGoalLocation, const FQuat& ExpectedGoalRotation, float MaxDistance, float MaxAngle);

void EnqueueGoalLocationTest(EControllerHand Hand, EUxtHandConstraintZone Zone, const FVector& ExpectedGoalLocation);
void EnqueueGoalRotationTest(EControllerHand Hand, EUxtHandConstraintZone Zone, const FQuat& ExpectedGoalRotation);

FFrameQueue FrameQueue;
UUxtHandConstraintComponent* HandConstraint;
UHandConstraintListener* HandConstraintListener;

END_DEFINE_SPEC(HandConstraintComponentSpec)

bool HandConstraintComponentSpec::TestQuatEqual(const FString& What, const FQuat& Actual, const FQuat& Expected, float Tolerance)
{
	return TestEqual(What, Actual.GetAxisX(), Expected.GetAxisX(), Tolerance) &&
		   TestEqual(What, Actual.GetAxisY(), Expected.GetAxisY(), Tolerance) &&
		   TestEqual(What, Actual.GetAxisZ(), Expected.GetAxisZ(), Tolerance);
}

void HandConstraintComponentSpec::TestGoal(const FVector& ExpectedLocation, const FQuat& ExpectedRotation)
{
	// auto pg = HandConstraint->GetGoalLocation();
	// auto rg = HandConstraint->GetGoalRotation();
	// UE_LOG(LogTemp, Display, TEXT("goal LOC FVector(%.4ff, %.4ff, %.4ff)"), pg.X, pg.Y, pg.Z);
	// UE_LOG(LogTemp, Display, TEXT("goal ROT FQuat(%.4ff, %.4ff, %.4ff, %.4ff)"), rg.X, rg.Y, rg.Z, rg.W);
	TestTrue("Constraint is active", HandConstraint->IsConstraintActive());
	TestEqual("Goal location", HandConstraint->GetGoalLocation(), ExpectedLocation, 1.0e-3f);
	TestQuatEqual("Goal rotation", HandConstraint->GetGoalRotation(), ExpectedRotation, 1.0e-3f);
}

void HandConstraintComponentSpec::TestActorDistance(
	const FVector& ExpectedLocation, const FQuat& ExpectedRotation, float MaxActorDistance, float MaxActorAngle)
{
	// auto pa = HandConstraint->GetOwner()->GetActorLocation();
	// auto ra = HandConstraint->GetOwner()->GetActorQuat();
	// UE_LOG(LogTemp, Display, TEXT("actor LOC FVector(%.4ff, %.4ff, %.4ff)"), pa.X, pa.Y, pa.Z);
	// UE_LOG(LogTemp, Display, TEXT("actor ROT FQuat(%.4ff, %.4ff, %.4ff, %.4ff)"), ra.X, ra.Y, ra.Z, ra.W);
	TestTrue(
		"Actor linear distance below threshold",
		(HandConstraint->GetOwner()->GetActorLocation() - ExpectedLocation).Size() <= MaxActorDistance);
	TestTrue(
		"Actor angular distance below threshold",
		HandConstraint->GetOwner()->GetActorQuat().AngularDistance(ExpectedRotation) <= MaxActorAngle);
}

void HandConstraintComponentSpec::EnqueueGoalLocationTest(
	EControllerHand Hand, EUxtHandConstraintZone Zone, const FVector& ExpectedGoalLocation)
{
	FrameQueue.Enqueue([this, Hand, Zone]() {
		HandConstraint->Hand = Hand;
		HandConstraint->Zone = Zone;
	});
	FrameQueue.Enqueue([this, ExpectedGoalLocation]() {
		// auto v = HandConstraint->GetGoalLocation();
		// UE_LOG(LogTemp, Display, TEXT("LOC FVector(%.4ff, %.4ff, %.4ff)"), v.X, v.Y, v.Z);
		TestTrue("Constraint is active", HandConstraint->IsConstraintActive());
		TestEqual("Goal location", HandConstraint->GetGoalLocation(), ExpectedGoalLocation, 1.0e-3f);
	});
}

void HandConstraintComponentSpec::EnqueueGoalRotationTest(
	EControllerHand Hand, EUxtHandConstraintZone Zone, const FQuat& ExpectedGoalRotation)
{
	FrameQueue.Enqueue([this, Hand, Zone]() {
		HandConstraint->Hand = Hand;
		HandConstraint->Zone = Zone;
	});
	FrameQueue.Enqueue([this, ExpectedGoalRotation]() {
		// auto r = HandConstraint->GetGoalRotation();
		// UE_LOG(LogTemp, Display, TEXT("ROT FQuat(%.4ff, %.4ff, %.4ff, %.4ff)"), r.X, r.Y, r.Z, r.W);
		TestTrue("Constraint is active", HandConstraint->IsConstraintActive());
		TestQuatEqual("Goal rotation", HandConstraint->GetGoalRotation(), ExpectedGoalRotation, 1.0e-3f);
	});
}

void HandConstraintComponentSpec::Define()
{
	Describe("Hand constraint", [this] {
		BeforeEach([this]() {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			UxtTestUtils::EnableTestHandTracker();

			AActor* Actor = World->SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(Actor);
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();
			HandConstraint = NewObject<UUxtHandConstraintComponent>(Actor);
			// Create listener before registering so all events are recorded
			HandConstraintListener = NewObject<UHandConstraintListener>(HandConstraint);
			HandConstraint->OnConstraintActivated.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnConstraintActivated);
			HandConstraint->OnConstraintDeactivated.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnConstraintDeactivated);
			HandConstraint->OnBeginTracking.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnBeginTracking);
			HandConstraint->OnEndTracking.AddDynamic(HandConstraintListener, &UHandConstraintListener::OnEndTracking);
			HandConstraint->RegisterComponent();

			// Register all new components.
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();

			HandConstraint->GetOwner()->Destroy();
			HandConstraint = nullptr;
		});

		LatentIt("should become inactive when hand tracking is lost", [this](const FDoneDelegate& Done) {
			// Start deactivated
			UxtTestUtils::GetTestHandTracker().SetTracked(false);

			FrameQueue.Enqueue([this]() {
				TestFalse("Hand bounds are valid", (bool)HandConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", HandConstraint->IsConstraintActive());
				// Events start at 1 because the constraint is active before losing hand tracking
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 1);

				UxtTestUtils::GetTestHandTracker().SetTracked(true);
			});
			FrameQueue.Enqueue([this]() {
				TestTrue("Hand bounds are valid", (bool)HandConstraint->GetHandBounds().IsValid);
				TestTrue("Constraint is active", HandConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 2);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 1);

				UxtTestUtils::GetTestHandTracker().SetTracked(false);
			});
			FrameQueue.Enqueue([this, Done]() {
				TestFalse("Hand bounds are valid", (bool)HandConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", HandConstraint->IsConstraintActive());
				TestEqual("ConstraintActivated events", HandConstraintListener->NumConstraintActivated, 2);
				TestEqual("ConstraintDeactivated events", HandConstraintListener->NumConstraintDeactivated, 2);

				Done.Execute();
			});
		});

		LatentIt("goal should have valid location in Hand-Rotation mode", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::HandRotation;
			});

			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::UlnarSide, FVector(68.5221f, 14.8143f, -11.3704f));
			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::RadialSide, FVector(71.4779f, 25.1857f, -18.6296f));
			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::AboveFingerTips, FVector(76.2985f, 18.4254f, -14.6851f));
			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::BelowWrist, FVector(63.7015f, 21.5746f, -15.3149f));

			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::UlnarSide, FVector(71.4779f, 25.1857f, -18.6296f));
			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::RadialSide, FVector(68.5221f, 14.8143f, -11.3704f));
			EnqueueGoalLocationTest(
				EControllerHand::Right, EUxtHandConstraintZone::AboveFingerTips, FVector(76.2985f, 18.4254f, -14.6851f));
			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::BelowWrist, FVector(63.7015f, 21.5746f, -15.3149f));

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("goal should have valid location in Look-At-Camera mode", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;
			});

			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::UlnarSide, FVector(67.4658f, 28.8696f, -15.0000f));
			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::RadialSide, FVector(72.5342f, 11.1304f, -15.0000f));
			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::AboveFingerTips, FVector(70.0000f, 20.0000f, -7.1513f));
			EnqueueGoalLocationTest(EControllerHand::Left, EUxtHandConstraintZone::BelowWrist, FVector(70.0000f, 20.0000f, -22.8487f));

			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::UlnarSide, FVector(72.5342f, 11.1304f, -15.0000f));
			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::RadialSide, FVector(67.4658f, 28.8696f, -15.0000f));
			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::AboveFingerTips, FVector(70.0000f, 20.0000f, -7.1513f));
			EnqueueGoalLocationTest(EControllerHand::Right, EUxtHandConstraintZone::BelowWrist, FVector(70.0000f, 20.0000f, -22.8487f));

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("goal should have valid rotation in Hand-Rotation mode", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::HandRotation;
				HandConstraint->RotationMode = EUxtHandConstraintRotationMode::HandRotation;
			});

			EnqueueGoalRotationTest(EControllerHand::Left, EUxtHandConstraintZone::UlnarSide, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));
			EnqueueGoalRotationTest(EControllerHand::Left, EUxtHandConstraintZone::RadialSide, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));
			EnqueueGoalRotationTest(
				EControllerHand::Left, EUxtHandConstraintZone::AboveFingerTips, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));
			EnqueueGoalRotationTest(EControllerHand::Left, EUxtHandConstraintZone::BelowWrist, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));

			EnqueueGoalRotationTest(EControllerHand::Right, EUxtHandConstraintZone::UlnarSide, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));
			EnqueueGoalRotationTest(
				EControllerHand::Right, EUxtHandConstraintZone::RadialSide, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));
			EnqueueGoalRotationTest(
				EControllerHand::Right, EUxtHandConstraintZone::AboveFingerTips, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));
			EnqueueGoalRotationTest(
				EControllerHand::Right, EUxtHandConstraintZone::BelowWrist, FQuat(-0.1195f, 0.6793f, -0.2946f, 0.6614f));

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("goal should have valid rotation in Look-At-Camera mode", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;
				HandConstraint->RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;
			});

			EnqueueGoalRotationTest(EControllerHand::Left, EUxtHandConstraintZone::UlnarSide, FQuat(0.0986f, 0.0202f, 0.9747f, -0.1998f));
			EnqueueGoalRotationTest(EControllerHand::Left, EUxtHandConstraintZone::RadialSide, FQuat(0.1004f, 0.0077f, 0.9920f, -0.0757f));
			EnqueueGoalRotationTest(
				EControllerHand::Left, EUxtHandConstraintZone::AboveFingerTips, FQuat(0.0485f, 0.0068f, 0.9891f, -0.1385f));
			EnqueueGoalRotationTest(EControllerHand::Left, EUxtHandConstraintZone::BelowWrist, FQuat(0.1500f, 0.0210f, 0.9789f, -0.1371f));

			EnqueueGoalRotationTest(EControllerHand::Right, EUxtHandConstraintZone::UlnarSide, FQuat(0.1004f, 0.0077f, 0.9920f, -0.0757f));
			EnqueueGoalRotationTest(EControllerHand::Right, EUxtHandConstraintZone::RadialSide, FQuat(0.0986f, 0.0202f, 0.9747f, -0.1998f));
			EnqueueGoalRotationTest(
				EControllerHand::Right, EUxtHandConstraintZone::AboveFingerTips, FQuat(0.0485f, 0.0068f, 0.9891f, -0.1385f));
			EnqueueGoalRotationTest(EControllerHand::Right, EUxtHandConstraintZone::BelowWrist, FQuat(0.1500f, 0.0210f, 0.9789f, -0.1371f));

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("tracked hand should be left when hand user setting is Left", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);

				// Left is expected default
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Left);

				HandConstraint->Hand = EControllerHand::Left;
			});

			FrameQueue.Enqueue([this]() {
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Left);
				// Left hand is default, no change in tracking should happen
				TestEqual("BeginTracking events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("EndTracking events", HandConstraintListener->NumConstraintDeactivated, 0);
			});

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("tracked hand should be right when hand user setting is Right", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);

				// Left is expected default
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Left);

				HandConstraint->Hand = EControllerHand::Right;
			});

			FrameQueue.Enqueue([this]() {
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Right);
				// Should change from default left hand
				TestEqual("BeginTracking events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("EndTracking events", HandConstraintListener->NumConstraintDeactivated, 0);
			});

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("tracked hand should switch when hand user setting is Any Hand", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);

				TestEqual("Hand", HandConstraint->Hand, EControllerHand::AnyHand);
				// Left is expected default
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Left);

				HandConstraint->Hand = EControllerHand::AnyHand;
			});

			FrameQueue.Enqueue([this]() {
				// Left is expected default when both hands are tracked
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Left);
				// Left hand is default, no change in tracking should happen
				TestEqual("BeginTracking events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("EndTracking events", HandConstraintListener->NumConstraintDeactivated, 0);

				UxtTestUtils::GetTestHandTracker().SetTracked(false);
			});

			FrameQueue.Enqueue([this]() {
				// Disabling tracking should switch to the opposite hand
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Right);
				// Lost tracking should send EndTracking event
				TestEqual("BeginTracking events", HandConstraintListener->NumConstraintActivated, 1);
				TestEqual("EndTracking events", HandConstraintListener->NumConstraintDeactivated, 1);

				UxtTestUtils::GetTestHandTracker().SetTracked(true);
			});

			FrameQueue.Enqueue([this]() {
				// No change when hand tracking is available for both
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Right);
				// Tracking starts again
				TestEqual("BeginTracking events", HandConstraintListener->NumConstraintActivated, 2);
				TestEqual("EndTracking events", HandConstraintListener->NumConstraintDeactivated, 1);

				UxtTestUtils::GetTestHandTracker().SetTracked(false);
			});

			FrameQueue.Enqueue([this]() {
				// And flip again back to the left hand when tracking is lost
				TestEqual("Tracked hand", HandConstraint->GetTrackedHand(), EControllerHand::Left);
				// Lost tracking should send EndTracking event
				TestEqual("BeginTracking events", HandConstraintListener->NumConstraintActivated, 2);
				TestEqual("EndTracking events", HandConstraintListener->NumConstraintDeactivated, 2);
			});

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("should snap actor to goal when smoothing is disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->Zone = EUxtHandConstraintZone::UlnarSide;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;
				HandConstraint->RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;

				// Disable smoothing
				HandConstraint->LocationLerpTime = 0.0f;
				HandConstraint->RotationLerpTime = 0.0f;
			});

			FrameQueue.Enqueue([this]() {
				// Start at goal location
				HandConstraint->GetOwner()->SetActorLocation(HandConstraint->GetGoalLocation());
				HandConstraint->GetOwner()->SetActorRotation(HandConstraint->GetGoalRotation());

				// Move the hand
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(20, -40, 90));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(-10, 40, -20), FVector(0, 20, 30)).ToQuat());
			});
			FrameQueue.Enqueue([this]() {
				TestGoal(FVector(26.0919f, -36.9541f, 90.0000f), FQuat(-0.4660f, 0.2414f, 0.7558f, 0.3916f));
				TestActorDistance(
					FVector(26.0919f, -36.9541f, 90.0000f), FQuat(-0.4660f, 0.2414f, 0.7558f, 0.3916f), KINDA_SMALL_NUMBER,
					KINDA_SMALL_NUMBER);
			});

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("should move actor towards goal when smoothing is enabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->Zone = EUxtHandConstraintZone::UlnarSide;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;
				HandConstraint->RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;

				// Enable smoothing
				HandConstraint->LocationLerpTime = 0.05f;
				HandConstraint->RotationLerpTime = 0.05f;
			});

			FrameQueue.Enqueue([this, Done]() {
				// Start at goal location
				HandConstraint->GetOwner()->SetActorLocation(HandConstraint->GetGoalLocation());
				HandConstraint->GetOwner()->SetActorRotation(HandConstraint->GetGoalRotation());

				// Move the hand
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(20, -40, 90));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(-10, 40, -20), FVector(0, 20, 30)).ToQuat());

				// Delayed distance test, wait for time interval to test smoothing
				// Timer is created from within the frame queue to ensure the previous frame timers have been executed.
				FTimerHandle DummyHandle;
				FTimerDelegate TimerDelegate = FTimerDelegate::CreateWeakLambda(HandConstraint, [this, Done] {
					TestGoal(FVector(26.0919f, -36.9541f, 90.0000f), FQuat(-0.4660f, 0.2414f, 0.7558f, 0.3916f));
					TestActorDistance(FVector(26.0919f, -36.9541f, 90.0000f), FQuat(-0.4660f, 0.2414f, 0.7558f, 0.3916f), 0.1f, 0.05f);
					Done.Execute();
				});
				HandConstraint->GetWorld()->GetTimerManager().SetTimer(DummyHandle, TimerDelegate, 1.0f, false);
			});
		});

		LatentIt("should not move actor when movement is disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->Zone = EUxtHandConstraintZone::UlnarSide;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;
				HandConstraint->RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;

				// Disable actor movement
				HandConstraint->bMoveOwningActor = false;
				// Disable smoothing
				HandConstraint->LocationLerpTime = 0.0f;
				HandConstraint->RotationLerpTime = 0.0f;
			});

			FrameQueue.Enqueue([this]() {
				// Start at goal location
				HandConstraint->GetOwner()->SetActorLocation(HandConstraint->GetGoalLocation());
				HandConstraint->GetOwner()->SetActorRotation(HandConstraint->GetGoalRotation());

				// Move the hand
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(20, -40, 90));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(-10, 40, -20), FVector(0, 20, 30)).ToQuat());
			});
			FrameQueue.Enqueue([this]() {
				TestGoal(FVector(26.0919f, -36.9541f, 90.0000f), FQuat(-0.4660f, 0.2414f, 0.7558f, 0.3916f));
				TestActorDistance(
					FVector(67.4658f, 28.8696f, -15.0000f), FQuat(-0.0986f, -0.0202f, -0.9747f, 0.1998f), KINDA_SMALL_NUMBER,
					KINDA_SMALL_NUMBER);
			});

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});

		LatentIt("should not move actor when goal is invalid", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this]() {
				UxtTestUtils::GetTestHandTracker().SetTracked(true);
				UxtTestUtils::GetTestHandTracker().SetAllJointRadii(1.5f);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(70, 20, -15));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(40, -10, 2), FVector(0, 30, -20)).ToQuat());

				HandConstraint->GoalMargin = 5.0f;
				HandConstraint->Hand = EControllerHand::Left;
				HandConstraint->Zone = EUxtHandConstraintZone::UlnarSide;
				HandConstraint->OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;
				HandConstraint->RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;

				// Disable actor movement
				HandConstraint->bMoveOwningActor = false;
				// Disable smoothing
				HandConstraint->LocationLerpTime = 0.0f;
				HandConstraint->RotationLerpTime = 0.0f;
			});

			FrameQueue.Enqueue([this]() {
				// Start at goal location
				HandConstraint->GetOwner()->SetActorLocation(HandConstraint->GetGoalLocation());
				HandConstraint->GetOwner()->SetActorRotation(HandConstraint->GetGoalRotation());

				// Move the hand
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(20, -40, 90));
				UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(
					FRotationMatrix::MakeFromXY(FVector(-10, 40, -20), FVector(0, 20, 30)).ToQuat());
				// Disable hand tracking, goal should become invalid
				UxtTestUtils::GetTestHandTracker().SetTracked(false);
			});

			FrameQueue.Enqueue([this]() {
				TestFalse("Hand bounds are valid", (bool)HandConstraint->GetHandBounds().IsValid);
				TestFalse("Constraint is active", HandConstraint->IsConstraintActive());

				TestActorDistance(
					FVector(67.4658f, 28.8696f, -15.0000f), FQuat(-0.0986f, -0.0202f, -0.9747f, 0.1998f), KINDA_SMALL_NUMBER,
					KINDA_SMALL_NUMBER);
			});

			FrameQueue.Enqueue([Done]() { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
