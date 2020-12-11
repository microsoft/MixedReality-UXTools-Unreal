// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtPointerFocus.h"

#include "Components/PrimitiveComponent.h"
#include "Input/UxtInputSubsystem.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabHandler.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"
#include "VisualLogger/VisualLogger.h"

bool FUxtPointerFocusSearchResult::IsValid() const
{
	return (Target != nullptr) && (Primitive != nullptr);
}

const FVector& FUxtPointerFocus::GetClosestTargetPoint() const
{
	return ClosestTargetPoint;
}

const FVector& FUxtPointerFocus::GetClosestTargetNormal() const
{
	return ClosestTargetNormal;
}

UObject* FUxtPointerFocus::GetFocusedTarget() const
{
	return FocusedTargetWeak.Get();
}

UPrimitiveComponent* FUxtPointerFocus::GetFocusedPrimitive() const
{
	return FocusedPrimitiveWeak.Get();
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

void FUxtPointerFocus::SelectClosestTarget(
	UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, const TArray<FOverlapResult>& Overlaps)
{
	FUxtPointerFocusSearchResult Result = FindClosestTarget(Overlaps, PointerTransform.GetLocation());
	SetFocus(Pointer, PointerTransform, Result);
}

void FUxtPointerFocus::UpdateClosestTarget(const FTransform& PointerTransform)
{
	if (UActorComponent* ClosesTarget = Cast<UActorComponent>(FocusedTargetWeak.Get()))
	{
		if (UPrimitiveComponent* Primitive = FocusedPrimitiveWeak.Get())
		{
			GetClosestPointOnTarget(ClosesTarget, Primitive, PointerTransform.GetLocation(), ClosestTargetPoint, ClosestTargetNormal);

#if ENABLE_VISUAL_LOG
			VLogFocus(Primitive, ClosestTargetPoint, ClosestTargetNormal, true);
#endif // ENABLE_VISUAL_LOG
		}
	}
}

void FUxtPointerFocus::SelectClosestPointOnTarget(
	UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, UActorComponent* NewTarget)
{
	if (NewTarget)
	{
		bool IsValidTarget = ensureMsgf(
			ImplementsTargetInterface(NewTarget), TEXT("Target object must implement %s interface for finding the closest point"),
			*GetInterfaceClass()->GetName());

		if (IsValidTarget)
		{
			const FVector Point = PointerTransform.GetLocation();

			FUxtPointerFocusSearchResult Result = FindClosestPointOnComponent(NewTarget, PointerTransform.GetLocation());
			if (Result.IsValid())
			{
				SetFocus(Pointer, PointerTransform, Result);
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
	UPrimitiveComponent* FocusedTarget = GetFocusedPrimitive();
	if (GetFocusedTargetChecked() && FocusedTarget)
	{
		RaiseExitFocusEvent(FocusedTarget, Pointer);
	}

	FocusedTargetWeak.Reset();
	FocusedPrimitiveWeak.Reset();
	ClosestTargetPoint = FVector::ZeroVector;
	ClosestTargetNormal = FVector::ForwardVector;
}

void FUxtPointerFocus::UpdateFocus(UUxtNearPointerComponent* Pointer) const
{
	UPrimitiveComponent* FocusedTarget = GetFocusedPrimitive();
	if (GetFocusedTargetChecked() && FocusedTarget)
	{
		RaiseUpdateFocusEvent(FocusedTarget, Pointer);
	}
}

void FUxtPointerFocus::SetFocus(
	UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, const FUxtPointerFocusSearchResult& FocusResult)
{
	UObject* FocusedTarget = FocusedTargetWeak.Get();
	UPrimitiveComponent* FocusedPrimitive = FocusedPrimitiveWeak.Get();

	// If focused target is unchanged, then update only the closest-point-on-target
	if (FocusResult.Target == FocusedTarget && FocusResult.Primitive == FocusedPrimitive)
	{
		ClosestTargetPoint = FocusResult.ClosestPointOnTarget;
		ClosestTargetNormal = FocusResult.Normal;
	}
	else
	{
		// Update focused target
		if (FocusedPrimitive && FocusedTarget && ImplementsTargetInterface(FocusedTarget))
		{
			RaiseExitFocusEvent(FocusedPrimitive, Pointer);
		}

		FocusedTarget = FocusResult.Target;
		FocusedPrimitive = FocusResult.Primitive;
		FocusedTargetWeak = FocusResult.Target;
		FocusedPrimitiveWeak = FocusResult.Primitive;
		ClosestTargetPoint = FocusResult.ClosestPointOnTarget;
		ClosestTargetNormal = FocusResult.Normal;

		if (FocusedPrimitive && FocusedTarget && ImplementsTargetInterface(FocusedTarget))
		{
			RaiseEnterFocusEvent(FocusedPrimitive, Pointer);
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
	UPrimitiveComponent* ClosestPrimitive = nullptr;
	FVector ClosestPointOnTarget = FVector::ZeroVector;
	FVector ClosestNormal = FVector::ForwardVector;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		UPrimitiveComponent* Primitive = Overlap.GetComponent();

		if (Actor && Primitive)
		{
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (ImplementsTargetInterface(Component))
				{
					FVector PointOnTarget;
					FVector Normal;

					if (GetClosestPointOnTarget(Component, Primitive, Point, PointOnTarget, Normal))
					{
						float DistanceSqr = (Point - PointOnTarget).SizeSquared();
						if (DistanceSqr < MinDistanceSqr)
						{
							MinDistanceSqr = DistanceSqr;
							ClosestTarget = Component;
							ClosestPrimitive = Primitive;
							ClosestPointOnTarget = PointOnTarget;
							ClosestNormal = Normal;
						}

#if ENABLE_VISUAL_LOG
						VLogFocus(Overlap.GetComponent(), PointOnTarget, Normal, false);
#endif // ENABLE_VISUAL_LOG

						// We keep the first target component that takes ownership of the primitive.
						break;
					}
				}
			}
		}
	}

	if (ClosestTarget != nullptr)
	{
#if ENABLE_VISUAL_LOG
		VLogFocus(ClosestPrimitive, ClosestPointOnTarget, ClosestNormal, true);
#endif // ENABLE_VISUAL_LOG

		return {ClosestTarget, ClosestPrimitive, ClosestPointOnTarget, ClosestNormal, FMath::Sqrt(MinDistanceSqr)};
	}
	else
	{
		return {nullptr, nullptr, FVector::ZeroVector, FVector::ForwardVector, MAX_FLT};
	}
}

FUxtPointerFocusSearchResult FUxtPointerFocus::FindClosestPointOnComponent(UActorComponent* Target, const FVector& Point) const
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	Target->GetOwner()->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	UPrimitiveComponent* ClosestPrimitive = nullptr;
	FVector ClosestPoint = FVector::ZeroVector;
	FVector ClosestNormal = FVector::ForwardVector;
	float MinDistanceSqr = -1.f;
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		FVector PointOnPrimitive;
		FVector Normal;
		GetClosestPointOnTarget(Target, Primitive, Point, PointOnPrimitive, Normal);

		float DistanceSqr = FVector::DistSquared(Point, PointOnPrimitive);
		if (!ClosestPrimitive || DistanceSqr < MinDistanceSqr)
		{
			ClosestPrimitive = Primitive;
			MinDistanceSqr = DistanceSqr;
			ClosestPoint = PointOnPrimitive;
			ClosestNormal = Normal;

			if (MinDistanceSqr <= KINDA_SMALL_NUMBER)
			{
				// Best result to be expected.
				break;
			}
		}
	}

	if (ClosestPrimitive != nullptr)
	{
		return {Target, ClosestPrimitive, ClosestPoint, ClosestNormal, FMath::Sqrt(MinDistanceSqr)};
	}
	else
	{
		return {nullptr, nullptr, FVector::ZeroVector, FVector::ForwardVector, MAX_FLT};
	}
}

#if ENABLE_VISUAL_LOG
void FUxtPointerFocus::VLogFocus(
	const UPrimitiveComponent* Primitive, const FVector& PointOnTarget, const FVector& Normal, bool bIsFocus) const
{
	if (!FVisualLogger::IsRecording())
	{
		return;
	}

	if (const UObject* VLogOwner = VLogOwnerWeak.Get())
	{
		const FColor FocusColor = bIsFocus ? VLogColor : FColor(VLogColor.R / 2, VLogColor.G / 2, VLogColor.B / 2, VLogColor.A);

		FBoxSphereBounds PrimitiveBounds = Primitive->CalcLocalBounds();
		UE_VLOG_OBOX(
			VLogOwner, VLogCategory, Verbose, PrimitiveBounds.GetBox(), Primitive->GetComponentTransform().ToMatrixWithScale(), FocusColor,
			TEXT("%s | %s"), *Primitive->GetOwner()->GetName(), *Primitive->GetName());
	}
}
#endif // ENABLE_VISUAL_LOG

void FUxtGrabPointerFocus::BeginGrab(UUxtNearPointerComponent* Pointer)
{
	if (UPrimitiveComponent* Target = GetFocusedPrimitive())
	{
		UUxtInputSubsystem::RaiseBeginGrab(Target, Pointer);
	}

	bIsGrabbing = true;
}

void FUxtGrabPointerFocus::UpdateGrab(UUxtNearPointerComponent* Pointer)
{
	if (UPrimitiveComponent* Target = GetFocusedPrimitive())
	{
		UUxtInputSubsystem::RaiseUpdateGrab(Target, Pointer);
	}
}

void FUxtGrabPointerFocus::EndGrab(UUxtNearPointerComponent* Pointer)
{
	bIsGrabbing = false;

	if (UPrimitiveComponent* Target = GetFocusedPrimitive())
	{
		UUxtInputSubsystem::RaiseEndGrab(Target, Pointer);
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

bool FUxtGrabPointerFocus::GetClosestPointOnTarget(
	const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint,
	FVector& OutNormal) const
{
	float NotUsed;
	if (FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed))
	{
		if (OutClosestPoint == Point)
		{
			OutNormal = Point - Primitive->GetComponentLocation();
		}
		else
		{
			OutNormal = Point - OutClosestPoint;
		}

		OutNormal.Normalize();
		return true;
	}
	return false;
}

void FUxtGrabPointerFocus::RaiseEnterFocusEvent(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer) const
{
	UUxtInputSubsystem::RaiseEnterGrabFocus(Target, Pointer);
}

void FUxtGrabPointerFocus::RaiseUpdateFocusEvent(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer) const
{
	UUxtInputSubsystem::RaiseUpdateGrabFocus(Target, Pointer);
}

void FUxtGrabPointerFocus::RaiseExitFocusEvent(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer) const
{
	UUxtInputSubsystem::RaiseExitGrabFocus(Target, Pointer);
}

void FUxtPokePointerFocus::BeginPoke(UUxtNearPointerComponent* Pointer)
{
	if (UPrimitiveComponent* Target = GetFocusedPrimitive())
	{
		UUxtInputSubsystem::RaiseBeginPoke(Target, Pointer);
	}

	bIsPoking = true;
}

void FUxtPokePointerFocus::UpdatePoke(UUxtNearPointerComponent* Pointer)
{
	if (UPrimitiveComponent* Target = GetFocusedPrimitive())
	{
		UUxtInputSubsystem::RaiseUpdatePoke(Target, Pointer);
	}
}

void FUxtPokePointerFocus::EndPoke(UUxtNearPointerComponent* Pointer)
{
	if (UPrimitiveComponent* Target = GetFocusedPrimitive())
	{
		UUxtInputSubsystem::RaiseEndPoke(Target, Pointer);
	}

	bIsPoking = false;
}

bool FUxtPokePointerFocus::IsPoking() const
{
	return bIsPoking;
}

UClass* FUxtPokePointerFocus::GetInterfaceClass() const
{
	return UUxtPokeTarget::StaticClass();
}

bool FUxtPokePointerFocus::ImplementsTargetInterface(UObject* Target) const
{
	return Target->Implements<UUxtPokeTarget>();
}

bool FUxtPokePointerFocus::GetClosestPointOnTarget(
	const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint,
	FVector& OutNormal) const
{
	return IUxtPokeTarget::Execute_IsPokeFocusable(Target, Primitive) &&
		   IUxtPokeTarget::Execute_GetClosestPoint(Target, Primitive, Point, OutClosestPoint, OutNormal);
}

void FUxtPokePointerFocus::RaiseEnterFocusEvent(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer) const
{
	UUxtInputSubsystem::RaiseEnterPokeFocus(Target, Pointer);
}

void FUxtPokePointerFocus::RaiseUpdateFocusEvent(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer) const
{
	UUxtInputSubsystem::RaiseUpdatePokeFocus(Target, Pointer);
}

void FUxtPokePointerFocus::RaiseExitFocusEvent(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer) const
{
	UUxtInputSubsystem::RaiseExitPokeFocus(Target, Pointer);
}
