// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtFarPointerComponent.h"

#include "CollisionQueryParams.h"
#include "UXTools.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Input/UxtInputSubsystem.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtFunctionLibrary.h"

UUxtFarPointerComponent::UUxtFarPointerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Finder(TEXT("/UXTools/Materials/MPC_UXSettings"));
	ParameterCollection = Finder.Object;
	// Tick before controls
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
}

void UUxtFarPointerComponent::SetActive(bool bNewActive, bool bReset)
{
	Super::SetActive(bNewActive, bReset);

	if (!bNewActive)
	{
		SetEnabled(false);
	}
}

void UUxtFarPointerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Obtain new pointer origin and orientation
	FQuat NewOrientation;
	FVector NewOrigin;
	const bool bIsTracked = UUxtHandTrackingFunctionLibrary::GetHandPointerPose(Hand, NewOrientation, NewOrigin);

	if (bIsTracked)
	{
		OnPointerPoseUpdated(NewOrientation, NewOrigin);
		UpdateParameterCollection(GetHitPoint());
		bool bNewPressed;
		if (UUxtHandTrackingFunctionLibrary::GetIsHandSelectPressed(Hand, bNewPressed))
		{
			SetPressed(bNewPressed);
		}

		FQuat WristOrientation;
		FVector WristLocation;
		float WristRadius;
		if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::Wrist, WristOrientation, WristLocation, WristRadius))
		{
			ControllerOrientation = WristOrientation;
		}
	}

	SetEnabled(bIsTracked);
}

void UUxtFarPointerComponent::SetFocusLocked(bool bLocked)
{
	Super::SetFocusLocked(bLocked);

	if (bLocked)
	{
		// Store current hit info in hit primitive space
		if (UPrimitiveComponent* HitPrimitive = GetHitPrimitive())
		{
			const FTransform WorldToTarget = HitPrimitive->GetComponentTransform();
			HitPointLocal = WorldToTarget.InverseTransformPosition(HitPoint);
			HitNormalLocal = WorldToTarget.InverseTransformVectorNoScale(HitNormal);
		}
	}
}

UObject* UUxtFarPointerComponent::GetFocusTarget() const
{
	return GetFarTarget();
}

FTransform UUxtFarPointerComponent::GetCursorTransform() const
{
	FTransform Transform;
	Transform.SetLocation(GetHitPoint());
	Transform.SetRotation(GetHitNormal().Rotation().Quaternion());

	return Transform;
}

// Finds the far target a primitive belongs to, if any
static UObject* FindFarTarget(UPrimitiveComponent* Primitive)
{
	if (Primitive)
	{
		for (UActorComponent* Component : Primitive->GetOwner()->GetComponents())
		{
			if (Component->Implements<UUxtFarTarget>() && IUxtFarTarget::Execute_IsFarFocusable(Component, Primitive))
			{
				return Component;
			}
		}
	}

	return nullptr;
}

void UUxtFarPointerComponent::OnPointerPoseUpdated(const FQuat& NewOrientation, const FVector& NewOrigin)
{
	PointerOrientation = NewOrientation;
	PointerOrigin = NewOrigin;

	UPrimitiveComponent* OldPrimitive = GetHitPrimitive();
	UPrimitiveComponent* NewPrimitive;

	if (bFocusLocked)
	{
		NewPrimitive = OldPrimitive;

		if (NewPrimitive)
		{
			// Update hit point and normal with the latest primitive transform
			FTransform TargetTransform = NewPrimitive->GetComponentTransform();
			HitPoint = TargetTransform.TransformPosition(HitPointLocal);
			HitNormal = TargetTransform.TransformVectorNoScale(HitNormalLocal);
		}
		else
		{
			HitPrimitiveWeak = nullptr;
			FarTargetWeak = nullptr;
			bFocusLocked = false;
		}
	}
	else
	{
		// Line trace to find new primitive
		FHitResult Hit;
		const FVector Forward = PointerOrientation.GetForwardVector();
		FVector Start = PointerOrigin + Forward * RayStartOffset;
		FVector End = Start + Forward * RayLength;
		GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel);

		NewPrimitive = Hit.GetComponent();

		if (NewPrimitive != OldPrimitive)
		{
			// Raise focus exit on old target
			if (OldPrimitive)
			{
				if (GetFarTarget())
				{
					UUxtInputSubsystem::RaiseExitFarFocus(OldPrimitive, this);
				}
			}

			// Update hit primitive and far target
			HitPrimitiveWeak = NewPrimitive;
			FarTargetWeak = FindFarTarget(NewPrimitive);
		}

		// Update cached hit info
		if (NewPrimitive)
		{
			HitPoint = Hit.Location;
			HitNormal = Hit.Normal;
		}
		else
		{
			HitPoint = End;
			HitNormal = -Forward;
		}
	}

	// Raise events on current target
	if (NewPrimitive)
	{
		if (GetFarTarget())
		{
			// Focus events
			if (NewPrimitive == OldPrimitive)
			{
				UUxtInputSubsystem::RaiseUpdatedFarFocus(NewPrimitive, this);
			}
			else
			{
				UUxtInputSubsystem::RaiseEnterFarFocus(NewPrimitive, this);
			}

			// Dragged event
			if (IsPressed())
			{
				UUxtInputSubsystem::RaiseFarDragged(NewPrimitive, this);
			}
		}
	}
}

