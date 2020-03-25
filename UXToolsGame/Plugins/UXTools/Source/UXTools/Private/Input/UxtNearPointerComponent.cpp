// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtTouchTarget.h"

#include "Engine/World.h"

namespace
{
	/** Find a component of the actor that implements the given interface type. */
	template <class InterfaceClass>
	UActorComponent* FindInterfaceComponent(AActor* Owner)
	{
		for (UActorComponent* Component : Owner->GetComponents())
		{
			if (Component->Implements<typename InterfaceClass::UClassType>())
			{
				return Component;
			}
		}

		return nullptr;
	}

	/** Generic function for finding the closest point using the given interface. */
	template <class InterfaceClass>
	bool GetClosestPointOnTarget(const UActorComponent* Target, const FVector& Point, FVector& OutClosestPoint);

	/** Specialization of the closest point function for the GrabTarget interface. */
	template <>
	bool GetClosestPointOnTarget<IUxtGrabTarget>(const UActorComponent* Target, const FVector& Point, FVector& OutClosestPoint)
	{
		return IUxtGrabTarget::Execute_GetClosestGrabPoint(Target, Point, OutClosestPoint);
	}

	/** Specialization of the closest point function for the TouchTarget interface. */
	template <>
	bool GetClosestPointOnTarget<IUxtTouchTarget>(const UActorComponent* Target, const FVector& Point, FVector& OutClosestPoint)
	{
		return IUxtTouchTarget::Execute_GetClosestTouchPoint(Target, Point, OutClosestPoint);
	}

	/** Generic function for finding the closest point using the given interface.
	 *  Checks if the target actually implements the interface.
	 */
	template <class InterfaceClass>
	bool GetClosestPointOnTargetChecked(const UActorComponent* Target, const FVector& Point, FVector& OutClosestPoint)
	{
		if (Target->Implements<typename InterfaceClass::UClassType>())
		{
			return GetClosestPointOnTarget<InterfaceClass>(Target, Point, OutClosestPoint);
		}
		return false;
	}

	/** Generic function for selecting the closest target to the given point, using the given interface for the distance function. */
	template <class InterfaceClass>
	bool FindClosestTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point, UActorComponent*& OutClosestTarget, FVector& OutClosestPointOnTarget, float& OutMinDistance)
	{
		float MinDistanceSqr = MAX_FLT;
		UActorComponent* ClosestTarget = nullptr;
		FVector ClosestPointOnTarget = FVector::ZeroVector;

		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (UActorComponent* Target = FindInterfaceComponent<InterfaceClass>(Overlap.GetActor()))
			{
				FVector PointOnTarget;
				if (GetClosestPointOnTarget<InterfaceClass>(Target, Point, PointOnTarget))
				{
					float DistanceSqr = (Point - PointOnTarget).SizeSquared();
					if (DistanceSqr < MinDistanceSqr)
					{
						MinDistanceSqr = DistanceSqr;
						ClosestTarget = Target;
						ClosestPointOnTarget = PointOnTarget;
					}
				}
			}
		}

		if (ClosestTarget != nullptr)
		{
			OutClosestTarget = ClosestTarget;
			OutClosestPointOnTarget = ClosestPointOnTarget;
			OutMinDistance = FMath::Sqrt(MinDistanceSqr);
			return true;
		}
		else
		{
			OutClosestTarget = nullptr;
			OutClosestPointOnTarget = FVector::ZeroVector;
			OutMinDistance = MAX_FLT;
			return false;
		}
	}
}


UUxtNearPointerComponent::UUxtNearPointerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick after physics so overlaps reflect the latest physics state.
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
}

void UUxtNearPointerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChangeFocusedGrabTarget(nullptr, FVector::ZeroVector);
	ChangeFocusedTouchTarget(nullptr, FVector::ZeroVector);

	Super::EndPlay(EndPlayReason);
}

void UUxtNearPointerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Don't update the focused target if locked
	if (!bFocusLocked)
	{
		const FVector ProximityCenter = GetGrabPointerTransform().GetLocation();
		const FVector GrabCenter = GetGrabPointerTransform().GetLocation();
		const FVector TouchCenter = GetTouchPointerTransform().GetLocation();

		TArray<FOverlapResult> Overlaps;
		/*bool HasBlockingOverlap = */ GetWorld()->OverlapMultiByChannel(Overlaps, ProximityCenter, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(ProximityRadius));

		FocusClosestGrabTarget(Overlaps, GrabCenter);
		FocusClosestTouchTarget(Overlaps, TouchCenter);
	}

	// Update focused grab target
	if (UObject* Target = FocusedGrabTargetWeak.Get())
	{
		if (Target->Implements<UUxtGrabTarget>())
		{
			FUxtPointerInteractionData Data;
			FTransform Transform = GetGrabPointerTransform();
			Data.Location = Transform.GetLocation();
			Data.Rotation = Transform.GetRotation();

			IUxtGrabTarget::Execute_OnUpdateGrabFocus(Target, (int32)GetUniqueID(), Data);

			if (bIsGrabbing)
			{
				IUxtGrabTarget::Execute_OnUpdateGrab(Target, (int32)GetUniqueID(), Data);
			}
		}
	}

	// Update focused touch target
	if (UObject* Target = FocusedTouchTargetWeak.Get())
	{
		if (Target->Implements<UUxtTouchTarget>())
		{
			FUxtPointerInteractionData Data;
			FTransform Transform = GetTouchPointerTransform();
			Data.Location = Transform.GetLocation();
			Data.Rotation = Transform.GetRotation();

			IUxtTouchTarget::Execute_OnUpdateTouchFocus(Target, (int32)GetUniqueID(), Data);
		}
	}
}

