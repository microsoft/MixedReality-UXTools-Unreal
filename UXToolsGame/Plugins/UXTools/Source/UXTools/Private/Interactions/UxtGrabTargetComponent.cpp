// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtGrabTargetComponent.h"
#include "Engine/World.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"

FVector UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(const FTransform &Transform, const FUxtGrabPointerData &GrabData)
{
	return Transform.TransformPosition(GrabData.LocalGrabPoint.GetLocation());
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetGrabRotation(const FTransform &Transform, const FUxtGrabPointerData &GrabData)
{
	return Transform.TransformRotation(GrabData.LocalGrabPoint.GetRotation()).Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(const FTransform &Transform, const FUxtGrabPointerData &GrabData)
{
	return GrabData.LocalGrabPoint * Transform;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(const FUxtGrabPointerData &GrabData)
{
	return GrabData.GrabPointTransform.GetLocation();
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetTargetRotation(const FUxtGrabPointerData &GrabData)
{
	return GrabData.GrabPointTransform.GetRotation().Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetTargetTransform(const FUxtGrabPointerData &GrabData)
{
	return GrabData.GrabPointTransform;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetLocationOffset(const FTransform &Transform, const FUxtGrabPointerData &GrabData)
{
	return UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabData) - UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(Transform, GrabData);
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(const FTransform &Transform, const FUxtGrabPointerData &GrabData)
{
	return (FQuat(UUxtGrabPointerDataFunctionLibrary::GetTargetRotation(GrabData)) * FQuat(UUxtGrabPointerDataFunctionLibrary::GetGrabRotation(Transform, GrabData).GetInverse())).Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(const FUxtGrabPointerData& GrabData)
{
	if (GrabData.FarPointer != nullptr)
	{
		return FTransform(GrabData.FarPointer->GetPointerOrientation(), GrabData.FarPointer->GetPointerOrigin());
	}
	else if (ensure(GrabData.NearPointer != nullptr))
	{
		return GrabData.NearPointer->GetGrabPointerTransform();
	}

	return FTransform::Identity;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(const FUxtGrabPointerData& GrabData)
{
	if (GrabData.FarPointer != nullptr)
	{
		return GrabData.FarPointer->GetPointerOrigin();
	}
	else if (ensure(GrabData.NearPointer != nullptr))
	{
		return GrabData.NearPointer->GetGrabPointerTransform().GetLocation();
	}

	return FVector::ZeroVector;
}

UUxtGrabTargetComponent::UUxtGrabTargetComponent()
{
	bTickOnlyWhileGrabbed = true;
}

const TArray<FUxtGrabPointerData> &UUxtGrabTargetComponent::GetGrabPointers() const
{
	return GrabPointers;
}

FVector UUxtGrabTargetComponent::GetGrabPointCentroid(const FTransform &Transform) const
{
	FVector centroid = FVector::ZeroVector;
	for (const FUxtGrabPointerData &GrabData : GrabPointers)
	{
		centroid += UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(Transform, GrabData);
	}
	centroid /= FMath::Max(GrabPointers.Num(), 1);
	return centroid;
}

FTransform UUxtGrabTargetComponent::GetPointersTransformCentroid() const
{
	if (GrabPointers.Num() > 0)
	{
		FTransform BlendedTransform = UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(GrabPointers[0]);
		for (int i = 1; i < GrabPointers.Num(); ++i)
		{
			FTransform PointerTransform = UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(GrabPointers[i]);
			BlendedTransform.BlendWith(PointerTransform, 1 / (i + 1));
		}

		return BlendedTransform;
	}

	return FTransform::Identity;
}

FVector UUxtGrabTargetComponent::GetTargetCentroid() const
{
	FVector centroid = FVector::ZeroVector;
	for (const FUxtGrabPointerData &GrabData : GrabPointers)
	{
		centroid += UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabData);
	}
	centroid /= FMath::Max(GrabPointers.Num(), 1);
	return centroid;
}

bool UUxtGrabTargetComponent::FindGrabPointerInternal(UUxtNearPointerComponent* NearPointer, UUxtFarPointerComponent* FarPointer, FUxtGrabPointerData const *&OutData, int &OutIndex) const
{
	for (int i = 0; i < GrabPointers.Num(); ++i)
	{
		const FUxtGrabPointerData& GrabData = GrabPointers[i];
		if ((NearPointer != nullptr && GrabData.NearPointer == NearPointer) || (FarPointer != nullptr && GrabData.FarPointer == FarPointer))
		{
			OutData = &GrabData;
			OutIndex = i;
			return true;
		}
	}

	OutData = nullptr;
	OutIndex = -1;
	return false;
}

void UUxtGrabTargetComponent::FindGrabPointer(UUxtNearPointerComponent* NearPointer, UUxtFarPointerComponent* FarPointer, bool &Success, FUxtGrabPointerData &PointerData, int &Index) const
{
	FUxtGrabPointerData const *pData;
	Success = FindGrabPointerInternal(NearPointer, FarPointer, pData, Index);
	if (Success)
	{
		PointerData = *pData;
	}
}

void UUxtGrabTargetComponent::GetPrimaryGrabPointer(bool &Valid, FUxtGrabPointerData &PointerData) const
{
	if (GrabPointers.Num() >= 1)
	{
		PointerData = GrabPointers[0];
		Valid = true;
	}
	else
	{
		Valid = false;
	}
}

void UUxtGrabTargetComponent::GetSecondaryGrabPointer(bool &Valid, FUxtGrabPointerData &PointerData) const
{
	if (GrabPointers.Num() >= 2)
	{
		PointerData = GrabPointers[1];
		Valid = true;
	}
	else
	{
		Valid = false;
	}
}

bool UUxtGrabTargetComponent::GetTickOnlyWhileGrabbed() const
{
	return bTickOnlyWhileGrabbed;
}

void UUxtGrabTargetComponent::SetTickOnlyWhileGrabbed(bool bEnable)
{
	bTickOnlyWhileGrabbed = bEnable;

	if (bEnable)
	{
		UpdateComponentTickEnabled();
	}
	else
	{
		PrimaryComponentTick.SetTickFunctionEnable(true);
	}
}

void UUxtGrabTargetComponent::UpdateComponentTickEnabled()
{
	if (bTickOnlyWhileGrabbed)
	{
		PrimaryComponentTick.SetTickFunctionEnable(GrabPointers.Num() > 0);
	}
}

void UUxtGrabTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize component tick
	UpdateComponentTickEnabled();
}

bool UUxtGrabTargetComponent::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	// We treat all primitives in the actor as grabbable by default.
	return true;
}

void UUxtGrabTargetComponent::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	FUxtGrabPointerData GrabData;
	GrabData.NearPointer = Pointer;
	GrabData.StartTime = GetWorld()->GetTimeSeconds();
	InitGrabTransform(GrabData);

	GrabPointers.Add(GrabData);

	// Lock the grabbing pointer so we remain the focused target as it moves.
	Pointer->SetFocusLocked(true);

	OnBeginGrab.Broadcast(this, GrabData);

	UpdateComponentTickEnabled();
}

void UUxtGrabTargetComponent::OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	// Update the copy of the pointer data in the grab pointer array
	for (FUxtGrabPointerData& GrabData : GrabPointers)
	{
		if (GrabData.NearPointer == Pointer)
		{
			GrabData.GrabPointTransform = Pointer->GetGrabPointerTransform();

			OnUpdateGrab.Broadcast(this, GrabData);
		}
	}
}


void UUxtGrabTargetComponent::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	int numRemoved = GrabPointers.RemoveAll([this, Pointer](const FUxtGrabPointerData& GrabData)
		{
			if (GrabData.NearPointer == Pointer)
			{
				// Unlock the pointer focus so that another target can be selected.
				Pointer->SetFocusLocked(false);

				OnEndGrab.Broadcast(this, GrabData);
				return true;
			}
			return false;
		});

	// make sure to update initial ptr transforms once a pointer gets removed to ensure
	// calculations are performed on the correct starting values
	for (FUxtGrabPointerData& GrabData : GrabPointers)
	{
		InitGrabTransform(GrabData);
	}

	UpdateComponentTickEnabled();
}

bool UUxtGrabTargetComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	// We treat all primitives in the actor as far targets by default.
	return true;
}

void UUxtGrabTargetComponent::InitGrabTransform(FUxtGrabPointerData& GrabData) const
{
	if (GrabData.NearPointer != nullptr)
	{
		GrabData.GrabPointTransform = GrabData.NearPointer->GetGrabPointerTransform();
		GrabData.LocalGrabPoint = GrabData.GrabPointTransform * GetComponentTransform().Inverse();
	}
	else if (ensure(GrabData.FarPointer != nullptr))
	{
		// store initial grab point in object space
		FTransform TransformAtRayEnd(GrabData.FarPointer->GetPointerOrientation(), GrabData.FarPointer->GetHitPoint());
		GrabData.GrabPointTransform = TransformAtRayEnd;
		GrabData.LocalGrabPoint = TransformAtRayEnd * GetComponentTransform().Inverse();

		// store ray hit point in pointer space
		FTransform PointerTransform(GrabData.FarPointer->GetPointerOrientation(), GrabData.FarPointer->GetPointerOrigin());
		GrabData.FarRayHitPointInPointer = TransformAtRayEnd * PointerTransform.Inverse();
	}
}

void UUxtGrabTargetComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	FUxtGrabPointerData PointerData;
	PointerData.FarPointer = Pointer;
	PointerData.StartTime = GetWorld()->GetTimeSeconds();

	InitGrabTransform(PointerData);
	GrabPointers.Add(PointerData);

	// Lock the grabbing pointer so we remain the hovered target as it moves.
	Pointer->SetFocusLocked(true);
	OnBeginGrab.Broadcast(this, PointerData);
	UpdateComponentTickEnabled();
}

void UUxtGrabTargetComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	int numRemoved = GrabPointers.RemoveAll([this, Pointer](const FUxtGrabPointerData& PointerData)
		{
			if (PointerData.FarPointer == Pointer)
			{
				Pointer->SetFocusLocked(false);
				OnEndGrab.Broadcast(this, PointerData);
				return true;
			}
			return false;
		});

	// make sure to update initial ptr transforms once a pointer gets removed to ensure
	// calculations are performed on the correct starting values
	for (FUxtGrabPointerData& GrabData : GrabPointers)
	{
		InitGrabTransform(GrabData);
	}

	UpdateComponentTickEnabled();
}

void UUxtGrabTargetComponent::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
{
	// Update the copy of the pointer data in the grab pointer array
	for (FUxtGrabPointerData& GrabData : GrabPointers)
	{
		if (GrabData.FarPointer == Pointer)
		{
			FTransform PointerTransform(GrabData.FarPointer->GetPointerOrientation(), GrabData.FarPointer->GetPointerOrigin());
			GrabData.GrabPointTransform = GrabData.FarRayHitPointInPointer * PointerTransform;

			OnUpdateGrab.Broadcast(this, GrabData);
		}
	}
}

void UUxtGrabTargetComponent::ResetLocalGrabPoint(FUxtGrabPointerData &PointerData)
{
	PointerData.LocalGrabPoint = PointerData.GrabPointTransform * GetComponentTransform().Inverse();
}
