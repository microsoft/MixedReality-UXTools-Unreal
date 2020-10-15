// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFarBeamComponent.h"

#include "UXTools.h"

#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UUxtFarBeamComponent::UUxtFarBeamComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Will enable tick on far pointer activation
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("StaticMesh'/UXTools/Pointers/Meshes/SM_Tube.SM_Tube'"));
	check(Mesh.Object);
	SetStaticMesh(Mesh.Object);
	SetMobility(EComponentMobility::Movable);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetHiddenInGame(true);
	BindGrab = BindSplineLength = false;
}

void UUxtFarBeamComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UUxtFarPointerComponent* FarPointer = GetOwner()->FindComponentByClass<UUxtFarPointerComponent>())
	{
		FarPointerWeak = FarPointer;

		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(FarPointer);

		// Activate now if the pointer is enabled
		if (FarPointer->IsEnabled())
		{
			OnFarPointerEnabled(FarPointer);
		}

		// Subscribe to pointer state changes
		FarPointer->OnFarPointerEnabled.AddDynamic(this, &UUxtFarBeamComponent::OnFarPointerEnabled);
		FarPointer->OnFarPointerDisabled.AddDynamic(this, &UUxtFarBeamComponent::OnFarPointerDisabled);
		UMaterial* Material = GetMaterial(0)->GetMaterial();

		SetBeamMaterial(Material);
	}
	else
	{
		UE_LOG(UXTools, Error, TEXT("Could not find a far pointer in actor '%s'. Far beam won't work properly."), *GetOwner()->GetName());
	}
}

void UUxtFarBeamComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		FarPointer->OnFarPointerEnabled.RemoveDynamic(this, &UUxtFarBeamComponent::OnFarPointerEnabled);
		FarPointer->OnFarPointerDisabled.RemoveDynamic(this, &UUxtFarBeamComponent::OnFarPointerDisabled);
	}
}

void UUxtFarBeamComponent::OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer)
{
	SetActive(true);
	SetHiddenInGame(false);
}

void UUxtFarBeamComponent::OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer)
{
	SetActive(false);
	SetHiddenInGame(true);
}

void UUxtFarBeamComponent::SetBeamMaterial(UMaterial* NewMaterial)
{
	if (NewMaterial)
	{
		MID = CreateDynamicMaterialInstance(0, NewMaterial);
		if (MID)
		{
			// first check for our target bound parameters and set them to defaults.
			TArray<FMaterialParameterInfo> OutParameterInfo;
			TArray<FGuid> OutParameterIds;
			NewMaterial->GetAllScalarParameterInfo(OutParameterInfo, OutParameterIds);

			BindGrab = false;
			BindSplineLength = false;
			for (int i = 0; i < OutParameterInfo.Num(); ++i)
			{
				if (OutParameterInfo[i].Name == FName("handIndex"))
				{
					float HandIndex = FarPointerWeak->Hand == EControllerHand::Left ? 0.0f : 1.0f;
					MID->SetScalarParameterValue(FName("handIndex"), HandIndex);
				}
				if (OutParameterInfo[i].Name == FName("IsGrabbing"))
				{
					MID->SetScalarParameterValue(FName("IsGrabbing"), 0.0f);
					BindGrab = true;
				}
				if (OutParameterInfo[i].Name == FName("SplineLength"))
				{
					BindSplineLength = true;
				}
			}
		}
	}
}

void UUxtFarBeamComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		const FVector Start = FarPointer->GetRayStart();
		const FVector End = FarPointer->GetHitPoint() + FarPointer->GetHitNormal() * HoverDistance;
		float Len = (Start - End).Size();

		FVector Target = Start + (FarPointer->GetPointerOrientation().GetForwardVector() * Len);
		// Use hand forward vector to influence the beam start tangent
		FVector SourceTangent = FarPointer->GetPointerOrientation().RotateVector(FVector(50, 0, 0));
		// Make end tangent point directly at the target
		FVector EndTangent = End - Target;
		SetStartPosition(Start, false);
		SetEndPosition(End, false);
		SetStartTangent(SourceTangent, false);
		SetEndTangent(EndTangent, true);

		if (MID)
		{
			if (BindSplineLength)
			{
				MID->SetScalarParameterValue(FName("SplineLength"), Len);
			}

			if (BindGrab)
			{
				if (FarPointer->IsPressed())
				{
					MID->SetScalarParameterValue(FName("IsGrabbing"), 1.0f);
				}
				else
				{
					MID->SetScalarParameterValue(FName("IsGrabbing"), 0.0f);
				}
			}
		}
	}
}