void UUxtFarPointerComponent::SetPressed(bool bNewPressed)
{
	if (bPressed != bNewPressed)
	{
		bPressed = bNewPressed;

		UPrimitiveComponent* TargetPrimitive = GetHitPrimitive();

		if (bPressed)
		{
			UUxtInputSubsystem::RaiseFarPressed(TargetPrimitive, this);
		}
		else
		{
			UUxtInputSubsystem::RaiseFarReleased(TargetPrimitive, this);
		}
	}
}

void UUxtFarPointerComponent::SetEnabled(bool bNewEnabled)
{
	if (bEnabled != bNewEnabled)
	{
		bEnabled = bNewEnabled;

		if (bEnabled)
		{
			OnFarPointerEnabled.Broadcast(this);
		}
		else
		{
			// Release pointer if it was pressed
			SetPressed(false);

			// Raise focus exit on the current target
			UPrimitiveComponent* TargetPrimitive = GetHitPrimitive();
			if (GetFarTarget() && TargetPrimitive)
			{
				UUxtInputSubsystem::RaiseExitFarFocus(TargetPrimitive, this);
			}

			HitPrimitiveWeak = nullptr;
			FarTargetWeak = nullptr;
			bFocusLocked = false;

			OnFarPointerDisabled.Broadcast(this);
		}
	}
}

FVector UUxtFarPointerComponent::GetPointerOrigin() const
{
	return PointerOrigin;
}

FQuat UUxtFarPointerComponent::GetPointerOrientation() const
{
	return PointerOrientation;
}

FQuat UUxtFarPointerComponent::GetControllerOrientation() const
{
	return ControllerOrientation;
}

FVector UUxtFarPointerComponent::GetRayStart() const
{
	return PointerOrigin + PointerOrientation.GetForwardVector() * RayStartOffset;
}

UPrimitiveComponent* UUxtFarPointerComponent::GetHitPrimitive() const
{
	return HitPrimitiveWeak.Get();
}

FVector UUxtFarPointerComponent::GetHitPoint() const
{
	return HitPoint;
}

FVector UUxtFarPointerComponent::GetHitNormal() const
{
	return HitNormal;
}

bool UUxtFarPointerComponent::IsPressed() const
{
	return bPressed;
}

bool UUxtFarPointerComponent::IsEnabled() const
{
	return bEnabled;
}

UObject* UUxtFarPointerComponent::GetFarTarget() const
{
	return FarTargetWeak.Get();
}

void UUxtFarPointerComponent::UpdateParameterCollection(FVector IndexTipPosition)
{
	if (ParameterCollection)
	{
		UMaterialParameterCollectionInstance* ParameterCollectionInstance = GetWorld()->GetParameterCollectionInstance(ParameterCollection);
		static FName ParameterNames[] = {"LeftPointerPosition", "RightPointerPosition"};
		FName ParameterName = Hand == EControllerHand::Left ? ParameterNames[0] : ParameterNames[1];
		const bool bFoundParameter = ParameterCollectionInstance->SetVectorParameterValue(ParameterName, IndexTipPosition);

		if (!bFoundParameter)
		{
			UE_LOG(
				UXTools, Warning, TEXT("Unable to find %s parameter in material parameter collection %s."), *ParameterName.ToString(),
				*ParameterCollection->GetPathName());
		}
	}
}
