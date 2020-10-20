// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "EngineUtils.h"
#include "FrameQueue.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/WidgetComponent.h"
#include "Controls/UxtWidgetComponent.h"
#include "GameFramework/Actor.h"
#include "Interactions/UxtInteractionMode.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UWidgetComponent* CreateTestComponent(const FString Path, UWorld* World, const FVector& Location)
	{
		UBlueprint* WidgetBP = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *Path));

		if (!WidgetBP)
		{
			return nullptr;
		}

		AActor* Actor = World->SpawnActor<AActor>(WidgetBP->GeneratedClass, Location, FRotator(0, 180, 0));

		return Cast<UWidgetComponent>(Actor->GetComponentByClass(UWidgetComponent::StaticClass()));
	}

	template <typename WidgetType>
	WidgetType* GetWidget(const UWidgetComponent* WidgetComponent)
	{
		UWidget* Root = WidgetComponent->GetUserWidgetObject()->WidgetTree->RootWidget;
		TArray<UWidget*> Children;
		WidgetComponent->GetUserWidgetObject()->WidgetTree->GetChildWidgets(Root, Children);
		for (UWidget* Child : Children)
		{
			if (WidgetType* Widget = Cast<WidgetType>(Child))
			{
				return Widget;
			}
		}

		return nullptr;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	WidgetComponentSpec, "UXTools.WidgetComponent", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void EnqueueButtonTest();
void EnqueueSliderTest();
void EnqueueCheckBoxTest();
void EnqueueTwoHandedTest();

UWidgetComponent* Widget;
UUxtNearPointerComponent* Pointer;
FVector TargetLocation{50, 0, 0};
FFrameQueue FrameQueue;

EUxtInteractionMode InteractionMode;
FUxtTestHand LeftHand = FUxtTestHand(EControllerHand::Left);
FUxtTestHand RightHand = FUxtTestHand(EControllerHand::Right);

const float MoveBy = 10;

END_DEFINE_SPEC(WidgetComponentSpec)

void WidgetComponentSpec::Define()
{
	BeforeEach([this] {
		// Load the empty test map to run the test in.
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		UWorld* World = UxtTestUtils::GetTestWorld();
		FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

		UxtTestUtils::EnableTestHandTracker();
	});

	AfterEach([this] {
		UxtTestUtils::DisableTestHandTracker();

		FrameQueue.Reset();
	});

	Describe("Widget Component Poke Interaction", [this] {
		BeforeEach([this] {
			InteractionMode = EUxtInteractionMode::Near;
			LeftHand.Configure(InteractionMode, TargetLocation);
		});

		AfterEach([this] { LeftHand.Reset(); });

		EnqueueButtonTest();
		EnqueueSliderTest();
		EnqueueCheckBoxTest();
	});

	Describe("Widget Component Far Interaction", [this] {
		BeforeEach([this] {
			InteractionMode = EUxtInteractionMode::Far;
			LeftHand.Configure(InteractionMode, TargetLocation);
		});

		AfterEach([this] { LeftHand.Reset(); });

		EnqueueButtonTest();
		EnqueueSliderTest();
		EnqueueCheckBoxTest();
	});

	Describe("Widget Component Two Hand Interaction", [this] {
		BeforeEach([this] {
			FVector Delta = FVector(0, 50, 0);
			InteractionMode = EUxtInteractionMode::Far;
			LeftHand.Configure(InteractionMode, TargetLocation - Delta);
			RightHand.Configure(InteractionMode, TargetLocation + Delta);

			Widget =
				CreateTestComponent("/Game/UXToolsGame/Tests/Widget/BP_TwoInOne.BP_TwoInOne", UxtTestUtils::GetTestWorld(), TargetLocation);

			TestTrue("Should have valid test actor", Widget != nullptr);
		});

		AfterEach([this] {
			LeftHand.Reset();
			RightHand.Reset();

			if (Widget)
			{
				Widget->GetOwner()->Destroy();
				Widget = nullptr;
			}
		});

		EnqueueTwoHandedTest();
	});
}

void WidgetComponentSpec::EnqueueButtonTest()
{
	Describe("Widget Component Button", [this] {
		BeforeEach([this] {
			Widget =
				CreateTestComponent("/Game/UXToolsGame/Tests/Widget/BP_Button.BP_Button", UxtTestUtils::GetTestWorld(), TargetLocation);

			TestTrue("Should have valid test actor", Widget != nullptr);
		});

		AfterEach([this] {
			if (Widget)
			{
				Widget->GetOwner()->Destroy();
				Widget = nullptr;
			}
		});

		LatentIt("should interact between a UMG button and UXT hands", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				// UUxtWidgetComponent::PointerMove requires previous and current positions in the pointer event
				// As a result, we expect no hover on the first frame
				TestFalse("Button is not hovered in first frame", GetWidget<UButton>(Widget)->IsHovered());
			});

			// test that we now have hover and attempt to press the button
			FrameQueue.Enqueue([this] {
				TestTrue("Button is hovered", GetWidget<UButton>(Widget)->IsHovered());

				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation);
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(true);
					break;
				default:
					break;
				}
			});

			// test the button is pressed and attempt to release
			FrameQueue.Enqueue([this] {
				TestTrue("Button is pressed", GetWidget<UButton>(Widget)->IsPressed());

				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation + FVector(-5, 0, 0));
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(false);
					break;
				default:
					break;
				}
			});

			// test the button is released
			FrameQueue.Enqueue([this] { TestFalse("Button is released", GetWidget<UButton>(Widget)->IsPressed()); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void WidgetComponentSpec::EnqueueSliderTest()
{
	Describe("Widget Component Slider", [this] {
		BeforeEach([this] {
			Widget =
				CreateTestComponent("/Game/UXToolsGame/Tests/Widget/BP_Slider.BP_Slider", UxtTestUtils::GetTestWorld(), TargetLocation);

			TestTrue("Should have valid test actor", Widget != nullptr);
		});

		AfterEach([this] {
			if (Widget)
			{
				Widget->GetOwner()->Destroy();
				Widget = nullptr;
			}
		});

		LatentIt("should interact between a UMG slider and UXT hands", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				// UUxtWidgetComponent::PointerMove requires previous and current positions in the pointer event
				// As a result, we expect no hover on the first frame
				TestFalse("Slider is not hovered in first frame", GetWidget<USlider>(Widget)->IsHovered());
			});

			// test that we now have hover and attempt to select the slider
			FrameQueue.Enqueue([this] {
				TestEqual("Slider initial value is minimum", GetWidget<USlider>(Widget)->GetValue(), 0.0f);
				TestTrue("Slider is hovered", GetWidget<USlider>(Widget)->IsHovered());

				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation);
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(true);
					break;
				default:
					break;
				}
			});

			// test the slider has moved and move further
			FrameQueue.Enqueue([this] {
				TestEqual("New slider value is middle value", GetWidget<USlider>(Widget)->GetValue(), 0.5f);

				LeftHand.SetTranslation(LeftHand.GetTransform().GetLocation() + FVector(0, 50, 0));
			});

			// Frame skip needed because of internal operation of the USlider, the position of
			// which appears to be one frame behind the cursor.
			FrameQueue.Skip();

			// test the slider is maximum
			FrameQueue.Enqueue([this] { TestEqual("New slider value is maximum value", GetWidget<USlider>(Widget)->GetValue(), 1.0f); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void WidgetComponentSpec::EnqueueCheckBoxTest()
{
	Describe("Widget Component Check Box", [this] {
		BeforeEach([this] {
			Widget =
				CreateTestComponent("/Game/UXToolsGame/Tests/Widget/BP_CheckBox.BP_CheckBox", UxtTestUtils::GetTestWorld(), TargetLocation);

			TestTrue("Should have valid test actor", Widget != nullptr);
		});

		AfterEach([this] {
			if (Widget)
			{
				Widget->GetOwner()->Destroy();
				Widget = nullptr;
			}
		});

		LatentIt("should interact between a UMG check box and UXT hands", [this](const FDoneDelegate& Done) {
			FrameQueue.Enqueue([this] {
				// UUxtWidgetComponent::PointerMove requires previous and current positions in the pointer event
				// As a result, we expect no hover on the first frame
				TestFalse("Check box is not hovered in first frame", GetWidget<UCheckBox>(Widget)->IsHovered());
			});

			// test that we now have hover and attempt to press the check box
			FrameQueue.Enqueue([this] {
				TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked());
				TestTrue("Check box is hovered", GetWidget<UCheckBox>(Widget)->IsHovered());

				// select the check box
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation);
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(true);
					break;
				default:
					break;
				}
			});

			// test the check box is not checked and release
			FrameQueue.Enqueue([this] {
				TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked());

				// deselect the check box
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation + FVector(-5, 0, 0));
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(false);
					break;
				default:
					break;
				}
			});

			// test the check box is checked and select again
			FrameQueue.Enqueue([this] {
				TestTrue("Check box is checked", GetWidget<UCheckBox>(Widget)->IsChecked());

				// select the check box again
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation);
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(true);
					break;
				default:
					break;
				}
			});

			// test the check box is still checked and deselect
			FrameQueue.Enqueue([this] {
				TestTrue("Check box is checked", GetWidget<UCheckBox>(Widget)->IsChecked());

				// deselect the check box again
				switch (InteractionMode)
				{
				case EUxtInteractionMode::Near:
					LeftHand.SetTranslation(TargetLocation + FVector(-5, 0, 0));
					break;
				case EUxtInteractionMode::Far:
					LeftHand.SetGrabbing(false);
					break;
				default:
					break;
				}
			});

			// test the check box is still checked and deselect
			FrameQueue.Enqueue([this] { TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked()); });

			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
	});
}

