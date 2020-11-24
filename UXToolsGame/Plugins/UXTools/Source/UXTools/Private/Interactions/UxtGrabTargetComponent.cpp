// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtGrabTargetComponent.h"

#include "Engine/World.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtInteractionMode.h"
#include "Interactions/UxtInteractionUtils.h"

namespace
{
	FTransform GetHandGripTransform(EControllerHand Hand)
	{
		FQuat GripOrientation;
		FVector GripPosition;
		float GripRadius;
		UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::Palm, GripOrientation, GripPosition, GripRadius);
		return FTransform{GripOrientation, GripPosition};
	}
} // namespace

FVector UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(const FTransform& Transform, const FUxtGrabPointerData& GrabData)
{
	return Transform.TransformPositionNoScale(GrabData.LocalGrabPoint.GetLocation());
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetGrabRotation(const FTransform& Transform, const FUxtGrabPointerData& GrabData)
{
	return Transform.TransformRotation(GrabData.LocalGrabPoint.GetRotation()).Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(const FTransform& Transform, const FUxtGrabPointerData& GrabData)
{
	FTransform TransformNoScale(Transform.GetRotation(), Transform.GetLocation());
	return GrabData.LocalGrabPoint * TransformNoScale;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(const FUxtGrabPointerData& GrabData)
{
	return GrabData.GrabPointTransform.GetLocation();
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetTargetRotation(const FUxtGrabPointerData& GrabData)
{
	return GrabData.GrabPointTransform.GetRotation().Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetGrabPointTransform(const FUxtGrabPointerData& GrabData)
{
	return GrabData.GrabPointTransform;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetLocationOffset(const FTransform& Transform, const FUxtGrabPointerData& GrabData)
{
	return UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabData) -
		   UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(Transform, GrabData);
}

FRotator UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(const FTransform& Transform, const FUxtGrabPointerData& GrabData)
{
	return (FQuat(UUxtGrabPointerDataFunctionLibrary::GetTargetRotation(GrabData)) *
			FQuat(UUxtGrabPointerDataFunctionLibrary::GetGrabRotation(Transform, GrabData).GetInverse()))
		.Rotator();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(const FUxtGrabPointerData& GrabData)
{
	if (GrabData.FarPointer != nullptr)
	{
		return FTransform(GrabData.FarPointer->GetPointerOrientation(), GrabData.FarPointer->GetPointerOrigin());
	}
	else if (ensure(GrabData.NearPointer != nullptr))
	{
		return GrabData.GripToGrabPoint * GetHandGripTransform(GrabData.NearPointer->Hand);
	}

	return FTransform::Identity;
}

FVector UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(const FUxtGrabPointerData& GrabData)
{
	return UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(GrabData).GetLocation();
}

FTransform UUxtGrabPointerDataFunctionLibrary::GetGripTransform(const FUxtGrabPointerData& GrabData)
{
	if (GrabData.FarPointer != nullptr)
	{
		return FTransform(GrabData.FarPointer->GetControllerOrientation(), GrabData.FarPointer->GetHitPoint());
	}
	else if (ensure(GrabData.NearPointer != nullptr))
	{
		return GetHandGripTransform(GrabData.NearPointer->Hand);
	}

	return FTransform::Identity;
}

UUxtGrabTargetComponent::UUxtGrabTargetComponent()
{
	bTickOnlyWhileGrabbed = true;
	InteractionMode = static_cast<int32>(EUxtInteractionMode::Near | EUxtInteractionMode::Far);
	GrabModes = static_cast<int32>(EUxtGrabMode::OneHanded | EUxtGrabMode::TwoHanded);
}

const TArray<FUxtGrabPointerData>& UUxtGrabTargetComponent::GetGrabPointers() const
{
	return GrabPointers;
}

FTransform UUxtGrabTargetComponent::GetGrabPointCentroid(const FTransform& ToWorldTransform) const
{
	if (GrabPointers.Num() > 0)
	{
		FTransform BlendedTransform = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(ToWorldTransform, GrabPointers[0]);
		for (int i = 1; i < GrabPointers.Num(); ++i)
		{
			FTransform GrabTransform = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(ToWorldTransform, GrabPointers[i]);
			BlendedTransform.BlendWith(GrabTransform, 1.0f / (i + 1));
		}

		return BlendedTransform;
	}

	return FTransform::Identity;
}

FTransform UUxtGrabTargetComponent::GetPointerCentroid() const
{
	if (GrabPointers.Num() > 0)
	{
		FTransform BlendedTransform = UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(GrabPointers[0]);
		for (int i = 1; i < GrabPointers.Num(); ++i)
		{
			FTransform PointerTransform = UUxtGrabPointerDataFunctionLibrary::GetPointerTransform(GrabPointers[i]);
			BlendedTransform.BlendWith(PointerTransform, 1.0f / (i + 1));
		}

		return BlendedTransform;
	}

	return FTransform::Identity;
}

FVector UUxtGrabTargetComponent::GetTargetCentroid() const
{
	FVector centroid = FVector::ZeroVector;
	for (const FUxtGrabPointerData& GrabData : GrabPointers)
	{
		centroid += UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabData);
	}
	centroid /= FMath::Max(GrabPointers.Num(), 1);
	return centroid;
}

bool UUxtGrabTargetComponent::FindGrabPointerInternal(
	UUxtNearPointerComponent* NearPointer, UUxtFarPointerComponent* FarPointer, FUxtGrabPointerData const*& OutData, int& OutIndex) const
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

void UUxtGrabTargetComponent::FindGrabPointer(
	UUxtNearPointerComponent* NearPointer, UUxtFarPointerComponent* FarPointer, bool& Success, FUxtGrabPointerData& PointerData,
	int& Index) const
{
	FUxtGrabPointerData const* pData;
	Success = FindGrabPointerInternal(NearPointer, FarPointer, pData, Index);
	if (Success)
	{
		PointerData = *pData;
	}
}

void UUxtGrabTargetComponent::GetPrimaryGrabPointer(bool& Valid, FUxtGrabPointerData& PointerData) const
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

void UUxtGrabTargetComponent::GetSecondaryGrabPointer(bool& Valid, FUxtGrabPointerData& PointerData) const
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

bool UUxtGrabTargetComponent::ForceEndGrab()
{
	if (GrabPointers.Num() == 0)
	{
		return false;
	}

	// Cache active pointers, the GrabPointers array is resized while ending grabs.
	TArray<UUxtNearPointerComponent*> NearPointers;
	TArray<UUxtFarPointerComponent*> FarPointers;
	NearPointers.Reserve(GrabPointers.Num());
	FarPointers.Reserve(GrabPointers.Num());
	for (const FUxtGrabPointerData& GrabPointer : GrabPointers)
	{
		if (GrabPointer.NearPointer)
		{
			NearPointers.Add(GrabPointer.NearPointer);
		}
		if (GrabPointer.FarPointer)
		{
			FarPointers.Add(GrabPointer.FarPointer);
		}
	}

	// End grab for all pointers
	for (UUxtNearPointerComponent* Pointer : NearPointers)
	{
		IUxtGrabHandler::Execute_OnEndGrab(this, Pointer);
	}
	for (UUxtFarPointerComponent* Pointer : FarPointers)
	{
		IUxtFarHandler::Execute_OnFarReleased(this, Pointer);
	}

	return true;
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

bool UUxtGrabTargetComponent::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	// We treat all primitives in the actor as grabbable by default.
	return true;
}

bool UUxtGrabTargetComponent::CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UUxtGrabTargetComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnUpdateFarFocus.Broadcast(this, Pointer);
}

void UUxtGrabTargetComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnEnterFarFocus.Broadcast(this, Pointer);
}

void UUxtGrabTargetComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnExitFarFocus.Broadcast(this, Pointer);
}

void UUxtGrabTargetComponent::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnEnterGrabFocus.Broadcast(this, Pointer);
}

void UUxtGrabTargetComponent::OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnUpdateGrabFocus.Broadcast(this, Pointer);
}

