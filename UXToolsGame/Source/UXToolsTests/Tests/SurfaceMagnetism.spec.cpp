// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtFarBeamComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "Controls/UxtSurfaceMagnetismComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
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

BEGIN_DEFINE_SPEC(FSurfaceMagnetism, "UXTools.SurfaceMagnetism", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

FFrameQueue FrameQueue;
AUxtHandInteractionActor* HandInteractionActor;
const FString TargetFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
const FVector NearPoint = FVector(110, 0, 0);
const FVector ActorLocation = FVector(0, 0, 0);
const FVector TargetLocation = FVector(135, 0, 0);
const FVector SurfaceLocation = FVector(600, 0, 0);

UUxtNearPointerComponent* Pointer;
const float TargetScale = .3f;
const float SurfaceScale = 7.f;
UUxtSurfaceMagnetismComponent* SurfaceMagnetismComponent;
AActor* TargetActor;
AActor* SurfaceActor;
float DistCheck;
float InRange = 30.f;
END_DEFINE_SPEC(FSurfaceMagnetism)

void FSurfaceMagnetism::Define()
{
	Describe("Surface Magnetism", [this] {
		BeforeEach([this] {
			// Load the empty test map to run the test in.
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(World->GetGameInstance()->TimerManager);
			UxtTestUtils::EnableTestHandTracker();

			// HandInteraction actor
			HandInteractionActor = World->SpawnActor<AUxtHandInteractionActor>();
			HandInteractionActor->SetHand(EControllerHand::Left);

			// pointer
			Pointer = UxtTestUtils::CreateNearPointer(World, "TestPointer", ActorLocation);

			// Target Actor
			TargetActor = World->SpawnActor<AActor>();

			UStaticMeshComponent* Root = UxtTestUtils::CreateBoxStaticMesh(TargetActor, FVector(0.3f));
			TargetActor->SetRootComponent(Root);
			Root->SetWorldLocation(TargetLocation);
			Root->RegisterComponent();

			SurfaceMagnetismComponent = NewObject<UUxtSurfaceMagnetismComponent>(TargetActor);
			SurfaceMagnetismComponent->SetTargetComponent(Root);
			SurfaceMagnetismComponent->bSmoothPosition = false;
			SurfaceMagnetismComponent->bSmoothRotation = false;
			SurfaceMagnetismComponent->RegisterComponent();

			if (!TargetFilename.IsEmpty())
			{
				UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(TargetActor);
				Mesh->SetupAttachment(TargetActor->GetRootComponent());
				Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
				Mesh->SetCollisionProfileName(TEXT("OverlapAll"));
				Mesh->SetGenerateOverlapEvents(true);

				UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(TargetActor, *TargetFilename);
				Mesh->SetStaticMesh(MeshAsset);
				Mesh->RegisterComponent();
			}

			/// Surface Actor
			SurfaceActor = World->SpawnActor<AActor>();
			USceneComponent* RootSurface = NewObject<USceneComponent>(SurfaceActor);
			SurfaceActor->SetRootComponent(RootSurface);
			RootSurface->SetWorldLocation(SurfaceLocation);
			RootSurface->RegisterComponent();

			UStaticMeshComponent* SurfaceMesh = UxtTestUtils::CreateBoxStaticMesh(SurfaceActor, FVector(.5, 5, 5));
			SurfaceMesh->SetupAttachment(SurfaceActor->GetRootComponent());
			SurfaceMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			SurfaceMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
			SurfaceMesh->SetGenerateOverlapEvents(true);
			SurfaceMesh->RegisterComponent();

			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
			FrameQueue.Reset();
			UxtTestUtils::DisableTestHandTracker();
			FrameQueue.Reset();
			HandInteractionActor->Destroy();
			HandInteractionActor = nullptr;
			Pointer->GetOwner()->Destroy();
			Pointer = nullptr;
			TargetActor->Destroy();
			SurfaceActor->Destroy();
		});

		LatentIt("Target should snap to surface when pointing", [this](const FDoneDelegate& Done) {
			UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);

			FrameQueue.Enqueue([this] {
				DistCheck = FVector::Distance(SurfaceActor->GetActorLocation(), TargetActor->GetActorLocation());
				TestTrue(TEXT("Distance outside range"), DistCheck > InRange);
				TestTrue(TEXT("Surface component not is ticking"), !SurfaceMagnetismComponent->IsComponentTickEnabled());
			});
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(NearPoint); });
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetSelectPressed(true); });

			FrameQueue.Enqueue([this] { TestTrue(TEXT("Component is ticking"), SurfaceMagnetismComponent->IsComponentTickEnabled()); });
			FrameQueue.Enqueue([this] {
				DistCheck = FVector::Distance(SurfaceActor->GetActorLocation(), TargetActor->GetActorLocation());
				TestTrue(TEXT("Distance within range"), DistCheck < InRange);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // #if WITH_DEV_AUTOMATION_TESTS