void WidgetComponentSpec::EnqueueTwoHandedTest()
{
	LatentIt("should interact between UMG widgets and two UXT hands", [this](const FDoneDelegate& Done) {
		FrameQueue.Enqueue([this] {
			// UUxtWidgetComponent::PointerMove requires previous and current positions in the pointer event
			// As a result, we expect no hover on the first frame
			TestFalse("Button is not hovered in first frame", GetWidget<UButton>(Widget)->IsHovered());
			TestFalse("Check box is not hovered in first frame", GetWidget<UCheckBox>(Widget)->IsHovered());
		});

		// test that we now have hover and attempt to select the checkbox
		FrameQueue.Enqueue([this] {
			TestFalse("Button is not pressed", GetWidget<UButton>(Widget)->IsPressed());
			TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked());
			TestTrue("Button is hovered", GetWidget<UButton>(Widget)->IsHovered());
			TestTrue("Check box is hovered", GetWidget<UCheckBox>(Widget)->IsHovered());

			RightHand.SetGrabbing(true);
		});

		// ensure that the state hasn't changed yet and press the button
		FrameQueue.Enqueue([this] {
			TestFalse("Button is not pressed", GetWidget<UButton>(Widget)->IsPressed());
			TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked());

			LeftHand.SetGrabbing(true);
		});

		// test the button is pressed and release the button
		FrameQueue.Enqueue([this] {
			TestTrue("Button is pressed", GetWidget<UButton>(Widget)->IsPressed());
			TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked());

			LeftHand.SetGrabbing(false);
		});

		// test the button is released and release the checkbox
		FrameQueue.Enqueue([this] {
			TestFalse("Button is not pressed", GetWidget<UButton>(Widget)->IsPressed());
			TestFalse("Check box is not checked", GetWidget<UCheckBox>(Widget)->IsChecked());

			RightHand.SetGrabbing(false);
		});

		// test the checkbox is now checked
		FrameQueue.Enqueue([this] {
			TestFalse("Button is not pressed", GetWidget<UButton>(Widget)->IsPressed());
			TestTrue("Check box is checked", GetWidget<UCheckBox>(Widget)->IsChecked());
		});

		FrameQueue.Enqueue([Done] { Done.Execute(); });
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
