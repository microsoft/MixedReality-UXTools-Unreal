// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "Tests/AutomationCommon.h"
#include "UIElementTestComponent.h"
#include "UxtTestUtils.h"

#include "Controls/UxtUIElement.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	UUxtUIElement* CreateUIElement(UUxtUIElement* Parent = nullptr)
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Root->RegisterComponent();
		Actor->SetRootComponent(Root);

		if (Parent)
		{
			Actor->AttachToActor(Parent->GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);
		}

		UUxtUIElement* UIElement = NewObject<UUxtUIElement>(Actor);
		UIElement->RegisterComponent();

		return UIElement;
	}

	UUIElementTestComponent* AddEventCaptureComponent(UUxtUIElement* UIElement)
	{
		AActor* Actor = UIElement->GetOwner();

		UUIElementTestComponent* EventCaptureComponent = NewObject<UUIElementTestComponent>(Actor);
		UIElement->OnElementActivated.AddDynamic(EventCaptureComponent, &UUIElementTestComponent::OnElementActivated);
		UIElement->OnElementDeactivated.AddDynamic(EventCaptureComponent, &UUIElementTestComponent::OnElementDeactivated);
		EventCaptureComponent->RegisterComponent();

		return EventCaptureComponent;
	}
}

BEGIN_DEFINE_SPEC(UIElementSpec, "UXTools.UIElement", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

	UUxtUIElement* RootUIElement;
	UUxtUIElement* ChildUIElement;

END_DEFINE_SPEC(UIElementSpec)

void UIElementSpec::Define()
{
	BeforeEach([this]
		{
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			RootUIElement = CreateUIElement();
			ChildUIElement = CreateUIElement(RootUIElement);
		});

	AfterEach([this]
		{
			RootUIElement->GetOwner()->Destroy();
			ChildUIElement->GetOwner()->Destroy();
		});

	It("should be visible by default", [this]
		{
			TestTrue("Element is active", RootUIElement->IsElementActiveSelf());
			TestTrue("Element is active in the hierarchy", RootUIElement->IsElementActiveInHierarchy());
			TestFalse("Actor is visible", RootUIElement->GetOwner()->IsHidden());
			TestTrue("Actor collision is enabled", RootUIElement->GetOwner()->GetActorEnableCollision());
		});

	It("should be hidden", [this]
		{
			RootUIElement->SetElementActive(false);
			TestFalse("Element is inactive", RootUIElement->IsElementActiveSelf());
			TestFalse("Element is inactive in the hierarchy", RootUIElement->IsElementActiveInHierarchy());
			TestTrue("Actor is hidden", RootUIElement->GetOwner()->IsHidden());
			TestFalse("Actor collision is disabled", RootUIElement->GetOwner()->GetActorEnableCollision());
		});

	It("should update children", [this]
		{
			RootUIElement->SetElementActive(false);
			TestFalse("Root element is inactive", RootUIElement->IsElementActiveInHierarchy());
			TestFalse("Child element is inactive", ChildUIElement->IsElementActiveInHierarchy());
		});

	It("children should retain state through transitions", [this]
		{
			ChildUIElement->SetElementActive(false);
			TestTrue("Root element is active", RootUIElement->IsElementActiveInHierarchy());
			TestFalse("Child element is inactive", ChildUIElement->IsElementActiveInHierarchy());

			RootUIElement->SetElementActive(false);
			TestFalse("Root element is inactive", RootUIElement->IsElementActiveInHierarchy());
			TestFalse("Child element is inactive", ChildUIElement->IsElementActiveInHierarchy());

			RootUIElement->SetElementActive(true);
			TestTrue("Root element is active", RootUIElement->IsElementActiveInHierarchy());
			TestFalse("Child element is inactive", ChildUIElement->IsElementActiveInHierarchy());
		});

	It("refresh should update state after re-parent", [this]
		{
			UUxtUIElement* NewParentElement = CreateUIElement();
			NewParentElement->SetElementActive(false);

			RootUIElement->GetOwner()->AttachToActor(NewParentElement->GetOwner(), FAttachmentTransformRules::KeepRelativeTransform);
			RootUIElement->RefreshElement();
			TestFalse("Root element is inactive", RootUIElement->IsElementActiveInHierarchy());
			TestFalse("Child element is inactive", ChildUIElement->IsElementActiveInHierarchy());

			NewParentElement->GetOwner()->Destroy();
		});

	It("should trigger events on all elements", [this]
		{
			UUIElementTestComponent* RootEvents = AddEventCaptureComponent(RootUIElement);
			UUIElementTestComponent* ChildEvents = AddEventCaptureComponent(ChildUIElement);

			RootUIElement->SetElementActive(false);
			TestEqual("Root triggered deactivation event", RootEvents->OnElementDeactivatedCount, 1);
			TestEqual("Child triggered deactivation event", ChildEvents->OnElementDeactivatedCount, 1);

			RootUIElement->SetElementActive(true);
			TestEqual("Root triggered activation event", RootEvents->OnElementActivatedCount, 1);
			TestEqual("Child triggered activation event", ChildEvents->OnElementActivatedCount, 1);
		});

	It("should not trigger events on inactive children", [this]
		{
			UUIElementTestComponent* RootEvents = AddEventCaptureComponent(RootUIElement);
			UUIElementTestComponent* ChildEvents = AddEventCaptureComponent(ChildUIElement);

			ChildUIElement->SetElementActive(false);
			TestEqual("Child triggered deactivation event", ChildEvents->OnElementDeactivatedCount, 1);

			RootUIElement->SetElementActive(false);
			TestEqual("Root triggered deactivation event", RootEvents->OnElementDeactivatedCount, 1);
			TestEqual("Child did not trigger deactivation event", ChildEvents->OnElementDeactivatedCount, 1);

			RootUIElement->SetElementActive(true);
			TestEqual("Root triggered activation event", RootEvents->OnElementActivatedCount, 1);
			TestEqual("Child did not trigger activation event", ChildEvents->OnElementActivatedCount, 0);
		});
}

#endif // WITH_DEV_AUTOMATION_TESTS
