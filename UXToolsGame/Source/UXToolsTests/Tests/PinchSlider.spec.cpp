#include "Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"
#include "UxtTestHandTracker.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UxtTestUtils.h"
#include "Input/UxtNearPointerComponent.h"
#include "FrameQueue.h"

#include "Templates/SharedPointer.h"
#include "Controls/UxtPinchSliderComponent.h"
#include "PinchSliderTestComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtPinchSliderComponent* CreateTestComponent(UWorld* World, const FVector& Location)
	{
		AActor* Actor = World->SpawnActor<AActor>();

		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocation(Location);
		Root->RegisterComponent();

		UUxtPinchSliderComponent* TestTarget = NewObject<UUxtPinchSliderComponent>(Actor);
		TestTarget->SetWorldLocation(Location);
		TestTarget->RegisterComponent();

		FString MeshFilename = TEXT("/UXTools/Slider/Meshes/SM_Button_Oval_Concave_12x24mm_optimized.SM_Button_Oval_Concave_12x24mm_optimized");

		if (!MeshFilename.IsEmpty())
		{
			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
			Mesh->SetupAttachment(Actor->GetRootComponent());
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Mesh->SetCollisionProfileName(TEXT("OverlapAll"));
			Mesh->SetGenerateOverlapEvents(true);

			UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(Actor, *MeshFilename);
			Mesh->SetStaticMesh(MeshAsset);
			Mesh->RegisterComponent();

			TestTarget->SetThumbVisuals(Mesh);
			TestTarget->SetSliderStartDistance(0.0f);
			TestTarget->SetSliderEndDistance(10.0f);
			TestTarget->SetSliderValue(0.0f);

		}
		return TestTarget;
	}
}

BEGIN_DEFINE_SPEC(PinchSliderSpec, "UXTools.PinchSlider", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

const int NumPointers = 2;
UUxtPinchSliderComponent* Slider;
UPinchSliderTestComponent* EventCaptureObj;
UUxtNearPointerComponent* Pointer;
FVector Center;
FFrameQueue FrameQueue;
const float MoveBy = 50.0f;
float StartValue;
END_DEFINE_SPEC(PinchSliderSpec)

void PinchSliderSpec::Define()
{
	Describe("Pinch Slider", [this]
		{
			BeforeEach([this]
				{
					// Load the empty test map to run the test in.
					TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

					UWorld* World = UxtTestUtils::GetTestWorld();
					FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());
					UxtTestUtils::EnableTestHandTracker();
					Pointer = UxtTestUtils::CreateNearPointer(World, "TestPointer", FVector::ZeroVector);

					Center = FVector(MoveBy, 0, 0);
					Slider = CreateTestComponent(World, Center);
					EventCaptureObj = NewObject<UPinchSliderTestComponent>(Slider->GetOwner());
					EventCaptureObj->RegisterComponent();

					Slider->OnInteractionStarted.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnInteractionStarted);
					Slider->OnInteractionEnded.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnInteractionEnded);
					Slider->OnFocusEnter.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnFocusEnter);
					Slider->OnFocusExit.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnFocusExit);
					Slider->OnValueUpdated.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnValueUpdated);

					World->UpdateWorldComponents(false, false);
				});

			AfterEach([this]
				{
					UxtTestUtils::DisableTestHandTracker();
					FrameQueue.Reset();
					Slider->GetOwner()->Destroy();
					Slider = nullptr;
					Pointer->GetOwner()->Destroy();
					Pointer = nullptr;

					// Force GC so that destroyed actors are removed from the world.
					// Running multiple tests will otherwise cause errors when creating duplicate actors.
					GEngine->ForceGarbageCollection();
				});
			LatentIt("Thumb should be grabbable", [this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							FVector Unused;
							UObject* Target = Pointer->GetFocusedGrabTarget(Unused);
							TestEqual("Thumb is grabbed", Target, (UObject*)Slider);
						});

					FrameQueue.Enqueue([Done] { Done.Execute(); });

				});
			LatentIt("Value should update on grab and move", [this](const FDoneDelegate& Done)
				{

					StartValue = Slider->GetSliderValue();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 10, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							TestEqual("Slider value", Slider->GetSliderValue(), 1.00f);
							TestTrue("Slider value has updated", StartValue != Slider->GetSliderValue());
						});

					FrameQueue.Enqueue([Done] { Done.Execute(); });

				});
			LatentIt("Value should be correct", [this](const FDoneDelegate& Done)
				{

					StartValue = Slider->GetSliderValue();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 10 / 2.0f, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							TestEqual("Slider value", Slider->GetSliderValue(), 0.50f);
							TestTrue("Slider Value has updated", StartValue != Slider->GetSliderValue());
						});

					FrameQueue.Enqueue([Done] { Done.Execute(); });

				});

			LatentIt("Events should fire", [this](const FDoneDelegate& Done)
				{

					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);
							TestFalse("Focus enter false on start", EventCaptureObj->OnFocusEnterReceived);
							TestFalse("Focus exit false on start", EventCaptureObj->OnFocusExitReceived);
							TestFalse("Interaction started false on start", EventCaptureObj->OnInteractionStartedReceived);
							TestFalse("Interaction ended false on start", EventCaptureObj->OnInteractionEndedReceived);
							TestFalse("Interaction ended false on start", EventCaptureObj->OnValueUpdatedReceived);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							TestTrue("Focus enter fired on Focus", EventCaptureObj->OnFocusEnterReceived);
							TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Focus);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(-MoveBy, 0, 0));
						});

					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(-MoveBy, 0, 0));
							TestTrue("Focus exit fired on Focus exit", EventCaptureObj->OnFocusExitReceived);
							TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Default);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
							TestTrue("Interaction started fired on interaction", EventCaptureObj->OnInteractionStartedReceived);
							TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Grab);
						});

					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 10, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
							TestTrue("Value updated fired on value update", EventCaptureObj->OnValueUpdatedReceived);
						});
					FrameQueue.Enqueue([this]
						{
							UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
							UxtTestUtils::GetTestHandTracker().SetGrabbing(false);

						});
					FrameQueue.Skip();
					FrameQueue.Enqueue([this]
						{
							TestTrue("Interaction ended fired on end interaction", EventCaptureObj->OnInteractionEndedReceived);
							TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Default);
						});
					FrameQueue.Enqueue([Done] { Done.Execute(); });

				});


		});
}



#endif // WITH_DEV_AUTOMATION_TESTS
