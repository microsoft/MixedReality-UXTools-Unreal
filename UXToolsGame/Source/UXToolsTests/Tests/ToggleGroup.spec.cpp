// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Engine.h"
#include "UxtTestUtils.h"

#include "Controls/UxtToggleGroupComponent.h"
#include "Controls/UxtToggleStateComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	template <typename TestComponent>
	TestComponent* CreateActorWithComponent()
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		TestComponent* Component = NewObject<TestComponent>(Actor);
		Component->RegisterComponent();

		return Component;
	}
} // namespace

BEGIN_DEFINE_SPEC(ToggleGroupSpec, "UXTools.ToggleGroup", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

UUxtToggleGroupComponent* ToggleGroup;
TArray<UUxtToggleStateComponent*> ToggleStates;

END_DEFINE_SPEC(ToggleGroupSpec)

void ToggleGroupSpec::Define()
{
	BeforeEach([this] {
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		ToggleGroup = CreateActorWithComponent<UUxtToggleGroupComponent>();

		for (int32 i = 0; i < 3; ++i)
		{
			ToggleStates.Add(CreateActorWithComponent<UUxtToggleStateComponent>());
		}
	});

	AfterEach([this] {
		ToggleGroup->GetOwner()->Destroy();

		for (int32 i = 0; i < ToggleStates.Num(); ++i)
		{
			if (ToggleStates[i] != nullptr)
			{
				ToggleStates[i]->GetOwner()->Destroy();
				ToggleStates[i] = nullptr;
			}
		}

		ToggleStates.Empty();
	});

	It("Test insertion and removal", [this] {
		ToggleGroup->AddToggleState(ToggleStates[0]);
		TestEqual("Toggle state added to toggle group", ToggleGroup->GetGroupCount(), 1);
		ToggleGroup->RemoveToggleState(ToggleStates[0]);
		TestEqual("Toggle state removed from toggle group", ToggleGroup->GetGroupCount(), 0);
		for (int32 i = 0; i < ToggleStates.Num(); ++i)
		{
			ToggleGroup->AddToggleState(ToggleStates[i]);
		}
		TestEqual("Three toggle states added to toggle group", ToggleGroup->GetGroupCount(), 3);
		ToggleGroup->RemoveToggleState(ToggleStates[1]);
		TestEqual("The second toggle state was removed from the toggle group", ToggleGroup->GetGroupCount(), 2);
		ToggleGroup->InsertToggleState(ToggleStates[1], 1);
		TestEqual("Insert the second toggle state at the second index", ToggleGroup->GetGroupCount(), 3);
		TestEqual("The second toggle state is at the second index", ToggleGroup->GetToggleStateIndex(ToggleStates[1]), 1);
		TestFalse("Add third toggle state a second time should fail", ToggleGroup->AddToggleState(ToggleStates[2]));
		TestFalse("Add nullptr should fail", ToggleGroup->AddToggleState(nullptr));
		TestFalse("Insert nullptr should fail", ToggleGroup->InsertToggleState(nullptr, 0));
		TestFalse("Remove nullptr should fail", ToggleGroup->RemoveToggleState(nullptr));
		ToggleGroup->EmptyGroup();
		TestEqual("Test emptying the toggle group.", ToggleGroup->GetGroupCount(), 0);
	});

	It("Test selection", [this] {
		for (int32 i = 0; i < ToggleStates.Num(); ++i)
		{
			ToggleGroup->AddToggleState(ToggleStates[i]);
		}
		TestEqual("Initially no index is selected", ToggleGroup->GetSelectedIndex(), INDEX_NONE);
		ToggleGroup->SetSelectedIndex(0);
		TestEqual("Index 0 is selected.", ToggleGroup->GetSelectedIndex(), 0);
		ToggleGroup->SetSelectedIndex(3);
		TestEqual("Out of range selection will result in last element", ToggleGroup->GetSelectedIndex(), 2);
		ToggleGroup->SetSelectedIndex(INDEX_NONE);
		TestEqual("Clear selection", ToggleGroup->GetSelectedIndex(), INDEX_NONE);
		ToggleGroup->SetSelectedIndex(1);
		TestEqual("Select middle toggle state", ToggleGroup->GetSelectedIndex(), 1);
		ToggleGroup->RemoveToggleState(ToggleStates[0]);
		TestEqual("Removal of previous toggle state decrements the selected index", ToggleGroup->GetSelectedIndex(), 0);
		ToggleGroup->RemoveToggleState(ToggleStates[1]);
		TestEqual("Removal of current toggle state invalidates the selected index", ToggleGroup->GetSelectedIndex(), INDEX_NONE);
		ToggleGroup->SetSelectedIndex(ToggleGroup->GetToggleStateIndex(ToggleStates[2]));
		ToggleGroup->EmptyGroup();
		TestEqual("After emptying the group no index should be selected", ToggleGroup->GetSelectedIndex(), INDEX_NONE);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
