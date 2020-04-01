#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "UxtTestUtils.h"
#include "PointerTestSequence.h"
#include "UxtTestHandTracker.h"

using namespace UxtPointerTests;

const FString TestCase_FocusEnterExit = TEXT("FocusEnterExit");
const FString TestCase_GraspTarget = TEXT("GraspTarget");
const FString TestCase_FocusLockTarget = TEXT("FocusLockTarget");

/** Creates a number of target actors as well as a list of keyframes that the pointer should pass through. */
static void SetupTestCase(UWorld* world, const FString& TestCase, PointerTestSequence& OutSequence)
{
	const FVector pTarget(120, -20, -5);
	const FVector pInside(113, -24, -8);
	const FVector pOutside(150, 40, -40);

	if (TestCase == TestCase_FocusEnterExit)
	{
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectFocusTargetNone();
		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectFocusTargetIndex(0);
		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectFocusTargetNone();
		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectFocusTargetIndex(0);
		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectFocusTargetNone();
	}

	if (TestCase == TestCase_GraspTarget)
	{
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectFocusTargetIndex(0);

		OutSequence.AddGrabKeyframe(true);

		OutSequence.AddGrabKeyframe(false);
	}

	if (TestCase == TestCase_FocusLockTarget)
	{
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectFocusTargetNone();

		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectFocusTargetIndex(0);

		OutSequence.AddGrabKeyframe(true);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectFocusTargetNone();

		OutSequence.AddGrabKeyframe(false);
	}
}


IMPLEMENT_COMPLEX_AUTOMATION_TEST(FBackgroundTouchTargetTest, "UXTools.BackgroundTouchTarget",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

void FBackgroundTouchTargetTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	// Util for adding a test combination.
	auto AddTestCase = [&OutBeautifiedNames, &OutTestCommands](const FString& TestCase)
	{
		OutBeautifiedNames.Add(TestCase);
		OutTestCommands.Add(TestCase);
	};

	AddTestCase(TestCase_FocusEnterExit);
	AddTestCase(TestCase_GraspTarget);
}

bool FBackgroundTouchTargetTest::RunTest(const FString& Parameters)
{
	FString TestCase = Parameters;

	// Load the empty test map to run the test in.
	AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	UWorld *world = UxtTestUtils::GetTestWorld();

	UxtTestUtils::EnableTestHandTracker();

	PointerTestSequence sequence;

	// Create pointers.
	sequence.CreatePointers(world, 1);

	// Construct target actors.
	SetupTestCase(world, TestCase, sequence);

	// Register all new components.
	world->UpdateWorldComponents(false, false);

	sequence.EnqueueTestSequence(this);

	ADD_LATENT_AUTOMATION_COMMAND(FUxtDisableTestHandTrackerCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}
