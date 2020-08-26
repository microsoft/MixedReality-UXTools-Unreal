#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "PinchSliderTestComponent.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtPinchSliderComponent.h"
#include "GameFramework/Actor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Templates/SharedPointer.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

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
		TestTarget->SetSmoothing(0.0f);
		TestTarget->RegisterComponent();

		FString MeshFilename =
			TEXT("/UXTools/Slider/Meshes/SM_Button_Oval_Concave_12x24mm_optimized.SM_Button_Oval_Concave_12x24mm_optimized");

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
} // namespace

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
	Describe("Pinch Slider", [this] {
		BeforeEach([this] {
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

			Slider->OnBeginInteraction.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnInteractionStarted);
			Slider->OnEndInteraction.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnInteractionEnded);
			Slider->OnBeginFocus.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnFocusEnter);
			Slider->OnEndFocus.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnFocusExit);
			Slider->OnUpdateValue.AddDynamic(EventCaptureObj, &UPinchSliderTestComponent::OnValueUpdated);

			World->UpdateWorldComponents(false, false);
		});

		AfterEach([this] {
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

		LatentIt("Thumb should be grabbable", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector); });
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0)); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				FVector Unused;
				FVector Normal;
				UObject* Target = Pointer->GetFocusedGrabTarget(Unused, Normal);
				TestEqual("Thumb is grabbed", Target, (UObject*)Slider);
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Value should update on grab and move", [this](const FDoneDelegate& Done) {
			StartValue = Slider->GetSliderValue();
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector); });
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0)); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 10, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				TestEqual("Slider value", Slider->GetSliderValue(), 1.00f);
				TestTrue("Slider value has updated", StartValue != Slider->GetSliderValue());
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Value should be correct", [this](const FDoneDelegate& Done) {
			StartValue = Slider->GetSliderValue();
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector); });
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0)); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 10 / 2.0f, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				TestEqual("Slider value", Slider->GetSliderValue(), 0.50f);
				TestTrue("Slider Value has updated", StartValue != Slider->GetSliderValue());
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("Events should fire", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector::ZeroVector);
				TestFalse("Focus enter false on start", EventCaptureObj->OnFocusEnterReceived);
				TestFalse("Focus exit false on start", EventCaptureObj->OnFocusExitReceived);
				TestFalse("Interaction started false on start", EventCaptureObj->OnInteractionStartedReceived);
				TestFalse("Interaction ended false on start", EventCaptureObj->OnInteractionEndedReceived);
				TestFalse("Interaction ended false on start", EventCaptureObj->OnValueUpdatedReceived);
			});
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0)); });
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				TestTrue("Focus enter fired on Focus", EventCaptureObj->OnFocusEnterReceived);
				TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Focus);
			});
			FrameQueue.Enqueue([this] { UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(-MoveBy, 0, 0)); });

			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(-MoveBy, 0, 0));
				TestTrue("Focus exit fired on Focus exit", EventCaptureObj->OnFocusExitReceived);
				TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Default);
			});
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
				TestTrue("Interaction started fired on interaction", EventCaptureObj->OnInteractionStartedReceived);
				TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Grab);
			});

			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 10, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
				TestTrue("Value updated fired on value update", EventCaptureObj->OnValueUpdatedReceived);
			});
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(false);
			});
			FrameQueue.Skip();
			FrameQueue.Enqueue([this] {
				TestTrue("Interaction ended fired on end interaction", EventCaptureObj->OnInteractionEndedReceived);
				TestEqual("Current state", Slider->GetCurrentState(), EUxtSliderState::Focus);
			});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should move to focused state when released with focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Slider is grabbed", Slider->IsGrabbed());

				UxtTestUtils::GetTestHandTracker().SetGrabbing(false);
			});

			FrameQueue.Enqueue([this] { TestTrue("Slider is focused", Slider->IsFocused()); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should not be grabbed when disabled", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Slider->SetEnabled(false);
				TestFalse("Slider is disabled", Slider->IsEnabled());

				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] { TestFalse("Slider is not grabbed", Slider->IsGrabbed()); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should release pointers when disabled mid grab", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
				UxtTestUtils::GetTestHandTracker().SetGrabbing(true);
			});

			FrameQueue.Enqueue([this] {
				TestTrue("Slider is grabbed", Slider->IsGrabbed());
				TestTrue("Pointer is locked", Pointer->GetFocusLocked());

				Slider->SetEnabled(false);
				TestFalse("Slider is not grabbed", Slider->IsGrabbed());
				TestFalse("Pointer is not locked", Pointer->GetFocusLocked());
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		LatentIt("should move to focused state when enabled with focus", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				Slider->SetEnabled(false);
				UxtTestUtils::GetTestHandTracker().SetAllJointPositions(FVector(MoveBy, 0, 0));
			});

			FrameQueue.Enqueue([this] {
				TestFalse("Slider is not focused", Slider->IsFocused());

				Slider->SetEnabled(true);
				TestTrue("Slider is focused", Slider->IsFocused());
			});

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});

		It("should be limited between slider bounds", [this] {
			const float LowerBound = 0.2f;
			const float UpperBound = 0.8f;

			Slider->SetSliderLowerBound(LowerBound);
			Slider->SetSliderUpperBound(UpperBound);

			Slider->SetSliderValue(0.0f);
			TestEqual("Thumb is at lower bound", Slider->GetSliderValue(), LowerBound);

			Slider->SetSliderValue(1.0f);
			TestEqual("Thumb is at upper bound", Slider->GetSliderValue(), UpperBound);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
