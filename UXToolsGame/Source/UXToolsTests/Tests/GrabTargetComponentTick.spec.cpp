// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "GrabTickTestComponent.h"
#include "UxtTestHandTracker.h"
#include "UxtTestTargetComponent.h"
#include "UxtTestUtils.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

static UGrabTickTestComponent* CreateTestComponent(UWorld* World, const FVector& Location)
{
	AActor* actor = World->SpawnActor<AActor>();

	USceneComponent* root = NewObject<USceneComponent>(actor);
	actor->SetRootComponent(root);
	root->SetWorldLocation(Location);
	root->RegisterComponent();

	UGrabTickTestComponent* testTarget = NewObject<UGrabTickTestComponent>(actor);
	testTarget->RegisterComponent();

	UStaticMeshComponent* mesh = UxtTestUtils::CreateBoxStaticMesh(actor, FVector(0.3f));
	mesh->SetupAttachment(actor->GetRootComponent());
	mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	mesh->SetCollisionProfileName(TEXT("OverlapAll"));
	mesh->SetGenerateOverlapEvents(true);
	mesh->RegisterComponent();

	return testTarget;
}

BEGIN_DEFINE_SPEC(
	GrabTargetComponentTickSpec, "UXTools.GrabTargetComponent",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext)

// Enqueue frames for testing expected tick behavior of the target component.
void SetupFrames(const FDoneDelegate& Done, bool bEnableGrabbing, bool bExpectTicking)
{
	// Update the hand grasp state.
	// Tick once so the pointer can query overlaps and raise events.
	FrameQueue.Enqueue([this, bEnableGrabbing] { UxtTestUtils::GetTestHandTracker().SetGrabbing(bEnableGrabbing); });
	FrameQueue.Enqueue([this] {});

	if (bEnableGrabbing)
	{
		// Wait for one more tick, because the pointer component is updated post-physics,
		// so the grab component won't be ticked until the next frame.
		FrameQueue.Enqueue([this] { TestTrue(TEXT("Target component grabbed"), Target->GetGrabPointers().Num() > 0); });
	}

	// Examine observed tick behavior of the target component.
	FrameQueue.Enqueue([this, Done, bExpectTicking] {
		bool bWasTicked = Target->GetNumTicks() > 0;
		TestEqual(TEXT("Grabbable component ticked"), bExpectTicking, bWasTicked);

		Done.Execute();
	});
}

UUxtNearPointerComponent* Pointer;
UGrabTickTestComponent* Target;
FFrameQueue FrameQueue;

END_DEFINE_SPEC(GrabTargetComponentTickSpec)

void GrabTargetComponentTickSpec::Define()
{
	Describe("Grab component", [this] {
		BeforeEach([this]() {
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);

			UxtTestUtils::EnableTestHandTracker();

			FVector Center(150, 0, 0);
			Pointer = UxtTestUtils::CreateNearPointer(World, TEXT("GrabTestPointer"), Center + FVector(-15, 0, 0));
			Target = CreateTestComponent(World, Center);

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

		LatentIt("should never tick when disabled", [this](const FDoneDelegate& Done) {
			Target->SetComponentTickEnabled(false);

			SetupFrames(Done, false, false);
		});

		LatentIt("should always tick with TickOnlyWhileGrabbed disabled (not grabbed)", [this](const FDoneDelegate& Done) {
			Target->SetTickOnlyWhileGrabbed(false);
			TestTrue(
				TEXT("Component tick should be enabled after disabling TickOnlyWhileGrabbed"),
				Target->PrimaryComponentTick.IsTickFunctionEnabled());

			SetupFrames(Done, false, true);
		});
		LatentIt("should always tick with TickOnlyWhileGrabbed disabled (grabbed)", [this](const FDoneDelegate& Done) {
			Target->SetTickOnlyWhileGrabbed(false);
			TestTrue(
				TEXT("Component tick should be enabled after disabling TickOnlyWhileGrabbed"),
				Target->PrimaryComponentTick.IsTickFunctionEnabled());

			SetupFrames(Done, true, true);
		});

		LatentIt("should not tick if not grabbed and TickOnlyWhileGrabbed enabled", [this](const FDoneDelegate& Done) {
			Target->SetTickOnlyWhileGrabbed(true);
			TestTrue(TEXT("Component tick should be disabled when not grabbed"), !Target->PrimaryComponentTick.IsTickFunctionEnabled());

			SetupFrames(Done, false, false);
		});
		LatentIt("should tick while grabbed and TickOnlyWhileGrabbed enabled", [this](const FDoneDelegate& Done) {
			Target->SetTickOnlyWhileGrabbed(true);
			TestTrue(TEXT("Component tick should be disabled when not grabbed"), !Target->PrimaryComponentTick.IsTickFunctionEnabled());

			SetupFrames(Done, true, true);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
