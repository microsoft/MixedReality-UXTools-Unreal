#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include <vector>

#include "MixedRealityTestUtils.h"
#include "TouchPointer.h"
#include "TestTouchPointerTarget.h"

static float KeyframeDuration = 0.2f;

/** Defines how the pointer positions are modified during the test. */
enum EPointerMovement
{
	/** Interpolate the position over multiple frames. */
	Interpolate,
	/** Set position directly within a single frame. */
	Teleport,
};

/** Defines a test setup for touch pointer targets to examine different overlap situations */
enum ETargetSetup
{
	/** No target. */
	None,
	/** Single target. */
	Single,
	/** Two targets with separated colliders. */
	TwoSeparate,
	/** Two targets whose colliders overlap. */
	TwoOverlapping,
};

//
// Enum-FString conversion functions

static FString PointerMovementToString(EPointerMovement PointerMovement)
{
	switch (PointerMovement)
	{
		case EPointerMovement::Interpolate:		return TEXT("Interpolate");
		case EPointerMovement::Teleport:		return TEXT("Teleport");
		default:								return TEXT("");
	}
}

static EPointerMovement PointerMovementFromString(const FString &Name)
{
	if (Name == TEXT("Interpolate"))			return EPointerMovement::Interpolate;
	else if (Name == TEXT("Teleport"))			return EPointerMovement::Teleport;
	else										return EPointerMovement::Interpolate;
}

static FString TargetSetupToString(ETargetSetup TargetSetup)
{
	switch (TargetSetup)
	{
		case ETargetSetup::None:			return TEXT("None");
		case ETargetSetup::Single:			return TEXT("Single");
		case ETargetSetup::TwoSeparate:		return TEXT("TwoSeparate");
		case ETargetSetup::TwoOverlapping:	return TEXT("TwoOverlapping");
		default:							return TEXT("");
	}
}

static ETargetSetup TargetSetupFromString(const FString &Name)
{
	if (Name == TEXT("None"))					return ETargetSetup::None;
	else if (Name == TEXT("Single"))			return ETargetSetup::Single;
	else if (Name == TEXT("TwoSeparate"))		return ETargetSetup::TwoSeparate;
	else if (Name == TEXT("TwoOverlapping"))	return ETargetSetup::TwoOverlapping;
	else										return ETargetSetup::None;
}

/** Position in space for moving the pointer, with a list of expected enter/exit event counts for each target. */
struct PointerKeyframe
{
	FVector Location;
	std::vector<int> TouchStartCountExpected;
	std::vector<int> TouchEndCountExpected;
};

/** Creates a number of target actors as well as a list of keyframes that the pointer should pass through. */
static void SetupTargets(UWorld *world, ETargetSetup TargetSetup, std::vector<UTestTouchPointerTarget*> &OutTargets, std::vector<PointerKeyframe> &OutPointerKeyframes)
{
	// Util function for adding a new target.
	auto AddTarget = [&OutTargets, world](const FVector &pos)
	{
		const FString &targetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
		const float targetScale = 0.3f;
		OutTargets.emplace_back(MixedRealityTestUtils::CreateTouchPointerTarget(world, pos, targetFilename, targetScale));
	};

	// Util function for adding a new keyframe.
	// Enter/Exit events must be incremented separately based on expected behavior.
	auto AddKeyframe = [&OutPointerKeyframes, &OutTargets](const FVector &pos)
	{
		PointerKeyframe keyframe{ pos };
		if (OutPointerKeyframes.empty())
		{
			keyframe.TouchStartCountExpected.resize(OutTargets.size(), 0);
			keyframe.TouchEndCountExpected.resize(OutTargets.size(), 0);
		}
		else
		{
			keyframe.TouchStartCountExpected = OutPointerKeyframes.back().TouchStartCountExpected;
			keyframe.TouchEndCountExpected = OutPointerKeyframes.back().TouchEndCountExpected;
		}

		OutPointerKeyframes.emplace_back(keyframe);
	};

	// Util function for incrementing expected enter events.
	auto EnterTarget = [&OutPointerKeyframes](int targetIndex)
	{
		++OutPointerKeyframes.back().TouchStartCountExpected[targetIndex];
	};

	// Util function for incrementing expected exit events.
	auto ExitTarget = [&OutPointerKeyframes](int targetIndex)
	{
		++OutPointerKeyframes.back().TouchEndCountExpected[targetIndex];
	};

	const FVector pStart(40, -50, 30);
	const FVector pEnd(50, 40, -40);
	switch (TargetSetup)
	{
		default:
		case ETargetSetup::None:
			break;

		case ETargetSetup::Single:
		{
			FVector p1(120, -20, -5);
			AddTarget(p1);

			AddKeyframe(pStart);
			AddKeyframe(p1);
			EnterTarget(0);
			AddKeyframe(pEnd);
			ExitTarget(0);
			break;
		}

		case ETargetSetup::TwoSeparate:
		{
			FVector p1(120, -40, -5);
			FVector p2(100, 30, 15);
			AddTarget(p1);
			AddTarget(p2);

			AddKeyframe(pStart);
			AddKeyframe(p1);
			EnterTarget(0);
			AddKeyframe(p2);
			ExitTarget(0);
			EnterTarget(1);
			AddKeyframe(pEnd);
			ExitTarget(1);
			break;
		}

		case ETargetSetup::TwoOverlapping:
		{
			FVector p1(110, 4, -5);
			FVector p2(115, 12, -2);
			AddTarget(p1);
			AddTarget(p2);

			AddKeyframe(pStart);
			AddKeyframe(p1);
			EnterTarget(0);
			EnterTarget(1);
			AddKeyframe(p2);
			AddKeyframe(pEnd);
			ExitTarget(0);
			ExitTarget(1);
			break;
		}
	}
}


