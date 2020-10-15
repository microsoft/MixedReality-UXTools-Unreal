// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableToggleButtonActor.h"

#include "Controls/UxtBackPlateComponent.h"
#include "Controls/UxtButtonBrush.h"
#include "Controls/UxtToggleStateComponent.h"

AUxtPressableToggleButtonActor::AUxtPressableToggleButtonActor()
{
	// Set the default button label.
	Label = NSLOCTEXT("UxtPressableButtonActor", "LabelToggle", "Toggle");

	// Create the component hierarchy.
	ToggleStateComponent = CreateDefaultSubobject<UUxtToggleStateComponent>(TEXT("UxtToggleState"));
	TogglePlateComponent = CreateAndAttachComponent<UUxtBackPlateComponent>(TEXT("UxtToggleBackPlate"), BackPlatePivotComponent);
}

void AUxtPressableToggleButtonActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateToggleVisuals();
}

void AUxtPressableToggleButtonActor::BeginPlay()
{
	Super::BeginPlay();

	ToggleStateComponent->OnToggled.AddDynamic(this, &AUxtPressableToggleButtonActor::OnButtonToggled);
	ToggleStateComponent->SetIsChecked(bIsInitiallyChecked);
}

void AUxtPressableToggleButtonActor::ConstructVisuals()
{
	Super::ConstructVisuals();

	// Allow the initially checked option to function while being edited.
	TEnumAsByte<EWorldType::Type> WorldType = GetWorld()->WorldType;
	if ((WorldType == EWorldType::Editor) || (WorldType == EWorldType::EditorPreview))
	{
		ToggleStateComponent->SetIsChecked(bIsInitiallyChecked);
	}

	if (TogglePlateComponent != nullptr)
	{
		// Apply the toggle plate material. If one is not specified by the button brush, use the default material.
		if (ButtonBrush.Visuals.TogglePlateMaterial != nullptr)
		{
			TogglePlateComponent->SetBackPlateMaterial(ButtonBrush.Visuals.TogglePlateMaterial);
		}

		// Srt the toggle plate to be 90% of the back plate, leave the depth unmodified.
		const FVector Size = GetSize();
		TogglePlateComponent->SetRelativeScale3D(FVector(BackPlateMeshComponent->GetRelativeScale3D().X, Size.Y * 0.9f, Size.Z * 0.9f));
		TogglePlateComponent->SetRelativeLocation(FVector(0.16f, 0, 0));
	}

	UpdateToggleVisuals();
}

void AUxtPressableToggleButtonActor::UpdateToggleVisuals()
{
	if (TogglePlateComponent != nullptr)
	{
		TogglePlateComponent->SetVisibility(ToggleStateComponent->IsChecked());
	}
}

void AUxtPressableToggleButtonActor::SetIsInitiallyChecked(bool InitiallyChecked)
{
	bIsInitiallyChecked = InitiallyChecked;

	ConstructVisuals();
}

void AUxtPressableToggleButtonActor::RemoveTogglePlate()
{
	if (TogglePlateComponent != nullptr)
	{
		TogglePlateComponent->DestroyComponent();
		TogglePlateComponent = nullptr;
	}
}

void AUxtPressableToggleButtonActor::OnButtonPressed(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer)
{
	Super::OnButtonPressed(Button, Pointer);

	if (!bToggleOnRelease)
	{
		ToggleStateComponent->SetIsChecked(!ToggleStateComponent->IsChecked());
	}
}

void AUxtPressableToggleButtonActor::OnButtonReleased(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer)
{
	Super::OnButtonReleased(Button, Pointer);

	if (bToggleOnRelease)
	{
		ToggleStateComponent->SetIsChecked(!ToggleStateComponent->IsChecked());
	}
}

void AUxtPressableToggleButtonActor::OnButtonToggled(UUxtToggleStateComponent* ToggleState)
{
	UpdateToggleVisuals();
}