void UUxtGrabTargetComponent::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnExitGrabFocus.Broadcast(this, Pointer);
}

void UUxtGrabTargetComponent::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!(InteractionMode & static_cast<int32>(EUxtInteractionMode::Near)) || !ShouldAcceptGrab())
	{
		return;
	}

	FUxtGrabPointerData GrabData;
	GrabData.NearPointer = Pointer;
	GrabData.StartTime = GetWorld()->GetTimeSeconds();

	GrabData.GripToGrabPoint = Pointer->GetGrabPointerTransform() * GetHandGripTransform(Pointer->Hand).Inverse();
	InitGrabTransform(GrabData);

	GrabPointers.Add(GrabData);

	// Lock the grabbing pointer so we remain the focused target as it moves.
	Pointer->SetFocusLocked(true);

	// Only trigger events when the grab mode requirement has been met.
	if (IsGrabModeRequirementMet())
	{
		OnBeginGrab.Broadcast(this, GrabData);
		UpdateComponentTickEnabled();
	}
}

void UUxtGrabTargetComponent::OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!(InteractionMode & static_cast<int32>(EUxtInteractionMode::Near)))
	{
		// release near pointer if we are not supporting near interaction
		OnEndGrab_Implementation(Pointer);
	}
	// Update the copy of the pointer data in the grab pointer array
	for (FUxtGrabPointerData& GrabData : GrabPointers)
	{
		if (GrabData.NearPointer == Pointer)
		{
			GrabData.GrabPointTransform = GrabData.GripToGrabPoint * GetHandGripTransform(Pointer->Hand);

			OnUpdateGrab.Broadcast(this, GrabData);
		}
	}
}