EControllerHand UUxtNearPointerComponent::GetHand() const
{
	return Hand;
}

void UUxtNearPointerComponent::SetHand(EControllerHand NewHand)
{
	Hand = NewHand;
}

ECollisionChannel UUxtNearPointerComponent::GetTraceChannel() const
{
	return TraceChannel;
}

void UUxtNearPointerComponent::SetTraceChannel(ECollisionChannel NewTraceChannel)
{
	TraceChannel = NewTraceChannel;
}

float UUxtNearPointerComponent::GetProximityRadius() const
{
	return ProximityRadius;
}

void UUxtNearPointerComponent::SetProximityRadius(float Radius)
{
	this->ProximityRadius = Radius;
}

float UUxtNearPointerComponent::GetTouchRadius() const
{
	return TouchRadius;
}

void UUxtNearPointerComponent::SetTouchRadius(float Radius)
{
	this->TouchRadius = Radius;
}

float UUxtNearPointerComponent::GetGrabRadius() const
{
	return GrabRadius;
}

void UUxtNearPointerComponent::SetGrabRadius(float Radius)
{
	this->GrabRadius = Radius;
}

UObject* UUxtNearPointerComponent::GetFocusedGrabTarget(FVector& OutClosestPointOnTarget) const
{
	if (auto Target = FocusedGrabTargetWeak.Get())
	{
		OutClosestPointOnTarget = ClosestGrabTargetPoint;
		return Target;
	}

	return nullptr;
}

UObject* UUxtNearPointerComponent::GetFocusedTouchTarget(FVector& OutClosestPointOnTarget) const
{
	if (auto Target = FocusedTouchTargetWeak.Get())
	{
		OutClosestPointOnTarget = ClosestTouchTargetPoint;
		return Target;
	}

	return nullptr;
}

bool UUxtNearPointerComponent::SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		FVector PointerLocation = GetGrabPointerTransform().GetLocation();
		FVector PointOnTarget;
		if (GetClosestPointOnTargetChecked<IUxtGrabTarget>(NewFocusedTarget, PointerLocation, PointOnTarget))
		{
			ChangeFocusedGrabTarget(NewFocusedTarget, PointOnTarget);
		}

		bFocusLocked = (NewFocusedTarget != nullptr && bEnableFocusLock);

		return true;
	}
	return false;
}

bool UUxtNearPointerComponent::SetFocusedTouchTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		FVector PointerLocation = GetTouchPointerTransform().GetLocation();
		FVector PointOnTarget;
		if (GetClosestPointOnTargetChecked<IUxtTouchTarget>(NewFocusedTarget, PointerLocation, PointOnTarget))
		{
			ChangeFocusedTouchTarget(NewFocusedTarget, PointOnTarget);
		}

		bFocusLocked = (NewFocusedTarget != nullptr && bEnableFocusLock);

		return true;
	}
	return false;
}

void UUxtNearPointerComponent::FocusClosestGrabTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point)
{
	UActorComponent* ClosestTarget;
	FVector ClosestPointOnTarget;
	float MinDistance;
	FindClosestTarget<IUxtGrabTarget>(Overlaps, Point, ClosestTarget, ClosestPointOnTarget, MinDistance);

	ChangeFocusedGrabTarget(ClosestTarget, ClosestPointOnTarget);
}

void UUxtNearPointerComponent::FocusClosestTouchTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point)
{
	UActorComponent* ClosestTarget;
	FVector ClosestPointOnTarget;
	float MinDistance;
	FindClosestTarget<IUxtTouchTarget>(Overlaps, Point, ClosestTarget, ClosestPointOnTarget, MinDistance);

	ChangeFocusedTouchTarget(ClosestTarget, ClosestPointOnTarget);
}

void UUxtNearPointerComponent::ChangeFocusedGrabTarget(UActorComponent* NewFocusedTarget, const FVector& NewClosestPointOnTarget)
{
	UObject* FocusedTarget = FocusedGrabTargetWeak.Get();

	// If focused target is unchanged, then update only the closest-point-on-target
	if (NewFocusedTarget == FocusedTarget)
	{
		ClosestGrabTargetPoint = NewClosestPointOnTarget;
	}
	else
	{
		// Update focused target
		if (FocusedTarget && FocusedTarget->Implements<UUxtGrabTarget>())
		{
			IUxtGrabTarget::Execute_OnExitGrabFocus(FocusedTarget, (int32)GetUniqueID());
		}

		if (NewFocusedTarget)
		{
			FocusedGrabTargetWeak = NewFocusedTarget;
			FocusedTarget = NewFocusedTarget;
		}
		else
		{
			// If the new target is null, use the default target instead.
			FocusedGrabTargetWeak = DefaultTargetWeak;
			FocusedTarget = DefaultTargetWeak.Get();
		}
		ClosestGrabTargetPoint = NewClosestPointOnTarget;

		if (FocusedTarget && FocusedTarget->Implements<UUxtGrabTarget>())
		{
			FUxtPointerInteractionData Data;
			FTransform Transform = GetGrabPointerTransform();
			Data.Location = Transform.GetLocation();
			Data.Rotation = Transform.GetRotation();

			IUxtGrabTarget::Execute_OnEnterGrabFocus(FocusedTarget, (int32)GetUniqueID(), Data);
		}
	}
}

