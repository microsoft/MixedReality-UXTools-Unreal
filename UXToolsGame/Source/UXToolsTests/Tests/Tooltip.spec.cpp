// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "AutomationBlueprintFunctionLibrary.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Blueprint/UserWidget.h"
#include "Components/SplineMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Misc/AutomationTest.h"
#include "Templates/SharedPointer.h"
#include "Tests/AutomationCommon.h"
#include "Tooltips/UxtTooltipActor.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(TooltipSpec, "UXTools.TooltipTest", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

AActor* SomeTargetActor;
AUxtTooltipActor* TooltipActor;
UUxtNearPointerComponent* Pointer;
FVector TooltipLocation;
FFrameQueue FrameQueue;
FUxtTestHandTracker* HandTracker;
const float MoveBy = 10;
FVector v0; // General purpose vector register.
END_DEFINE_SPEC(TooltipSpec)

void TooltipSpec::Define()
{
	Describe("Tooltip", [this] {
		BeforeEach([this] {
			// Load the empty test map to run the test in.
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			TooltipLocation = FVector(75, 10, 0);

			// Target Actor.
			SomeTargetActor = World->SpawnActor<AActor>();
			USceneComponent* RootNode = NewObject<USceneComponent>(SomeTargetActor);
			SomeTargetActor->SetRootComponent(RootNode);
			UStaticMeshComponent* MeshComponent = UxtTestUtils::CreateBoxStaticMesh(SomeTargetActor);
			SomeTargetActor->SetRootComponent(MeshComponent);
			MeshComponent->RegisterComponent();
			SomeTargetActor->SetActorLocation(FVector(50, 10, 0), false);

			// Tooltip Actor
			TooltipActor = World->SpawnActor<AUxtTooltipActor>();
			TooltipActor->SetActorLocationAndRotation(TooltipLocation, FQuat::Identity, false);
			const FTransform T1 = TooltipActor->GetTransform();

			// Hand Tracker.
			HandTracker = &UxtTestUtils::EnableTestHandTracker();
			HandTracker->SetAllJointPositions(FVector::ZeroVector);
			HandTracker->SetAllJointOrientations(FQuat::Identity);

			Pointer = UxtTestUtils::CreateNearPointer(World, "TestPointer", FVector::ZeroVector);
			Pointer->PokeDepth = 5;
		});

		AfterEach([this] {
			HandTracker = nullptr;
			UxtTestUtils::DisableTestHandTracker();

			FrameQueue.Reset();
			UxtTestUtils::GetTestWorld()->DestroyActor(TooltipActor);
			TooltipActor = nullptr;
			UxtTestUtils::GetTestWorld()->DestroyActor(SomeTargetActor);
			SomeTargetActor = nullptr;
			Pointer->GetOwner()->Destroy();
			Pointer = nullptr;
		});

		LatentIt("Creating/destroying the tooltip", [this](const FDoneDelegate& Done) {
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] { TestTrue("Make sure tooltip is constructed", TooltipActor->IsValidLowLevel()); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Changing the tooltip widget", [this](const FDoneDelegate& Done) {
			const FString WidgetBPAddress("/Game/UXToolsGame/Tests/Tooltip/W_TestText.W_TestText");
			UBlueprint* WidgetBP =
				Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *WidgetBPAddress, nullptr, LOAD_None, nullptr));
			TestTrue("Loading Test widget", WidgetBP != nullptr);
			FrameQueue.Skip();
			FrameQueue.Enqueue([this, WidgetBP] { TooltipActor->WidgetClass = WidgetBP->GeneratedClass; });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, WidgetBP] {
				TSubclassOf<class UUserWidget> CurrentClass = TooltipActor->TooltipWidgetComponent->GetWidgetClass();
				TSubclassOf<class UUserWidget> GeneratedClass = TooltipActor->WidgetClass;
				bool bAreClassEqual = CurrentClass == GeneratedClass;
				TestTrue(TEXT("Our Tooltip should now have the custom TestText widget"), bAreClassEqual);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Attaching tooltip to target", [this](const FDoneDelegate& Done) {
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				TooltipActor->TooltipTarget.OtherActor = SomeTargetActor;
				TooltipActor->TooltipTarget.OverrideComponent = SomeTargetActor->GetRootComponent();
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this] {
				// we should be visible and have a spline
				auto StartPos = TooltipActor->SplineMeshComponent->GetStartPosition();
				auto EndPos = TooltipActor->SplineMeshComponent->GetEndPosition();
				auto Delta = StartPos - EndPos;
				TestTrue(
					TEXT("A tooltip attached to an actor should have a spline with a different start and end point."), Delta.Size() > 1.0);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Attaching to movable and making sure it follows", [this](const FDoneDelegate& Done) {
			const FVector Offset(10.0f, 0.0f, 0.0f);
			FrameQueue.Skip();
			FrameQueue.Enqueue([this, Offset] { TooltipActor->SetTarget(SomeTargetActor, SomeTargetActor->GetRootComponent()); });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, Offset] {
				v0 = TooltipActor->GetActorLocation();
				auto OriginalActorLocation = SomeTargetActor->GetActorLocation();
				SomeTargetActor->SetActorLocation(OriginalActorLocation + Offset);
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, Offset] {
				auto NewTooltipLocation = TooltipActor->GetActorLocation();
				auto Delta = NewTooltipLocation - v0;
				Delta = Delta - Offset;
				TestTrue(TEXT("The tooltip should move by the same amount as the target"), Delta.Size() < 0.01f);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Test Anchors", [this](const FDoneDelegate& Done) {
			const FVector Offset(10.0f, 0.0f, 0.0f);
			FrameQueue.Skip();
			FrameQueue.Enqueue([this, Offset] { TooltipActor->SetTarget(SomeTargetActor, SomeTargetActor->GetRootComponent()); });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, Offset] {
				auto EndPositionLocal = TooltipActor->SplineMeshComponent->GetEndPosition();
				auto EndPositionWorld = TooltipActor->GetTransform().TransformPositionNoScale(EndPositionLocal);
				auto SomeActorPosition = SomeTargetActor->GetActorLocation();
				auto Delta = SomeActorPosition - EndPositionWorld;
				TestTrue(TEXT("The tooltip end + anchor doesn't match what we anticipate it to be"), Delta.Size() < 0.01f);

				TooltipActor->Anchor->SetRelativeLocation(Offset);
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, Offset] {
				auto EndPositionLocal = TooltipActor->SplineMeshComponent->GetEndPosition();
				auto EndPositionWorld = TooltipActor->GetTransform().TransformPositionNoScale(EndPositionLocal);
				auto SomeActorPosition = SomeTargetActor->GetActorLocation();
				auto Delta = EndPositionWorld - SomeActorPosition;
				Delta = Delta - Offset;
				TestTrue(TEXT("The tooltip end + anchor doesn't match what we anticipate it to be"), Delta.Size() < 0.01f);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Test Billboard", [this](const FDoneDelegate& Done) {
			FrameQueue.Skip();
			FrameQueue.Enqueue([this, Done] {
				FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(UxtTestUtils::GetTestWorld());
				const FVector TargetVector = HeadTransform.GetLocation() - TooltipActor->GetActorLocation();
				auto Rot1 = TooltipActor->GetActorRotation().Vector();
				auto Rot2 = FRotationMatrix::MakeFromX(TargetVector).Rotator().Vector();
				TestEqual("Make sure the tooltip is billboarding to the head.", Rot1, Rot2);
				Done.Execute();
			});
		});

		LatentIt("Test Auto Anchor", [this](const FDoneDelegate& Done) {
			const FVector Offset(10.0f, 0.0f, 0.0f);
			FrameQueue.Skip();
			FrameQueue.Enqueue([this, Offset] { TooltipActor->SetTarget(SomeTargetActor, SomeTargetActor->GetRootComponent()); });
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, Offset] {
				v0 = TooltipActor->GetActorLocation();
				auto OriginalActorLocation = SomeTargetActor->GetActorLocation();
				SomeTargetActor->SetActorLocation(OriginalActorLocation + Offset);
			});
			FrameQueue.Skip(2);
			FrameQueue.Enqueue([this, Offset] {
				auto SplineStartPos = TooltipActor->SplineMeshComponent->GetStartPosition();

				TestTrue(TEXT("The start position should not be 0 0 0 if the auto anchor is working."), SplineStartPos.Size() > 1.0f);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
		// Commenting out tests which currently fail until they can be fixed.
		// LatentIt("Test SetVisibility", [this](const FDoneDelegate& Done)
		//{
		//	//wait for the screen to really load before taking the screenshot
		//	FrameQueue.Skip();
		//	FrameQueue.Enqueue([this]
		//	{
		//		UAutomationBlueprintFunctionLibrary::FinishLoadingBeforeScreenshot();
		//		TooltipActor->BackPlate->SetHiddenInGame(true); // The backplate rendering is non deterministic.
		//	});
		//	FrameQueue.Skip(5);
		//	FrameQueue.Enqueue([this]
		//	{
		//		UWorld* World = UxtTestUtils::GetTestWorld();
		//		FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.AddLambda([this]() {
		//			FrameQueue.Resume();
		//		});
		//		FAutomationScreenshotOptions Options(EComparisonTolerance::Low);
		//		if (UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(World, "TestVisibilityScreenshot.jpg", "You should
		// see the tooltip", Options))
		//		{
		//			FrameQueue.Pause();
		//		}
		//	});
		//	FrameQueue.Skip(2);
		//	FrameQueue.Enqueue([this]
		//	{
		//		TooltipActor->SetActorHiddenInGame(true);
		//	});
		//	FrameQueue.Skip();
		//	FrameQueue.Enqueue([this]
		//	{
		//		UWorld* World = UxtTestUtils::GetTestWorld();
		//		FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.AddLambda([this]() {
		//			FrameQueue.Resume();
		//		});
		//		FAutomationScreenshotOptions Options(EComparisonTolerance::Low);
		//		if (UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(World, "TestInVisibilityScreenshot.jpg", "You
		// shouldn't see the tooltip", Options))
		//		{
		//			FrameQueue.Pause();
		//		}
		//	});
		//	FrameQueue.Skip(2);
		//	FrameQueue.Enqueue([this, Done]
		//	{
		//		FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.RemoveAll(this);
		//		Done.Execute();
		//	});
		//});
		// LatentIt("Test SetText", [this](const FDoneDelegate& Done)
		//{
		//	//wait for the screen to really load before taking the screenshot
		//	FrameQueue.Skip();
		//	FrameQueue.Enqueue([this]
		//	{
		//		UAutomationBlueprintFunctionLibrary::FinishLoadingBeforeScreenshot();
		//		TooltipActor->BackPlate->SetHiddenInGame(true); // The backplate rendering is non deterministic.
		//	});
		//	FrameQueue.Skip();
		//	FrameQueue.Enqueue([this]
		//	{
		//		TooltipActor->SetText(FText::AsCultureInvariant("Some custom text"));
		//	});
		//	FrameQueue.Skip(5);
		//	FrameQueue.Enqueue([this]
		//	{
		//		UWorld* World = UxtTestUtils::GetTestWorld();
		//		FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.AddLambda([this]() {
		//			FrameQueue.Resume();
		//		});
		//		FAutomationScreenshotOptions Options(EComparisonTolerance::Low);
		//		if (UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(World, "TestCustomTextScreenshot.jpg", "The
		// default text should have changed.", Options))
		//		{
		//			FrameQueue.Pause();
		//		}
		//	});
		//	FrameQueue.Skip(2);
		//	FrameQueue.Enqueue([this, Done]
		//	{
		//		FAutomationTestFramework::Get().OnScreenshotTakenAndCompared.RemoveAll(this);
		//		Done.Execute();
		//	});
		//});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
