// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTestTargetComponent.h"

#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Components/PrimitiveComponent.h"
#include "Input/UxtNearPointerComponent.h"

namespace
{
	bool GetDefaultClosestPointOnPrimitive(
		const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface, float& OutDistanceSqr)
	{
		OutPointOnSurface = Point;
		OutDistanceSqr = -1.f;

		if (Primitive->IsRegistered() && Primitive->IsCollisionEnabled())
		{
			FVector ClosestPoint;
			float DistanceSqr = -1.f;

			if (Primitive->GetSquaredDistanceToCollision(Point, DistanceSqr, ClosestPoint))
			{
				OutPointOnSurface = ClosestPoint;
				OutDistanceSqr = DistanceSqr;
				return true;
			}
		}

		return false;
	}
} // namespace

void UTestGrabTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
}

bool UTestGrabTarget::CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UTestGrabTarget::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestGrabTarget::OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestGrabTarget::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

bool UTestGrabTarget::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return true;
}

void UTestGrabTarget::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginGrabCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(true);
	}
}

void UTestGrabTarget::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndGrabCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(false);
	}
}

void UTestPokeTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
	BeginPokeCount = 0;
	EndPokeCount = 0;
}

bool UTestPokeTarget::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return true;
}

EUxtPokeBehaviour UTestPokeTarget::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::FrontFace;
}

bool UTestPokeTarget::GetClosestPoint_Implementation(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	OutNormal = Primitive->GetComponentTransform().GetUnitAxis(EAxis::X);

	float NotUsed;
	return GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed);
}

bool UTestPokeTarget::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UTestPokeTarget::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestPokeTarget::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestPokeTarget::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

void UTestPokeTarget::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginPokeCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(true);
	}
}

void UTestPokeTarget::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndPokeCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(false);
	}
}
