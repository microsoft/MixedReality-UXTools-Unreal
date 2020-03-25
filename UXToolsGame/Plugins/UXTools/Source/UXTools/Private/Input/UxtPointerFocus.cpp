// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtPointerFocus.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtTouchTarget.h"

#include "Components/PrimitiveComponent.h"


bool FUxtPointerFocusSearchResult::IsValid() const
{
	return (Target != nullptr) && (Primitive != nullptr);
}


const FVector& FUxtPointerFocus::GetClosestTargetPoint() const
{
	return ClosestTargetPoint;
}

UObject* FUxtPointerFocus::GetFocusedTarget() const
{
	return FocusedTargetWeak.Get();
}

UObject* FUxtPointerFocus::GetFocusedTargetChecked() const
{
	if (UObject* Target = FocusedTargetWeak.Get())
	{
		if (ImplementsTargetInterface(Target))
		{
			return Target;
		}
	}
	return nullptr;
}

void FUxtPointerFocus::SelectClosestTarget(UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, const TArray<FOverlapResult>& Overlaps)
{
	FUxtPointerFocusSearchResult Result = FindClosestTarget(Overlaps, PointerTransform.GetLocation());
	if (Result.IsValid())
	{
		SetFocus(Pointer, PointerTransform, Result.Target, Result.Primitive, Result.ClosestPointOnTarget);
	}
	else
	{
		SetFocus(Pointer, PointerTransform, nullptr, nullptr, FVector::ZeroVector);
	}
}

void FUxtPointerFocus::SelectClosestPointOnTarget(
	UUxtNearPointerComponent* Pointer,
	const FTransform& PointerTransform,
	UActorComponent* NewTarget)
{
	if (NewTarget)
	{
		bool IsValidTarget = ensureMsgf(ImplementsTargetInterface(NewTarget), TEXT("Target object must implement %s interface for finding the closest point"), *GetInterfaceClass()->GetName());

		if (IsValidTarget)
		{
			const FVector Point = PointerTransform.GetLocation();

			FUxtPointerFocusSearchResult Result = FindClosestPointOnComponent(NewTarget, PointerTransform.GetLocation());
			if (Result.IsValid())
			{
				SetFocus(Pointer, PointerTransform, Result.Target, Result.Primitive, Result.ClosestPointOnTarget);
			}
		}
	}
	else
	{
		ClearFocus(Pointer);
	}
}

void FUxtPointerFocus::ClearFocus(UUxtNearPointerComponent* Pointer)
{
	UObject* FocusedTarget = FocusedTargetWeak.Get();
	if (FocusedTarget && ImplementsTargetInterface(FocusedTarget))
	{
		RaiseExitFocusEvent(FocusedTarget, Pointer);
	}

	FocusedTargetWeak.Reset();
	FocusedPrimitiveWeak.Reset();
	ClosestTargetPoint = FVector::ZeroVector;
}

void FUxtPointerFocus::UpdateFocus(UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform) const
{
	if (UObject* FocusedTarget = GetFocusedTargetChecked())
	{
		FUxtPointerInteractionData Data = { PointerTransform.GetLocation(), PointerTransform.GetRotation() };

		RaiseUpdateFocusEvent(FocusedTarget, Pointer, Data);
	}
}

UObject* FUxtPointerFocus::GetDefaultTarget() const
{
	return DefaultTargetWeak.Get();
}

void FUxtPointerFocus::SetDefaultTarget(UObject* NewDefaultTarget)
{
	DefaultTargetWeak = NewDefaultTarget;
}

void FUxtPointerFocus::SetFocus(
	UUxtNearPointerComponent* Pointer,
	const FTransform& PointerTransform,
	UObject* NewTarget,
	UPrimitiveComponent* NewPrimitive,
	const FVector& NewClosestPointOnTarget)
{
	UObject* FocusedTarget = FocusedTargetWeak.Get();
	UPrimitiveComponent* FocusedPrimitive = FocusedPrimitiveWeak.Get();

	// If focused target is unchanged, then update only the closest-point-on-target
	if (NewTarget == FocusedTarget && NewPrimitive == FocusedPrimitive)
	{
		ClosestTargetPoint = NewClosestPointOnTarget;
	}
	else
	{
		// Update focused target
		if (FocusedTarget && ImplementsTargetInterface(FocusedTarget))
		{
			RaiseExitFocusEvent(FocusedTarget, Pointer);
		}

		FocusedTarget = NewTarget;
		FocusedTargetWeak = NewTarget;
		FocusedPrimitiveWeak = NewPrimitive;
		ClosestTargetPoint = NewClosestPointOnTarget;

		if (FocusedTarget && ImplementsTargetInterface(FocusedTarget))
		{
			FUxtPointerInteractionData Data = { PointerTransform.GetLocation(), PointerTransform.GetRotation() };

			RaiseEnterFocusEvent(FocusedTarget, Pointer, Data);
		}
	}
}

/** Find a component of the actor that implements the given interface type. */
UActorComponent* FUxtPointerFocus::FindInterfaceComponent(AActor* Owner) const
{
	for (UActorComponent* Component : Owner->GetComponents())
	{
		if (ImplementsTargetInterface(Component))
		{
			return Component;
		}
	}
	return nullptr;
}

