// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestTargetComponent.h"
#include "UxtTestUtils.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(
	GrabTargetComponentFocusLockSpec, "UXTools.GrabTargetComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtNearPointerComponent* Pointer;
UTestGrabTarget* Target;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(GrabTargetComponentFocusLockSpec)

void GrabTargetComponentFocusLockSpec::Define()
{
	Describe("Focus Lock", [this] {
		BeforeEach([this] {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			UxtTestUtils::EnableTestHandTracker();

			FVector Center(150, 0, 0);
			Pointer = UxtTestUtils::CreateNearPointer(World, TEXT("GrabTestPointer"), Center + FVector(-15, 0, 0));

			const FString& TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
			const float TargetScale = 0.3f;
			Target = UxtTestUtils::CreateNearPointerGrabTarget(World, Center, TargetFilename, TargetScale);
			// Enable focus lock when the target is grabbed
			Target->bUseFocusLock = true;

			// Register all new components.
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();
			Pointer->GetOwner()->Destroy();
			Pointer = nullptr;
			Target->GetOwner()->Destroy();
			Target = nullptr;
		});

		LatentIt("should be released when target is deleted", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				// Enable grab.
				// Wait one tick so the pointer can update overlaps and raise events.
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				// Check that focus lock is enabled.
				TestTrue(TEXT("Focus lock enabled"), Pointer->GetFocusLocked());

				// Delete the target actor.
				// Wait one tick so the pointer can update and release focus lock.
				Target->GetOwner()->Destroy();
			});

			FrameQueue.Enqueue([this, Done] {
				// Ensure focus lock has been disabled.
				TestFalse(TEXT("Focus lock disabled"), Pointer->GetFocusLocked());

				Done.Execute();
			});
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