void UUxtNearPointerComponent::ChangeFocusedTouchTarget(UActorComponent* NewFocusedTarget, const FVector& NewClosestPointOnTarget)
{
	UObject* FocusedTarget = FocusedTouchTargetWeak.Get();

	// If focused target is unchanged, then update only the closest-point-on-target
	if (NewFocusedTarget == FocusedTarget)
	{
		ClosestTouchTargetPoint = NewClosestPointOnTarget;
	}
	else
	{
		// Update focused target
		if (FocusedTarget && FocusedTarget->Implements<UUxtTouchTarget>())
		{
			IUxtTouchTarget::Execute_OnExitTouchFocus(FocusedTarget, (int32)GetUniqueID());
		}

		if (NewFocusedTarget)
		{
			FocusedTouchTargetWeak = NewFocusedTarget;
			FocusedTarget = NewFocusedTarget;
		}
		else
		{
			// If the new target is null, use the default target instead.
			FocusedTouchTargetWeak = DefaultTargetWeak;
			FocusedTarget = DefaultTargetWeak.Get();
		}
		ClosestTouchTargetPoint = NewClosestPointOnTarget;

		if (FocusedTarget && FocusedTarget->Implements<UUxtTouchTarget>())
		{
			FUxtPointerInteractionData Data;
			FTransform Transform = GetTouchPointerTransform();
			Data.Location = Transform.GetLocation();
			Data.Rotation = Transform.GetRotation();

			IUxtTouchTarget::Execute_OnEnterTouchFocus(FocusedTarget, (int32)GetUniqueID(), Data);
		}
	}
}

bool UUxtNearPointerComponent::GetFocusLocked() const
{
	return bFocusLocked;
}

void UUxtNearPointerComponent::SetFocusLocked(bool Value)
{
	bFocusLocked = Value;
}

bool UUxtNearPointerComponent::IsGrabbing() const
{
	return bIsGrabbing;
}

void UUxtNearPointerComponent::SetGrabbing(bool bValue)
{
	if (bIsGrabbing != bValue)
	{
		bIsGrabbing = bValue;

		if (auto Target = FocusedGrabTargetWeak.Get())
		{
			if (Target->Implements<UUxtGrabTarget>())
			{
				if (bIsGrabbing)
				{
					FUxtPointerInteractionData Data;
					FTransform Transform = GetGrabPointerTransform();
					Data.Location = Transform.GetLocation();
					Data.Rotation = Transform.GetRotation();

					IUxtGrabTarget::Execute_OnBeginGrab(Target, (int32)GetUniqueID(), Data);

					// Lock the grabbing pointer so the target remains focused as it moves.
					SetFocusLocked(true);
				}
				else
				{
					IUxtGrabTarget::Execute_OnEndGrab(Target, (int32)GetUniqueID());

					// Unlock the focused target selection
					SetFocusLocked(false);
				}

			}
		}
	}
}

FTransform UUxtNearPointerComponent::GetIndexTipTransform() const
{
	return IndexTipTransform;
}

void UUxtNearPointerComponent::SetIndexTipTransform(const FTransform& NewTransform)
{
	IndexTipTransform = NewTransform;
}

FTransform UUxtNearPointerComponent::GetThumbTipTransform() const
{
	return ThumbTipTransform;
}

void UUxtNearPointerComponent::SetThumbTipTransform(const FTransform& NewTransform)
{
	ThumbTipTransform = NewTransform;
}

UObject* UUxtNearPointerComponent::GetDefaultTarget() const
{
	return DefaultTargetWeak.Get();
}

void UUxtNearPointerComponent::SetDefaultTarget(UObject* NewDefaultTarget)
{
	DefaultTargetWeak = NewDefaultTarget;
}

FTransform UUxtNearPointerComponent::GetGrabPointerTransform() const
{
	// Use the midway point between the thumb and index finger tips for grab
	const float LerpFactor = 0.5f;
	FTransform GrabTransform = FTransform(
		FMath::Lerp(ThumbTipTransform.GetRotation(), IndexTipTransform.GetRotation(), LerpFactor),
		FMath::Lerp(ThumbTipTransform.GetLocation(), IndexTipTransform.GetLocation(), LerpFactor),
		FVector::OneVector
	);

	return GrabTransform;
}

FTransform UUxtNearPointerComponent::GetTouchPointerTransform() const
{
	return IndexTipTransform;
}