FUxtPointerFocusSearchResult FUxtPointerFocus::FindClosestTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point) const
{
	float MinDistanceSqr = MAX_FLT;
	UActorComponent* ClosestTarget = nullptr;
	UPrimitiveComponent* Primitive = nullptr;
	FVector ClosestPointOnTarget = FVector::ZeroVector;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (UActorComponent* Target = FindInterfaceComponent(Overlap.GetActor()))
		{
			FVector PointOnTarget;
			if (GetClosestPointOnTarget(Target, Overlap.GetComponent(), Point, PointOnTarget))
			{
				float DistanceSqr = (Point - PointOnTarget).SizeSquared();
				if (DistanceSqr < MinDistanceSqr)
				{
					MinDistanceSqr = DistanceSqr;
					ClosestTarget = Target;
					Primitive = Overlap.GetComponent();
					ClosestPointOnTarget = PointOnTarget;
				}
			}
		}
	}

	if (ClosestTarget != nullptr)
	{
		return { ClosestTarget, Primitive, ClosestPointOnTarget, FMath::Sqrt(MinDistanceSqr) };
	}
	else
	{
		return { nullptr, nullptr, FVector::ZeroVector, MAX_FLT };
	}
}

FUxtPointerFocusSearchResult FUxtPointerFocus::FindClosestPointOnComponent(UActorComponent* Target, const FVector& Point) const
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Target->GetOwner()->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	UPrimitiveComponent* ClosestPrimitive = nullptr;
	FVector ClosestPoint = FVector::ZeroVector;
	float MinDistanceSqr = -1.f;
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		FVector PointOnPrimitive;
		GetClosestPointOnTarget(Target, Primitive, Point, PointOnPrimitive);

		float DistanceSqr = FVector::DistSquared(Point, PointOnPrimitive);
		if (!ClosestPrimitive || DistanceSqr < MinDistanceSqr)
		{
			ClosestPrimitive = Primitive;
			MinDistanceSqr = DistanceSqr;
			ClosestPoint = PointOnPrimitive;

			if (MinDistanceSqr <= KINDA_SMALL_NUMBER)
			{
				// Best result to be expected.
				break;
			}
		}
	}

	if (ClosestPrimitive != nullptr)
	{
		return { Target, ClosestPrimitive, ClosestPoint, FMath::Sqrt(MinDistanceSqr) };
	}
	else
	{
		return { nullptr, nullptr, FVector::ZeroVector, MAX_FLT };
	}
}


void FUxtGrabPointerFocus::BeginGrab(UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		FUxtPointerInteractionData Data = { PointerTransform.GetLocation(), PointerTransform.GetRotation() };

		IUxtGrabTarget::Execute_OnBeginGrab(Target, Pointer, Data);
	}

	bIsGrabbing = true;
}

void FUxtGrabPointerFocus::UpdateGrab(UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform)
{
	if (UObject* Target = GetFocusedTargetChecked())
	{
		FUxtPointerInteractionData Data = { PointerTransform.GetLocation(), PointerTransform.GetRotation() };

		IUxtGrabTarget::Execute_OnUpdateGrab(Target, Pointer, Data);
	}
}

void FUxtGrabPointerFocus::EndGrab(UUxtNearPointerComponent* Pointer)
{
	bIsGrabbing = false;

	if (UObject* Target = GetFocusedTargetChecked())
	{
		IUxtGrabTarget::Execute_OnEndGrab(Target, Pointer);
	}
}

bool FUxtGrabPointerFocus::IsGrabbing() const
{
	return bIsGrabbing;
}

UClass* FUxtGrabPointerFocus::GetInterfaceClass() const
{
	return UUxtGrabTarget::StaticClass();
}

bool FUxtGrabPointerFocus::ImplementsTargetInterface(UObject* Target) const
{
	return Target->Implements<UUxtGrabTarget>();
}

bool FUxtGrabPointerFocus::GetClosestPointOnTarget(const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint) const
{
	return IUxtGrabTarget::Execute_GetClosestGrabPoint(Target, Primitive, Point, OutClosestPoint);
}

void FUxtGrabPointerFocus::RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) const
{
	IUxtGrabTarget::Execute_OnEnterGrabFocus(Target, Pointer, Data);
}

void FUxtGrabPointerFocus::RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) const
{
	IUxtGrabTarget::Execute_OnUpdateGrabFocus(Target, Pointer, Data);
}

void FUxtGrabPointerFocus::RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtGrabTarget::Execute_OnExitGrabFocus(Target, Pointer);
}


UClass* FUxtTouchPointerFocus::GetInterfaceClass() const
{
	return UUxtTouchTarget::StaticClass();
}

bool FUxtTouchPointerFocus::ImplementsTargetInterface(UObject* Target) const
{
	return Target->Implements<UUxtTouchTarget>();
}

bool FUxtTouchPointerFocus::GetClosestPointOnTarget(const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint) const
{
	return IUxtTouchTarget::Execute_GetClosestTouchPoint(Target, Primitive, Point, OutClosestPoint);
}

void FUxtTouchPointerFocus::RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) const
{
	IUxtTouchTarget::Execute_OnEnterTouchFocus(Target, Pointer, Data);
}

void FUxtTouchPointerFocus::RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) const
{
	IUxtTouchTarget::Execute_OnUpdateTouchFocus(Target, Pointer, Data);
}

void FUxtTouchPointerFocus::RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const
{
	IUxtTouchTarget::Execute_OnExitTouchFocus(Target, Pointer);
}
