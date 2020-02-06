#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include <vector>

#include "UXToolsTestUtils.h"
#include "Input/UxtTouchPointer.h"
#include "TouchPointerAnimUtils.h"

using namespace TouchPointerAnimUtils;

static float KeyframeDuration = 0.2f;

const FString TestCase_HoverEnterExit = TEXT("HoverEnterExit");
const FString TestCase_GraspTarget = TEXT("GraspTarget");
const FString TestCase_GraspBackground = TEXT("GraspBackground");
const FString TestCase_HoverLockTarget = TEXT("HoverLockTarget");
const FString TestCase_HoverLockBackground = TEXT("HoverLockBackground");

/** Creates a number of target actors as well as a list of keyframes that the pointer should pass through. */
static void SetupTestCase(UWorld* world, const FString& TestCase, TouchAnimSequence& OutSequence)
{
	const FVector pTarget(120, -20, -5);
	const FVector pInside(113, -24, -8);
	const FVector pOutside(150, 40, -40);

	if (TestCase == TestCase_HoverEnterExit)
	{
		OutSequence.AddBackgroundTarget(world);
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectHoverTargetIndex(0, false);
		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectHoverTargetIndex(1);
		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectHoverTargetIndex(0);
		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectHoverTargetIndex(1);
		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectHoverTargetIndex(0);
	}

	if (TestCase == TestCase_GraspTarget)
	{
		OutSequence.AddBackgroundTarget(world);
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectHoverTargetIndex(1);

		OutSequence.AddGraspKeyframe(true);

		OutSequence.AddGraspKeyframe(false);
	}

	if (TestCase == TestCase_GraspBackground)
	{
		OutSequence.AddBackgroundTarget(world);
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectHoverTargetIndex(0, false);

		OutSequence.AddGraspKeyframe(true);

		OutSequence.AddGraspKeyframe(false);
	}

	if (TestCase == TestCase_HoverLockTarget)
	{
		OutSequence.AddBackgroundTarget(world);
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectHoverTargetIndex(0, false);

		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectHoverTargetIndex(1);

		OutSequence.AddGraspKeyframe(true);

		OutSequence.AddMovementKeyframe(pOutside);

		OutSequence.AddGraspKeyframe(false);
		OutSequence.ExpectHoverTargetIndex(0);
	}

	if (TestCase == TestCase_HoverLockBackground)
	{
		OutSequence.AddBackgroundTarget(world);
		OutSequence.AddTarget(world, pTarget);

		OutSequence.AddMovementKeyframe(pInside);
		OutSequence.ExpectHoverTargetIndex(1);

		OutSequence.AddMovementKeyframe(pOutside);
		OutSequence.ExpectHoverTargetIndex(0);

		OutSequence.AddGraspKeyframe(true);

		OutSequence.AddMovementKeyframe(pInside);

		OutSequence.AddGraspKeyframe(false);
		OutSequence.ExpectHoverTargetIndex(1);
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

	AddTestCase(TestCase_HoverEnterExit);
	AddTestCase(TestCase_GraspTarget);
	AddTestCase(TestCase_GraspBackground);
	AddTestCase(TestCase_GraspBackground);
	AddTestCase(TestCase_HoverLockBackground);
}

bool FBackgroundTouchTargetTest::RunTest(const FString& Parameters)
{
	FString TestCase = Parameters;

	// Load the empty test map to run the test in.
	AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	UWorld *world = UXToolsTestUtils::GetTestWorld();

	TouchAnimSequence sequence;

	// Create pointers.
	sequence.CreatePointers(world, 1);

	// Construct target actors.
	SetupTestCase(world, TestCase, sequence);

	// Register all new components.
	world->UpdateWorldComponents(false, false);

	sequence.RunInterpolatedPointersTest(this, KeyframeDuration);

	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}