/** Test enter/exit events for all targets at the given keyframe position. */
static void TestKeyframe(FAutomationTestBase *Test, const std::vector<UTouchPointer*> &Pointers, const std::vector<UTestTouchPointerTarget*> &Targets, const PointerKeyframe &keyframe)
{
	for (int targetIndex = 0; targetIndex < Targets.size(); ++targetIndex)
	{
		const UTestTouchPointerTarget *target = Targets[targetIndex];
		int expectedTouchStart = keyframe.TouchStartCountExpected[targetIndex] * Pointers.size();
		int expectedTouchEnd = keyframe.TouchEndCountExpected[targetIndex] * Pointers.size();

		FString whatStarted; whatStarted.Appendf(TEXT("Target %d TouchStarted count"), targetIndex);
		FString whatEnded; whatEnded.Appendf(TEXT("Target %d TouchEnded count"), targetIndex);
		Test->TestEqual(whatStarted, target->TouchStartedCount, expectedTouchStart);
		Test->TestEqual(whatEnded, target->TouchEndedCount, expectedTouchEnd);
	}
}


/**
 * Latent command that interpolates the pointers over multiple frames.
 * Tests for correctness at each keyframe position.
 */
class FAnimatePointersCommand : public IAutomationLatentCommand
{
public:
	FAnimatePointersCommand(
		FAutomationTestBase *Test,
		const std::vector<UTouchPointer*> &Pointers,
		const std::vector<UTestTouchPointerTarget*> &Targets,
		const std::vector<PointerKeyframe> &Keyframes,
		float Duration = 1.0f)
		: Test(Test)
		, Pointers(Pointers)
		, Targets(Targets)
		, Keyframes(Keyframes)
		, Duration(Duration)
		, Section(-1)
	{}

	virtual ~FAnimatePointersCommand()
	{}

	virtual bool Update() override
	{
		if (Pointers.empty())
		{
			return true;
		}
		
		const int NumSections = (int)(Keyframes.size() - 1);
		float Elapsed = FPlatformTime::Seconds() - StartTime;
		float AnimTime = Elapsed / Duration;
		int NewSection = FMath::Clamp((int)AnimTime, 0, NumSections);

		if (NewSection != Section)
		{
			Section = NewSection;

			TestKeyframe(Test, Pointers, Targets, Keyframes[Section]);
		}

		if (Section < Keyframes.size() - 1)
		{
			float Alpha = AnimTime - (float)Section;
			const PointerKeyframe &start = Keyframes[Section];
			const PointerKeyframe &end = Keyframes[Section + 1];
			FVector location = FMath::Lerp(start.Location, end.Location, Alpha);

			for (UTouchPointer *pointer : Pointers)
			{
				pointer->GetOwner()->SetActorLocation(location);
			}
		}
		
		return Elapsed >= Duration * NumSections;
	}

private:

	FAutomationTestBase *Test;

	const std::vector<UTouchPointer*> Pointers;
	const std::vector<UTestTouchPointerTarget*> Targets;
	const std::vector<PointerKeyframe> Keyframes;

	/** Time in seconds between each pair of keyframes. */
	float Duration;

	/** Current section of the keyframe list that is being interpolated. */
	int Section;
};


