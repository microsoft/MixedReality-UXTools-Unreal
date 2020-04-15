// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "PointerTestSequence.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

using namespace UxtPointerTests;

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(PointerActivationSpec, "UXTools.HandPointers", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

	AUxtHandInteractionActor* HandActor;
	UTestGrabTarget* Target;
	FFrameQueue FrameQueue;

	const FString TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
	const float TargetScale = 0.3f;
	const FVector TargetLocation = FVector(150, 0, 0);
	const FVector NearPoint = FVector(135, 0, 0);
	const FVector FarPoint = FVector(40, -50, 30);

END_DEFINE_SPEC(PointerActivationSpec)

void PointerActivationSpec::Define()
{
	Describe("Hand Interaction Actor", [this]
		{
			BeforeEach([this]
				{
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();

					UxtTestUtils::EnableTestHandTracker();

					HandActor = World->SpawnActor<AUxtHandInteractionActor>();
					HandActor->SetHand(EControllerHand::Left);

					Target = UxtTestUtils::CreateNearPointerTarget(World, TargetLocation, TargetFilename, TargetScale);

					// Register all new components.
					World->UpdateWorldComponents(false, false);

					FrameQueue.Init(&World->GetTimerManager());
				});

			AfterEach([this]
				{
					FrameQueue.Reset();

					HandActor->Destroy();
					HandActor = nullptr;
					Target->GetOwner()->Destroy();
					Target = nullptr;

					UxtTestUtils::DisableTestHandTracker();

					// Force GC so that destroyed actors are removed from the world.
					// Running multiple tests will otherwise cause errors when creating duplicate actors.
					GEngine->ForceGarbageCollection();
				});

			LatentIt("should activate near pointer if close to a target", [this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().TestPosition = NearPoint;
						});
					FrameQueue.Enqueue([this] {});

					FrameQueue.Enqueue([this, Done]
						{
							auto NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
							auto FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();

							FVector ClosestPoint;
							TestEqual(TEXT("Near pointer focusing target"), NearPointer->GetFocusedGrabTarget(ClosestPoint), (UObject*)Target);
							TestTrue(TEXT("Near pointer active"), NearPointer->IsActive());
							TestFalse(TEXT("Far pointer active"), FarPointer->IsActive());

							Done.Execute();
						});
				});

			LatentIt("should activate far pointer if not close to a target", [this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().TestPosition = FarPoint;
						});

					FrameQueue.Enqueue([this, Done]
						{
							auto NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
							auto FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();

							FVector ClosestPoint;
							TestNull(TEXT("Near pointer not focused"), NearPointer->GetFocusedGrabTarget(ClosestPoint));
							TestFalse(TEXT("Near pointer active"), NearPointer->IsActive());
							TestTrue(TEXT("Far pointer active"), FarPointer->IsActive());

							Done.Execute();
						});
				});

			LatentIt("should deactivate pointers when tracking is lost", [this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this]
						{
							auto NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
							auto FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();

							// Enable focus lock to ensure it is released when tracking is lost.
							// Have to activate the pointers as well to test that focus lock is released during deactivation.
							NearPointer->Activate();
							FarPointer->Activate();
							NearPointer->SetFocusLocked(true);
							FarPointer->SetFocusLocked(true);

							UxtTestUtils::GetTestHandTracker().TestPosition = NearPoint;
							UxtTestUtils::GetTestHandTracker().bIsTracked = false;
						});

					FrameQueue.Enqueue([this, Done]
						{
							auto NearPointer = HandActor->FindComponentByClass<UUxtNearPointerComponent>();
							auto FarPointer = HandActor->FindComponentByClass<UUxtFarPointerComponent>();

							FVector ClosestPoint;
							TestNull(TEXT("Near pointer not focused"), NearPointer->GetFocusedGrabTarget(ClosestPoint));
							TestFalse(TEXT("Near pointer active"), NearPointer->IsActive());
							TestFalse(TEXT("Far pointer active"), FarPointer->IsActive());

							TestFalse(TEXT("Near pointer focus locked"), NearPointer->GetFocusLocked());
							TestFalse(TEXT("Far pointer focus locked"), FarPointer->GetFocusLocked());

							Done.Execute();
						});
				});
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
