// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

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

/** No target. */
const FString TargetSetup_None = TEXT("None");
/** Single target. */
const FString TargetSetup_Single = TEXT("Single");
/** Two targets with separated colliders. */
const FString TargetSetup_TwoSeparate = TEXT("TwoSeparate");
/** Two targets whose colliders overlap. */
const FString TargetSetup_TwoOverlapping = TEXT("TwoOverlapping");

/** Creates a number of target actors as well as a list of keyframes that the pointer should pass through. */
static void SetupTargets(UWorld *world, const FString& TargetSetup, PointerTestSequence& OutSequence, int NumPointers)
{
	const FVector pStart(40, -50, 30);
	const FVector pEnd(150, 40, -40);

	if (TargetSetup == TargetSetup_Single)
	{
		// Nothing to do
	}

	if (TargetSetup == TargetSetup_Single)
	{
		FVector p1(120, -20, -5);
		OutSequence.AddTarget(world, p1);

		OutSequence.AddMovementKeyframe(pStart);
		OutSequence.ExpectFocusTargetNone();
		OutSequence.AddMovementKeyframe(p1);
		OutSequence.ExpectFocusTargetIndex(0);
		OutSequence.AddMovementKeyframe(pEnd);
		OutSequence.ExpectFocusTargetNone();
	}

	if (TargetSetup == TargetSetup_TwoSeparate)
	{
		FVector p1(120, -40, -5);
		FVector p2(100, 30, 15);
		OutSequence.AddTarget(world, p1);
		OutSequence.AddTarget(world, p2);

		OutSequence.AddMovementKeyframe(pStart);
		OutSequence.ExpectFocusTargetNone();
		OutSequence.AddMovementKeyframe(p1);
		OutSequence.ExpectFocusTargetIndex(0);
		OutSequence.AddMovementKeyframe(p2);
		OutSequence.ExpectFocusTargetIndex(1);
		OutSequence.AddMovementKeyframe(pEnd);
		OutSequence.ExpectFocusTargetNone();
	}

	if (TargetSetup == TargetSetup_TwoOverlapping)
	{
		FVector p1(110, 4, -5);
		FVector p2(115, 12, -2);
		OutSequence.AddTarget(world, p1);
		OutSequence.AddTarget(world, p2);

		OutSequence.AddMovementKeyframe(pStart);
		OutSequence.ExpectFocusTargetNone();
		OutSequence.AddMovementKeyframe(p1);
		OutSequence.ExpectFocusTargetIndex(0);
		OutSequence.AddMovementKeyframe(p2);
		OutSequence.ExpectFocusTargetIndex(1);
		OutSequence.AddMovementKeyframe(pEnd);
		OutSequence.ExpectFocusTargetNone();
	}
}


IMPLEMENT_COMPLEX_AUTOMATION_TEST(FNearPointerTest, "UXTools.NearPointer",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

void FNearPointerTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	// Util for adding a test combination.
	auto AddTestCase = [&OutBeautifiedNames, &OutTestCommands](int NumPointers, const FString& TargetSetup)
	{
		FString name;
		name.Appendf(TEXT("NearPointerTest_%d_%s"), NumPointers, *TargetSetup);
		FString command;
		command.Appendf(TEXT("%d %s"), NumPointers, *TargetSetup);

		OutBeautifiedNames.Add(name);
		OutTestCommands.Add(command);
	};

	// No pointers (sanity check)
	AddTestCase(0, TargetSetup_Single);

	// Single target
	AddTestCase(1, TargetSetup_Single);
	AddTestCase(2, TargetSetup_Single);

	// Two separate targets
	AddTestCase(1, TargetSetup_TwoSeparate);
	AddTestCase(2, TargetSetup_TwoSeparate);

	// Two overlapping targets
	AddTestCase(1, TargetSetup_TwoOverlapping);
	AddTestCase(2, TargetSetup_TwoOverlapping);
}

/** Util for parsing a parameter string into test settings. */
static bool ParseTestCase(const FString& Parameters, int& OutNumPointers, FString& OutTargetSetup)
{
	FString sNumPointers;
	FString sSplit = Parameters;
	sSplit.Split(TEXT(" "), &sNumPointers, &OutTargetSetup);

	OutNumPointers = FCString::Atoi(*sNumPointers);

	return true;
}

bool FNearPointerTest::RunTest(const FString& Parameters)
{
	int NumPointers;
	FString TargetSetup;
	if (!ParseTestCase(Parameters, NumPointers, TargetSetup))
	{
		return false;
	}

	// Load the empty test map to run the test in.
	AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	UWorld *world = UxtTestUtils::GetTestWorld();

	UxtTestUtils::EnableTestHandTracker();

	PointerTestSequence sequence;


	// Create pointers.
	sequence.CreatePointers(world, NumPointers);

	// Construct target actors.
	SetupTargets(world, TargetSetup, sequence, NumPointers);

	// Register all new components.
	world->UpdateWorldComponents(false, false);

	sequence.EnqueueTestSequence(this);

	ADD_LATENT_AUTOMATION_COMMAND(FUxtDisableTestHandTrackerCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}

