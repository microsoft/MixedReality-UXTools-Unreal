// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactions/UxtGrabbableComponent.h"
#include "Input/UxtTouchPointer.h"


FVector UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(const FTransform &Transform, const FUxtGrabPointerData &PointerData)
{
	return Transform.TransformPosition(PointerData.LocalGrabPoint.GetLocation());
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetGrabRotation(const FTransform &Transform, const FUxtGrabPointerData &PointerData)
{
	return Transform.TransformRotation(PointerData.LocalGrabPoint.GetRotation()).Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(const FTransform &Transform, const FUxtGrabPointerData &PointerData)
{
	return PointerData.LocalGrabPoint * Transform;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(const FUxtGrabPointerData &PointerData)
{
	if (ensure(PointerData.Pointer != nullptr))
	{
		return PointerData.Pointer->GetComponentLocation();
	}
	return FVector::ZeroVector;
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetTargetRotation(const FUxtGrabPointerData &PointerData)
{
	if (ensure(PointerData.Pointer != nullptr))
	{
		return PointerData.Pointer->GetComponentRotation();
	}
	return FRotator::ZeroRotator;
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetTargetTransform(const FUxtGrabPointerData &PointerData)
{
	if (ensure(PointerData.Pointer != nullptr))
	{
		return PointerData.Pointer->GetComponentTransform();
	}
	return FTransform::Identity;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetLocationOffset(const FTransform &Transform, const FUxtGrabPointerData &PointerData)
{
	return UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(PointerData) - UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(Transform, PointerData);
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(const FTransform &Transform, const FUxtGrabPointerData &PointerData)
{
	return (FQuat(UUxtGrabPointerDataFunctionLibrary::GetTargetRotation(PointerData)) * FQuat(UUxtGrabPointerDataFunctionLibrary::GetGrabRotation(Transform, PointerData).GetInverse())).Rotator();
}


UUxtGrabbableComponent::UUxtGrabbableComponent()
{
	bTickOnlyWhileGrabbed = true;
}

const TArray<FUxtGrabPointerData> &UUxtGrabbableComponent::GetGrabPointers() const
{
	return GrabPointers;
}

FVector UUxtGrabbableComponent::GetGrabPointCentroid(const FTransform &Transform) const
{
	FVector centroid = FVector::ZeroVector;
	for (const FUxtGrabPointerData &data : GrabPointers)
	{
		centroid += UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(Transform, data);
	}
	centroid /= FMath::Max(GrabPointers.Num(), 1);
	return centroid;
}

FVector UUxtGrabbableComponent::GetTargetCentroid() const
{
	FVector centroid = FVector::ZeroVector;
	for (const FUxtGrabPointerData &data : GrabPointers)
	{
		if (ensure(data.Pointer != nullptr))
		{
			centroid += UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(data);
		}
	}
	centroid /= FMath::Max(GrabPointers.Num(), 1);
	return centroid;
}

bool UUxtGrabbableComponent::FindGrabPointerInternal(UUxtTouchPointer *Pointer, FUxtGrabPointerData const *&OutData, int &OutIndex) const
{
	for (int i = 0; i < GrabPointers.Num(); ++i)
	{
		const FUxtGrabPointerData &data = GrabPointers[i];
		if (data.Pointer == Pointer)
		{
			OutData = &data;
			OutIndex = i;
			return true;
		}
	}

	OutData = nullptr;
	OutIndex = -1;
	return false;
}

void UUxtGrabbableComponent::FindGrabPointer(UUxtTouchPointer *Pointer, bool &Success, FUxtGrabPointerData &PointerData, int &Index) const
{
	FUxtGrabPointerData const *pData;
	Success = FindGrabPointerInternal(Pointer, pData, Index);
	if (Success)
	{
		PointerData = *pData;
	}
}

void UUxtGrabbableComponent::GetPrimaryGrabPointer(bool &Valid, FUxtGrabPointerData &PointerData) const
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

void UUxtGrabbableComponent::GetSecondaryGrabPointer(bool &Valid, FUxtGrabPointerData &PointerData) const
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

bool UUxtGrabbableComponent::GetTickOnlyWhileGrabbed() const
{
	return bTickOnlyWhileGrabbed;
}

void UUxtGrabbableComponent::SetTickOnlyWhileGrabbed(bool bEnable)
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

void UUxtGrabbableComponent::UpdateComponentTickEnabled()
{
	if (bTickOnlyWhileGrabbed)
	{
		PrimaryComponentTick.SetTickFunctionEnable(GrabPointers.Num() > 0);
	}
}

void UUxtGrabbableComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize component tick
	UpdateComponentTickEnabled();
}

void UUxtGrabbableComponent::GraspStarted_Implementation(UUxtTouchPointer* Pointer)
{
	Super::GraspStarted_Implementation(Pointer);

	FUxtGrabPointerData data;
	data.Pointer = Pointer;
	data.StartTime = GetWorld()->GetTimeSeconds();
	data.LocalGrabPoint = Pointer->GetComponentTransform() * GetComponentTransform().Inverse();

	GrabPointers.Add(data);

	// Lock the grabbing pointer so we remain the hovered target as it moves.
	Pointer->SetHoverLocked(true);

	OnBeginGrab.Broadcast(this, data);

	UpdateComponentTickEnabled();
}

void UUxtGrabbableComponent::ResetLocalGrabPoint(FUxtGrabPointerData &PointerData)
{
	PointerData.LocalGrabPoint = PointerData.Pointer->GetComponentTransform() * GetComponentTransform().Inverse();
}

void UUxtGrabbableComponent::GraspEnded_Implementation(UUxtTouchPointer* Pointer)
{
	int numRemoved = GrabPointers.RemoveAll([this, Pointer](const FUxtGrabPointerData &data)
	{
		if (data.Pointer == Pointer)
		{
			Pointer->SetHoverLocked(false);
			OnEndGrab.Broadcast(this, data);
			return true;
		}
		return false;
	});

	UpdateComponentTickEnabled();

	Super::GraspEnded_Implementation(Pointer);
}
