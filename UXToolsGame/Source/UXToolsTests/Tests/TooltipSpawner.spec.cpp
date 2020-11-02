// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "TooltipEventListener.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Components/SplineMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Controls/UxtFarBeamComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Templates/SharedPointer.h"
#include "Tests/AutomationCommon.h"
#include "Tooltips/UxtTooltipActor.h"
#include "Tooltips/UxtTooltipSpawnerComponent.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(
	TooltipSpawnerSpec, "UXTools.TooltipTest", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UTooltipEventListener* EventListener = nullptr;
UUxtFarPointerComponent* Pointer = nullptr;
FUxtTestHandTracker* HandTracker;
FVector Center;
FFrameQueue FrameQueue;
AActor* TooltipSpawnerActor = nullptr;
EControllerHand Hand = EControllerHand::Right;
UPrimitiveComponent* HitPrimitive = nullptr;

const FVector PivotOffset = FVector(10.0f, 5.0f, 1.f);

END_DEFINE_SPEC(TooltipSpawnerSpec)

void TooltipSpawnerSpec::Define()
{
	Describe("TooltipSpawner", [this] {
		BeforeEach([this] {
			// Load the empty test map to run the test in.
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			Center = FVector(100, 0, 0);

			TooltipSpawnerActor = World->SpawnActor<AActor>();

			// Root.
			USceneComponent* RootNode = NewObject<USceneComponent>(TooltipSpawnerActor);
			TooltipSpawnerActor->SetRootComponent(RootNode);

			// Mesh.
			UStaticMeshComponent* MeshComponent = UxtTestUtils::CreateBoxStaticMesh(TooltipSpawnerActor);
			TooltipSpawnerActor->SetRootComponent(MeshComponent);
			MeshComponent->RegisterComponent();
			HitPrimitive = MeshComponent;

			// Object to listen for onshow/onhide.
			EventListener = NewObject<UTooltipEventListener>();

			// Spawner.
			UUxtTooltipSpawnerComponent* TooltipSpawnerComponent = NewObject<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor);
			TooltipSpawnerComponent->RegisterComponent();
			TooltipSpawnerComponent->OnShowTooltip.AddDynamic(EventListener, &UTooltipEventListener::OnShow);
			TooltipSpawnerComponent->OnHideTooltip.AddDynamic(EventListener, &UTooltipEventListener::OnHide);

			// Pivot.
			USceneComponent* TooltipPivotComponent = NewObject<USceneComponent>(TooltipSpawnerActor);
			TooltipPivotComponent->SetupAttachment(TooltipSpawnerActor->GetRootComponent());
			TooltipPivotComponent->RegisterComponent();

			TooltipSpawnerComponent->Pivot.ComponentProperty = NAME_None;
			TooltipSpawnerComponent->Pivot.PathToComponent = TooltipPivotComponent->GetPathName(TooltipSpawnerActor);

			TooltipSpawnerActor->SetActorLocation(Center);

			// Hand Tracker.
			HandTracker = &UxtTestUtils::EnableTestHandTracker();
			HandTracker->SetAllJointPositions(FVector::ZeroVector);
			HandTracker->SetAllJointOrientations(FQuat::Identity);

			// Far Pointer actor
			{
				AActor* PointerActor = World->SpawnActor<AActor>();

				// Root.
				USceneComponent* Root = NewObject<USceneComponent>(PointerActor);
				Root->RegisterComponent();
				PointerActor->SetRootComponent(Root);

				// Pointer.
				Pointer = NewObject<UUxtFarPointerComponent>(PointerActor);
				Pointer->RegisterComponent();
				Pointer->Hand = Hand;
				Pointer->RayLength = 10.0f;

				// Beam.
				UUxtFarBeamComponent* Beam = NewObject<UUxtFarBeamComponent>(PointerActor);
				Beam->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
				Beam->RegisterComponent();

				// Cursor.
				UUxtFarCursorComponent* Cursor = NewObject<UUxtFarCursorComponent>(PointerActor);
				Cursor->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
				Cursor->RegisterComponent();
			}
		});

		AfterEach([this] {
			UxtTestUtils::DisableTestHandTracker();

			Pointer->GetOwner()->Destroy();
			Pointer = nullptr;

			HandTracker = nullptr;

			EventListener = nullptr;

			HitPrimitive = nullptr;

			FrameQueue.Reset();
			auto* TooltipSpawnerComponent =
				Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
			TooltipSpawnerComponent->OnShowTooltip.RemoveAll(EventListener);

			EventListener = nullptr;
			UxtTestUtils::GetTestWorld()->DestroyActor(TooltipSpawnerActor);
			TooltipSpawnerActor = nullptr;

			// Force GC so that destroyed actors are removed from the world.
			// Running multiple tests will otherwise cause errors when creating duplicate actors.
			GEngine->ForceGarbageCollection();
		});

		LatentIt("Creating/destroying the tooltip spawner", [this](const FDoneDelegate& Done) {
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { TestTrue(TEXT("Construction."), TooltipSpawnerActor->IsValidLowLevel()); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Spawn/Unspawn on focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				auto* TooltipSpawnerComponent =
					Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TooltipSpawnerComponent->AppearType = EUxtTooltipAppearType::AppearOnFocusEnter;
				TooltipSpawnerComponent->VanishType = EUxtTooltipVanishType::VanishOnFocusExit;
				TooltipSpawnerComponent->VanishDelay = 0.f;
				TooltipSpawnerComponent->AppearDelay = 0.f;
				TooltipSpawnerComponent->Lifetime = 10.f;
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				TestEqual(TEXT("We should not have received an onshow event."), EventListener->ShowCount, 0);
				TestEqual(TEXT("We should not have received an onhide event."), EventListener->HideCount, 0);
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] { Pointer->RayLength = 100.0f; });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] {
				TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1);
				TestEqual(TEXT("We should not have received an onhide event."), EventListener->HideCount, 0);
			});

			FrameQueue.Enqueue([this] { Pointer->RayLength = 10.0f; });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] { TestEqual(TEXT("We should have received an onhide event."), EventListener->HideCount, 1); });
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Spawn/Unspawn on tap", [this](const FDoneDelegate& Done) {
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				auto* TooltipSpawnerComponent =
					Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TooltipSpawnerComponent->AppearType = EUxtTooltipAppearType::AppearOnTap;
				TooltipSpawnerComponent->VanishType = EUxtTooltipVanishType::VanishOnTap;
				TooltipSpawnerComponent->VanishDelay = 0.f;
				TooltipSpawnerComponent->AppearDelay = 0.f;
				TooltipSpawnerComponent->Lifetime = 10.f;
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] { Pointer->RayLength = 100.0f; });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 0); });
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetSelectPressed(true); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetSelectPressed(false); });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] { TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1); });

			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetSelectPressed(true); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetSelectPressed(false); });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] { TestEqual(TEXT("We should have received an onhide event."), EventListener->HideCount, 1); });
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		/*LatentIt("Appear delay", [this](const FDoneDelegate& Done)
		{
			FrameQueue.Skip();
			FrameQueue.Enqueue([this]
			{
				auto* TooltipSpawnerComponent =
		Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TooltipSpawnerComponent->AppearType = EUxtTooltipAppearType::AppearOnFocusEnter;
				TooltipSpawnerComponent->VanishType = EUxtTooltipVanishType::VanishOnFocusExit;
				TooltipSpawnerComponent->VanishDelay = 0.f;
				TooltipSpawnerComponent->AppearDelay = 1.f;
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this]
			{
				Pointer->RayLength = 100.0f;
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We shouldn't spawn the tooltip yet because of appear delay."), EventListener->ShowCount, 0);
			});
			FrameQueue.Skip(120);
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Vanish delay", [this](const FDoneDelegate& Done)
		{
			FrameQueue.Enqueue([this]
			{
				auto* TooltipSpawnerComponent =
		Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TooltipSpawnerComponent->AppearType = EUxtTooltipAppearType::AppearOnFocusEnter;
				TooltipSpawnerComponent->VanishType = EUxtTooltipVanishType::VanishOnFocusExit;
				TooltipSpawnerComponent->VanishDelay = 1.f;
				TooltipSpawnerComponent->AppearDelay = 0.f;
				TooltipSpawnerComponent->Lifetime = 10.f;
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should not have received an onshow event."), EventListener->ShowCount, 0);
				TestEqual(TEXT("We should not have received an onhide event."), EventListener->HideCount, 0);
			});
			FrameQueue.Skip(4);
			FrameQueue.Enqueue([this]
			{
				Pointer->RayLength = 100.0f;
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1);
				TestEqual(TEXT("We should not have received an onhide event."), EventListener->HideCount, 0);
			});
			FrameQueue.Enqueue([this]
			{
				Pointer->RayLength = 10.0f;
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1);
				TestEqual(TEXT("We should not have received an onhide event."), EventListener->HideCount, 0);
			});
			FrameQueue.Skip(120);
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should not have received an onhide event."), EventListener->HideCount, 1);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Lifetime", [this](const FDoneDelegate& Done)
		{
			FrameQueue.Skip();
			FrameQueue.Enqueue([this]
			{
				auto* TooltipSpawnerComponent =
		Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TooltipSpawnerComponent->AppearType = EUxtTooltipAppearType::AppearOnFocusEnter;
				TooltipSpawnerComponent->VanishType = EUxtTooltipVanishType::VanishOnFocusExit;
				TooltipSpawnerComponent->VanishDelay = 0.f;
				TooltipSpawnerComponent->AppearDelay = 0.f;
				TooltipSpawnerComponent->Lifetime = 1.f;
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this]
			{
				Pointer->RayLength = 100.0f;
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1);
			});
			FrameQueue.Skip(120);
			FrameQueue.Enqueue([this]
			{
				TestEqual(TEXT("We should have received an onhide event."), EventListener->HideCount, 1);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});*/

		LatentIt("Pivot Offset", [this](const FDoneDelegate& Done) {
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				auto* TooltipSpawnerComponent =
					Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TooltipSpawnerComponent->AppearType = EUxtTooltipAppearType::AppearOnFocusEnter;
				TooltipSpawnerComponent->VanishType = EUxtTooltipVanishType::VanishOnFocusExit;
				TooltipSpawnerComponent->VanishDelay = 0.f;
				TooltipSpawnerComponent->AppearDelay = 0.f;
				TooltipSpawnerComponent->Lifetime = 5.f;
				TooltipSpawnerComponent->bIsAutoAnchoring = false;
				USceneComponent* PivotComponent = Cast<USceneComponent>(TooltipSpawnerComponent->Pivot.GetComponent(TooltipSpawnerActor));
				if (TestNotNull(TEXT("Pivot component should be valid"), PivotComponent))
				{
					PivotComponent->SetRelativeLocation(PivotOffset);
				}
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { Pointer->RayLength = 100.0f; });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] {
				TestEqual(TEXT("We should have received an onshow event."), EventListener->ShowCount, 1);
				auto* TooltipSpawnerComponent =
					Cast<UUxtTooltipSpawnerComponent>(TooltipSpawnerActor->GetComponentByClass(UUxtTooltipSpawnerComponent::StaticClass()));
				TestTrue(
					TEXT("The tooltip component should not be null when spawned."), TooltipSpawnerComponent->SpawnedTooltip != nullptr);
				if (TooltipSpawnerComponent->SpawnedTooltip)
				{
					auto Position = TooltipSpawnerComponent->SpawnedTooltip->GetActorLocation();
					auto RelPosition = TooltipSpawnerActor->GetActorTransform().Inverse().TransformPosition(Position);
					TestEqual(TEXT("The distance of the tooltip should match the pivot offset."), RelPosition, PivotOffset);
				}
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