IMPLEMENT_COMPLEX_AUTOMATION_TEST(FTouchPointerTest, "MixedRealityUtils.TouchPointer",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

void FTouchPointerTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	// Util for adding a test combination.
	auto AddTestCase = [&OutBeautifiedNames, &OutTestCommands](int NumPointers, EPointerMovement PointerMovement, ETargetSetup TargetSetup)
	{
		FString sTargetSetup = TargetSetupToString(TargetSetup);
		FString sPointerMovement = PointerMovementToString(PointerMovement);
		FString name;
		name.Appendf(TEXT("TouchPointerTest_%d_%s_%s"), NumPointers, *sPointerMovement, *sTargetSetup);
		FString command;
		command.Appendf(TEXT("%d %s %s"), NumPointers, *sPointerMovement, *sTargetSetup);

		OutBeautifiedNames.Add(name);
		OutTestCommands.Add(command);
	};

	// No pointers (sanity check)
	AddTestCase(0, EPointerMovement::Interpolate, ETargetSetup::Single);

	// Single target
	AddTestCase(1, EPointerMovement::Interpolate, ETargetSetup::Single);
	AddTestCase(2, EPointerMovement::Interpolate, ETargetSetup::Single);
	AddTestCase(1, EPointerMovement::Teleport, ETargetSetup::Single);
	AddTestCase(2, EPointerMovement::Teleport, ETargetSetup::Single);

	// Two separate targets
	AddTestCase(1, EPointerMovement::Interpolate, ETargetSetup::TwoSeparate);
	AddTestCase(2, EPointerMovement::Interpolate, ETargetSetup::TwoSeparate);
	AddTestCase(1, EPointerMovement::Teleport, ETargetSetup::TwoSeparate);
	AddTestCase(2, EPointerMovement::Teleport, ETargetSetup::TwoSeparate);

	// Two overlapping targets
	AddTestCase(1, EPointerMovement::Interpolate, ETargetSetup::TwoOverlapping);
	AddTestCase(2, EPointerMovement::Interpolate, ETargetSetup::TwoOverlapping);
	AddTestCase(1, EPointerMovement::Teleport, ETargetSetup::TwoOverlapping);
	AddTestCase(2, EPointerMovement::Teleport, ETargetSetup::TwoOverlapping);
}

/** Util for parsing a parameter string into test settings. */
static bool ParseTestCase(const FString& Parameters, int &OutNumPointers, EPointerMovement &OutPointerMovement, ETargetSetup &OutTargetSetup)
{
	FString sNumPointers, sTargetSetup, sPointerMovement;
	FString sSplit = Parameters;
	sSplit.Split(TEXT(" "), &sNumPointers, &sSplit);
	sSplit.Split(TEXT(" "), &sPointerMovement, &sTargetSetup);

	OutNumPointers = FCString::Atoi(*sNumPointers);
	OutPointerMovement = PointerMovementFromString(sPointerMovement);
	OutTargetSetup = TargetSetupFromString(sTargetSetup);

	return true;
}

bool FTouchPointerTest::RunTest(const FString& Parameters)
{
	int NumPointers;
	ETargetSetup TargetSetup;
	EPointerMovement PointerMovement;
	if (!ParseTestCase(Parameters, NumPointers, PointerMovement, TargetSetup))
	{
		return false;
	}

	// Load the empty test map to run the test in.
	AutomationOpenMap(TEXT("/Game/Tests/Maps/TestEmpty"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	UWorld *world = MixedRealityTestUtils::GetTestWorld();

	// Construct target actors.
	std::vector<UTestTouchPointerTarget*> targets;
	std::vector<PointerKeyframe> pointerKeyframes;
	SetupTargets(world, TargetSetup, targets, pointerKeyframes);

	// Create pointers.
	std::vector<UTouchPointer*> pointers(NumPointers);
	for (int i = 0; i < NumPointers; ++i)
	{
		pointers[i] = MixedRealityTestUtils::CreateTouchPointer(world, FVector::ZeroVector);
	}

	// Register all new components.
	world->UpdateWorldComponents(false, false);

	switch (PointerMovement)
	{
		case EPointerMovement::Interpolate:
			ADD_LATENT_AUTOMATION_COMMAND(FAnimatePointersCommand(this, pointers, targets, pointerKeyframes, KeyframeDuration));
			break;
		case EPointerMovement::Teleport:
			for (int Section = 0; Section < pointerKeyframes.size(); ++Section)
			{
				const PointerKeyframe &keyframe = pointerKeyframes[Section];

				for (UTouchPointer *pointer : pointers)
				{
					pointer->GetOwner()->SetActorLocation(keyframe.Location);
				}

				TestKeyframe(this, pointers, targets, keyframe);
			}
			break;
	}

	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}
