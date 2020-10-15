// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestTargetComponent.h"
#include "UxtTestUtils.h"

#include "Controls/UxtFarBeamComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FFarBeamSpec, "UXTools.FarBeam", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

FFrameQueue FrameQueue;
UUxtFarBeamComponent* Beam;
AUxtHandInteractionActor* HandInteractionActor;
UTestGrabTarget* Target;
const FVector NearPoint = FVector(135, 0, 0);
const FString TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
const FVector TargetLocation = FVector(150, 0, 0);
const float TargetScale = 0.3f;

END_DEFINE_SPEC(FFarBeamSpec)

void FFarBeamSpec::Define()
{
	Describe("Far Beam", [this] {
		BeforeEach([this] {
			// Load the empty test map to run the test in.
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);
			UxtTestUtils::EnableTestHandTracker();

			// handinteractionactor
			HandInteractionActor = World->SpawnActor<AUxtHandInteractionActor>();
			HandInteractionActor->SetHand(EControllerHand::Left);
			TArray<UUxtFarBeamComponent*> FarBeamComponent;
			HandInteractionActor->GetComponents<UUxtFarBeamComponent>(FarBeamComponent);
			Beam = FarBeamComponent.Num() > 0 ? FarBeamComponent[0] : NULL;

			Target = UxtTestUtils::CreateNearPointerGrabTarget(World, TargetLocation, TargetFilename, TargetScale);
			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			FrameQueue.Reset();
			UxtTestUtils::DisableTestHandTracker();
			FrameQueue.Reset();
			HandInteractionActor->Destroy();
			HandInteractionActor = NULL;
			Target->GetOwner()->Destroy();
			Target = NULL;
			// Force GC so that destroyed actors are removed from the world.
			// Running multiple tests will otherwise cause errors when creating duplicate actors.
			GEngine->ForceGarbageCollection();
		});

		LatentIt("Test FarBeam activation", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint); });
			FrameQueue.Skip();

			FrameQueue.Enqueue([this] {
				TestTrue(TEXT("Beam is not null"), Beam != NULL);
				if (Beam)
				{
					TestTrue(TEXT("Beam is not ticking"), !Beam->IsComponentTickEnabled());
					TestTrue(TEXT("Beam is invisible"), !Beam->IsVisible());
				}
			});
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector); });
			FrameQueue.Skip();
			FrameQueue.Skip(); // extra skip to avoid sweep resulting in hit on last frame
			FrameQueue.Enqueue([this] {
				TestTrue(TEXT("Beam is ticking"), Beam->IsComponentTickEnabled());
				TestTrue(TEXT("Beam is visible"), Beam->IsVisible());
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // #if WITH_DEV_AUTOMATION_TESTS
