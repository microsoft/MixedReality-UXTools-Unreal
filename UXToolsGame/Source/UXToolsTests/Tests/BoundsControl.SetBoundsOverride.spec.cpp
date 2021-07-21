// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTestUtils.h"

#include "Controls/UxtBoundsControlComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(
	BoundsControlBoundsOverrideSpec, "UXTools.BoundsControl.SetBoundsOverride",
	EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)
// This is required to make visible the base class overloads that would otherwise be hidden by the overload below.
using FAutomationTestBase::TestEqual;
bool TestEqual(const FString& What, const FBox& Actual, const FBox& Expected, float Tolerance = KINDA_SMALL_NUMBER);
END_DEFINE_SPEC(BoundsControlBoundsOverrideSpec)

// Test two boxes for equality within a given tolerance per component
bool BoundsControlBoundsOverrideSpec::TestEqual(const FString& What, const FBox& Actual, const FBox& Expected, float Tolerance)
{
	if (!Expected.Min.Equals(Actual.Min, Tolerance) || !Expected.Max.Equals(Actual.Max, Tolerance))
	{
		AddError(
			FString::Printf(
				TEXT("Expected '%s' to be {%s}, but it was {%s} within tolerance %f."), *What, *Expected.ToString(), *Actual.ToString(),
				Tolerance),
			1);
		return false;
	}
	return true;
}

void BoundsControlBoundsOverrideSpec::Define()
{
	It("should modify bounds at runtime", [this] {
		// Open test map
		TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

		// Create actor
		AActor* Actor = UxtTestUtils::GetTestWorld()->SpawnActor<AActor>();

		const FString MeshAssetRefName = TEXT("/Engine/BasicShapes/Cone.Cone");

		// Add root mesh
		UStaticMeshComponent* RootMesh = UxtTestUtils::CreateStaticMesh(Actor, FVector::OneVector, MeshAssetRefName);
		Actor->SetRootComponent(RootMesh);
		RootMesh->RegisterComponent();

		// Add child mesh
		UStaticMeshComponent* ChildMesh = UxtTestUtils::CreateStaticMesh(Actor, FVector::OneVector, MeshAssetRefName);
		ChildMesh->SetupAttachment(Actor->GetRootComponent());
		ChildMesh->SetRelativeLocation(FVector(200, 0, 0));
		ChildMesh->RegisterComponent();

		// Add bounds control
		UUxtBoundsControlComponent* BoundsControl = NewObject<UUxtBoundsControlComponent>(Actor);
		BoundsControl->RegisterComponent();

		// Get root and child local bounds
		FVector Min, Max;
		RootMesh->GetLocalBounds(Min, Max);
		FBox RootBounds(Min, Max);
		ChildMesh->GetLocalBounds(Min, Max);
		FBox ChildBounds(Min, Max);

		// Check bounds control bounds before overriding
		FBox ActualBounds = BoundsControl->GetBounds();
		FBox ExpectedBounds = RootBounds + ChildBounds.ShiftBy(ChildMesh->GetRelativeLocation());
		TestEqual("Bounds before override", ActualBounds, ExpectedBounds);

		// Override bounds
		BoundsControl->SetBoundsOverride(ChildMesh);

		// Check the override component
		USceneComponent* ActualOverride = BoundsControl->GetBoundsOverride();
		USceneComponent* ExpectedOverride = ChildMesh;
		TestEqual("Bounds override", ActualOverride, ExpectedOverride);

		// Check bounds control bounds after overriding
		ActualBounds = BoundsControl->GetBounds();
		ExpectedBounds = ChildBounds;
		TestEqual("Bounds after override", ActualBounds, ExpectedBounds);

		Actor->Destroy();
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
