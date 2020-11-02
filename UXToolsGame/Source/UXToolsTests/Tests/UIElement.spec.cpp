// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "UIElementTestComponent.h"
#include "UxtTestUtils.h"

#include "Controls/UxtUIElementComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtUIElementComponent* CreateUIElement(UUxtUIElementComponent* Parent = nullptr)
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		UUxtUIElementComponent* UIElement = NewObject<UUxtUIElementComponent>(Actor);
		UIElement->RegisterComponent();
		Actor->SetRootComponent(UIElement);

		if (Parent)
		{
			Actor->AttachToActor(Parent->GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);
		}

		return UIElement;
	}

	UUIElementTestComponent* AddEventCaptureComponent(UUxtUIElementComponent* UIElement)
	{
		AActor* Actor = UIElement->GetOwner();

		UUIElementTestComponent* EventCaptureComponent = NewObject<UUIElementTestComponent>(Actor);
		UIElement->OnShowElement.AddDynamic(EventCaptureComponent, &UUIElementTestComponent::OnShowElement);
		UIElement->OnHideElement.AddDynamic(EventCaptureComponent, &UUIElementTestComponent::OnHideElement);
		EventCaptureComponent->RegisterComponent();

		return EventCaptureComponent;
	}
} // namespace

BEGIN_DEFINE_SPEC(UIElementSpec, "UXTools.UIElement", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtUIElementComponent* RootUIElement;
UUxtUIElementComponent* ChildUIElement;

END_DEFINE_SPEC(UIElementSpec)

void UIElementSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		RootUIElement = CreateUIElement();
		ChildUIElement = CreateUIElement(RootUIElement);
	});

	AfterEach([this] {
		RootUIElement->GetOwner()->Destroy();
		ChildUIElement->GetOwner()->Destroy();
	});

	It("should be visible by default", [this] {
		TestEqual("Element is visible", RootUIElement->GetUIVisibilitySelf(), EUxtUIElementVisibility::Show);
		TestEqual("Element is visible in the hierarchy", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Show);
		TestFalse("Actor is visible", RootUIElement->GetOwner()->IsHidden());
		TestTrue("Actor collision is enabled", RootUIElement->GetOwner()->GetActorEnableCollision());
	});

	It("should be hidden", [this] {
		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Element is hidden", RootUIElement->GetUIVisibilitySelf(), EUxtUIElementVisibility::Hide);
		TestEqual("Element is hidden in the hierarchy", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);
		TestTrue("Actor is hidden", RootUIElement->GetOwner()->IsHidden());
		TestFalse("Actor collision is disabled", RootUIElement->GetOwner()->GetActorEnableCollision());
	});

	It("should update children", [this] {
		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Root element is hidden", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);
		TestEqual("Child element is hidden", ChildUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);
	});

	It("children should retain state through transitions", [this] {
		ChildUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Root element is visible", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Show);
		TestEqual("Child element is hidden", ChildUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);

		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Root element is hidden", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);
		TestEqual("Child element is hidden", ChildUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);

		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Show);
		TestEqual("Root element is visible", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Show);
		TestEqual("Child element is hidden", ChildUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);
	});

	It("should update state after re-parent", [this] {
		UUxtUIElementComponent* NewParentElement = CreateUIElement();
		NewParentElement->SetUIVisibility(EUxtUIElementVisibility::Hide);

		RootUIElement->GetOwner()->AttachToActor(NewParentElement->GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);
		TestEqual("Root element is hidden", RootUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);
		TestEqual("Child element is hidden", ChildUIElement->GetUIVisibilityInHierarchy(), EUxtUIElementVisibility::Hide);

		NewParentElement->GetOwner()->Destroy();
	});

	It("should trigger events on all elements", [this] {
		UUIElementTestComponent* RootEvents = AddEventCaptureComponent(RootUIElement);
		UUIElementTestComponent* ChildEvents = AddEventCaptureComponent(ChildUIElement);

		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Root triggered deactivation event", RootEvents->HideCount, 1);
		TestEqual("Child triggered deactivation event", ChildEvents->HideCount, 1);

		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Show);
		TestEqual("Root triggered activation event", RootEvents->ShowCount, 1);
		TestEqual("Child triggered activation event", ChildEvents->ShowCount, 1);
	});

	It("should not trigger events on hidden children", [this] {
		UUIElementTestComponent* RootEvents = AddEventCaptureComponent(RootUIElement);
		UUIElementTestComponent* ChildEvents = AddEventCaptureComponent(ChildUIElement);

		ChildUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Child triggered deactivation event", ChildEvents->HideCount, 1);

		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Hide);
		TestEqual("Root triggered deactivation event", RootEvents->HideCount, 1);
		TestEqual("Child did not trigger deactivation event", ChildEvents->HideCount, 1);

		RootUIElement->SetUIVisibility(EUxtUIElementVisibility::Show);
		TestEqual("Root triggered activation event", RootEvents->ShowCount, 1);
		TestEqual("Child did not trigger activation event", ChildEvents->ShowCount, 0);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
