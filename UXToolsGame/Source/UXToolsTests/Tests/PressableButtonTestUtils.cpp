// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "PressableButtonTestUtils.h"

#include "CoreMinimal.h"

#include "Components/SceneComponent.h"

USceneComponent* SetTestButtonVisuals(UUxtPressableButtonComponent* Button, EUxtPushBehavior PushBehavior)
{
	const FString MeshFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
	const float MeshScale = 0.1f;

	AActor* Actor = Button->GetOwner();
	USceneComponent* VisualsSet = nullptr;

	if (!MeshFilename.IsEmpty())
	{
		USceneComponent* Visuals = nullptr;

		// Create a pivot parent component for the compressible visuals
		if (PushBehavior == EUxtPushBehavior::Compress)
		{
			Visuals = NewObject<USceneComponent>(Actor);
			Visuals->SetupAttachment(Actor->GetRootComponent());
			Visuals->RegisterComponent();
		}

		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
		Mesh->SetupAttachment((Visuals != nullptr) ? Visuals : Button);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionProfileName(TEXT("OverlapAll"));
		Mesh->SetGenerateOverlapEvents(true);

		UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(Actor, *MeshFilename);
		Mesh->SetStaticMesh(MeshAsset);
		Mesh->SetRelativeLocation((Visuals != nullptr) ? FVector(-5, 0, 0) : FVector::ZeroVector);
		Mesh->SetRelativeScale3D((Visuals != nullptr) ? FVector(MeshScale * 0.5f, MeshScale, MeshScale) : FVector::OneVector * MeshScale);
		Mesh->RegisterComponent();

		VisualsSet = (Visuals != nullptr) ? Visuals : Mesh;

		Button->SetVisuals(VisualsSet);
	}

	return VisualsSet;
}

UUxtPressableButtonComponent* CreateTestButtonComponent(AActor* Actor, const FVector& Location, EUxtPushBehavior PushBehavior)
{
	UUxtPressableButtonComponent* ButtonComponent = NewObject<UUxtPressableButtonComponent>(Actor);
	ButtonComponent->SetupAttachment(Actor->GetRootComponent());
	ButtonComponent->SetPushBehavior(PushBehavior);
	ButtonComponent->SetWorldRotation(FRotator(0, 180, 0));
	ButtonComponent->SetWorldLocation(Location);
	ButtonComponent->RecoverySpeed = BIG_NUMBER;
	ButtonComponent->SetMaxPushDistance(5);
	SetTestButtonVisuals(ButtonComponent, PushBehavior);
	ButtonComponent->RegisterComponent();

	return ButtonComponent;
}