void UUxtGrabTargetComponent::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!IsGrabbingPointer(Pointer))
	{
		return;
	}

	// Only trigger release events if the grab mode is active.
	const bool bIsEndingGrab = IsGrabModeRequirementMet();

	FUxtGrabPointerData PointerData;
	GrabPointers.RemoveAll([this, Pointer, &PointerData](const FUxtGrabPointerData& GrabData) {
		if (GrabData.NearPointer == Pointer)
		{
			// Unlock the pointer focus so that another target can be selected.
			Pointer->SetFocusLocked(false);
			PointerData = GrabData;

			return true;
		}
		return false;
	});

	if (bIsEndingGrab)
	{
		OnEndGrab.Broadcast(this, PointerData);

		// make sure to update initial ptr transforms once a pointer gets removed to ensure
		// calculations are performed on the correct starting values
		for (FUxtGrabPointerData& GrabData : GrabPointers)
		{
			InitGrabTransform(GrabData);
		}

		UpdateComponentTickEnabled();
	}
}

bool UUxtGrabTargetComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	// We treat all primitives in the actor as far targets by default.
	return true;
}

bool UUxtGrabTargetComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UUxtGrabTargetComponent::InitGrabTransform(FUxtGrabPointerData& GrabData) const
{
	const AActor* Owner = GetOwner();
	const FTransform TransformNoScale = FTransform(Owner->GetActorRotation(), Owner->GetActorLocation());
	const FTransform GripTransform = UUxtGrabPointerDataFunctionLibrary::GetGripTransform(GrabData);
	if (GrabData.NearPointer != nullptr)
	{
		GrabData.GrabPointTransform = GrabData.GripToGrabPoint * GripTransform;
		GrabData.LocalGrabPoint = GrabData.GrabPointTransform * TransformNoScale.Inverse();
	}
	else if (ensure(GrabData.FarPointer != nullptr))
	{
		// store initial grab point in object space
		GrabData.GrabPointTransform = GripTransform;
		GrabData.LocalGrabPoint = GripTransform * TransformNoScale.Inverse();

		// store ray hit point in pointer space
		FTransform PointerTransform(GrabData.FarPointer->GetControllerOrientation(), GrabData.FarPointer->GetPointerOrigin());
		GrabData.FarRayHitPointInPointer = GripTransform * PointerTransform.Inverse();
	}
	GrabData.GripToObject = TransformNoScale * GripTransform.Inverse();
}

void UUxtGrabTargetComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!(InteractionMode & static_cast<int32>(EUxtInteractionMode::Far)) || !ShouldAcceptGrab())
	{
		return;
	}

	FUxtGrabPointerData PointerData;
	PointerData.FarPointer = Pointer;
	PointerData.StartTime = GetWorld()->GetTimeSeconds();

	InitGrabTransform(PointerData);
	GrabPointers.Add(PointerData);

	// Lock the grabbing pointer so we remain the hovered target as it moves.
	Pointer->SetFocusLocked(true);

	// Only trigger events when the grab mode requirement has been met.
	if (IsGrabModeRequirementMet())
	{
		OnBeginGrab.Broadcast(this, PointerData);
		UpdateComponentTickEnabled();
	}
}

void UUxtGrabTargetComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!IsGrabbingPointer(Pointer))
	{
		return;
	}

	// Only trigger release events if the grab mode is active.
	const bool bIsEndingGrab = IsGrabModeRequirementMet();

	FUxtGrabPointerData PointerData;
	GrabPointers.RemoveAll([this, Pointer, &PointerData](const FUxtGrabPointerData& GrabData) {
		if (GrabData.FarPointer == Pointer)
		{
			Pointer->SetFocusLocked(false);
			PointerData = GrabData;
			return true;
		}
		return false;
	});

	if (bIsEndingGrab)
	{
		OnEndGrab.Broadcast(this, PointerData);

		// make sure to update initial ptr transforms once a pointer gets removed to ensure
		// calculations are performed on the correct starting values
		for (FUxtGrabPointerData& GrabData : GrabPointers)
		{
			InitGrabTransform(GrabData);
		}

		UpdateComponentTickEnabled();
	}
}

void UUxtGrabTargetComponent::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!(InteractionMode & static_cast<int32>(EUxtInteractionMode::Far)))
	{
		// release far pointer if we are not supporting far interaction
		OnFarReleased_Implementation(Pointer);
	}

	// Update the copy of the pointer data in the grab pointer array
	for (FUxtGrabPointerData& GrabData : GrabPointers)
	{
		if (GrabData.FarPointer == Pointer)
		{
			FTransform PointerTransform(GrabData.FarPointer->GetControllerOrientation(), GrabData.FarPointer->GetPointerOrigin());
			GrabData.GrabPointTransform = GrabData.FarRayHitPointInPointer * PointerTransform;

			OnUpdateGrab.Broadcast(this, GrabData);
		}
	}
}

void UUxtGrabTargetComponent::ResetLocalGrabPoint(FUxtGrabPointerData& PointerData)
{
	const AActor* Owner = GetOwner();
	const FTransform TransformNoScale = FTransform(Owner->GetActorRotation(), Owner->GetActorLocation());
	PointerData.LocalGrabPoint = PointerData.GrabPointTransform * TransformNoScale.Inverse();
}

bool UUxtGrabTargetComponent::ShouldAcceptGrab() const
{
	if (GrabModes & static_cast<uint32_t>(EUxtGrabMode::OneHanded) && GrabModes & static_cast<uint32_t>(EUxtGrabMode::TwoHanded) &&
		GrabPointers.Num() < 2)
	{
		return true;
	}
	else if (GrabModes & static_cast<uint32_t>(EUxtGrabMode::OneHanded) && GrabPointers.Num() < 1)
	{
		return true;
	}
	else if (GrabModes & static_cast<uint32_t>(EUxtGrabMode::TwoHanded) && GrabPointers.Num() < 2)
	{
		return true;
	}

	return false;
}

bool UUxtGrabTargetComponent::IsGrabModeRequirementMet() const
{
	if (GrabModes & static_cast<uint32_t>(EUxtGrabMode::OneHanded) && GrabModes & static_cast<uint32_t>(EUxtGrabMode::TwoHanded) &&
		GrabPointers.Num() < 2)
	{
		return true;
	}
	else if (GrabModes & static_cast<uint32_t>(EUxtGrabMode::OneHanded) && GrabPointers.Num() == 1)
	{
		return true;
	}
	else if (GrabModes & static_cast<uint32_t>(EUxtGrabMode::TwoHanded) && GrabPointers.Num() == 2)
	{
		return true;
	}

	return false;
}

bool UUxtGrabTargetComponent::IsGrabbingPointer(const UUxtPointerComponent* Pointer)
{
	return GrabPointers.ContainsByPredicate(
		[&Pointer](const FUxtGrabPointerData& GrabData) { return GrabData.NearPointer == Pointer || GrabData.FarPointer == Pointer; });
}
