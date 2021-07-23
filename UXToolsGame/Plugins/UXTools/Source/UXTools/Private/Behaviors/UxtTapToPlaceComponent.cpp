// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Behaviors/UxtTapToPlaceComponent.h"

#include "SceneManagement.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtInputSubsystem.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Utils/UxtInternalFunctionLibrary.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

namespace
{
	FQuat GetOrientationQuat(FVector Forward, FVector Up = FVector::UpVector)
	{
		FVector Right = FVector::CrossProduct(Up, Forward);
		Up = FVector::CrossProduct(Forward, Right);

		Forward.Normalize();
		Right.Normalize();
		Up.Normalize();

		if (Forward.SizeSquared() == 0 || Right.SizeSquared() == 0 || Up.SizeSquared() == 0)
		{
			return FQuat::Identity;
		}

		FMatrix Axes;
		Axes.SetAxes(&Forward, &Right, &Up);
		return FQuat(Axes);
	}

	float GetDefaultSurfaceNormalOffset(const USceneComponent* TargetComponent)
	{
		if (TargetComponent->IsA(UPrimitiveComponent::StaticClass()))
		{
			FVector BoundsCentre, BoxExtent;
			float SphereRadius;
			UKismetSystemLibrary::GetComponentBounds(TargetComponent, BoundsCentre, BoxExtent, SphereRadius);
			float PivotPointOffset = TargetComponent->GetComponentLocation().X - BoundsCentre.X;
			return BoxExtent.X - PivotPointOffset;
		}
		else
		{
			// Use the bounding box as approximation in case the Target Component is a parent of multiple meshes
			return UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(TargetComponent, TargetComponent->GetComponentTransform())
				.BoxExtent.X;
		}
	}
} // namespace

UUxtTapToPlaceComponent::UUxtTapToPlaceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

USceneComponent* UUxtTapToPlaceComponent::GetTargetComponent() const
{
	return Cast<USceneComponent>(TargetComponent.GetComponent(GetOwner()));
}

void UUxtTapToPlaceComponent::SetTargetComponent(USceneComponent* Target)
{
	TargetComponent.OverrideComponent = Target;
	if (Target)
	{
		DefaultSurfaceNormalOffset = GetDefaultSurfaceNormalOffset(Target);
	}
}

void UUxtTapToPlaceComponent::StartPlacement()
{
	if (!bIsBeingPlaced)
	{
		if (FocusLockedPointerWeak.IsValid())
		{
			FocusLockedPointerWeak->SetFocusLocked(false);
		}

		bIsBeingPlaced = true;
		UUxtInputSubsystem::RegisterHandler(this, UUxtFarHandler::StaticClass());
		OnBeginPlacing.Broadcast(this);
	}
}

void UUxtTapToPlaceComponent::EndPlacement()
{
	if (bIsBeingPlaced)
	{
		FocusLockedPointerWeak = nullptr;
		bIsBeingPlaced = false;
		UUxtInputSubsystem::UnregisterHandler(this, UUxtFarHandler::StaticClass());
		OnEndPlacing.Broadcast(this);
	}
}

void UUxtTapToPlaceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetTargetComponent() && GetOwner())
	{
		SetTargetComponent(GetOwner()->GetRootComponent());
	}

	if (USceneComponent* Target = GetTargetComponent())
	{
		DefaultSurfaceNormalOffset = GetDefaultSurfaceNormalOffset(Target);
	}
}

void UUxtTapToPlaceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsBeingPlaced)
	{
		if (USceneComponent* Target = GetTargetComponent())
		{
			FTransform OriginPose;
			switch (PlacementType)
			{
			case EUxtTapToPlaceMode::Head:
				OriginPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());
				break;
			case EUxtTapToPlaceMode::Hand:
				OriginPose = FTransform(FocusLockedPointerWeak->GetPointerOrientation(), FocusLockedPointerWeak->GetPointerOrigin());
			}

			// Ignore the target being placed and any other actors attached to it
			TArray<AActor*> ChildActors;
			GetOwner()->GetAttachedActors(ChildActors);
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(GetOwner());
			for (AActor* ChildActor : ChildActors)
			{
				QueryParams.AddIgnoredActor(ChildActor);
			}

			FHitResult Result;
			FVector Start = OriginPose.GetLocation();
			FVector End = Start + OriginPose.GetUnitAxis(EAxis::X) * MaxRaycastDistance;
			GetWorld()->LineTraceSingleByChannel(Result, Start, End, TraceChannel, QueryParams);

			FVector HitPosition = Start + OriginPose.GetUnitAxis(EAxis::X) * DefaultPlacementDistance;
			FVector Facing = OriginPose.GetLocation() - HitPosition;
			FVector Up = FVector::UpVector;

			if (Result.GetComponent())
			{
				// Add SurfaceNormalOffset so object is placed touching the surface, rather than overlapping with it
				HitPosition =
					Result.Location + Result.Normal * (bUseDefaultSurfaceNormalOffset ? DefaultSurfaceNormalOffset : SurfaceNormalOffset);
				const FVector TowardsOriginPose = Facing = OriginPose.GetLocation() - HitPosition;

				// Check if the target surface is flat, e.g. floor or ceiling
				const float HorizontalSurfaceCosineThreshold =
					FMath::Clamp(cosf(FMath::DegreesToRadians(HorizontalSurfaceThreshold)), 0.0f, THRESH_NORMALS_ARE_PARALLEL);
				const bool IsFloor = FVector::Coincident(Result.Normal, FVector::UpVector, HorizontalSurfaceCosineThreshold);
				const bool IsCeiling = FVector::Coincident(Result.Normal, FVector::DownVector, HorizontalSurfaceCosineThreshold);

				if (OrientationType == EUxtTapToPlaceOrientBehavior::AlignToSurface)
				{
					Facing = Result.Normal;

					if (KeepOrientationVertical)
					{
						// Horizontal surface - object sitting vertically on the surface, front should face the camera
						// Vertical or tilted surface - front of the object should align with the surface
						FVector ParallelToSurface =
							FVector::CrossProduct((IsFloor || IsCeiling) ? TowardsOriginPose : Result.Normal, FVector::UpVector);
						Facing = FVector::CrossProduct(FVector::UpVector, ParallelToSurface);
					}
					else if (IsFloor || IsCeiling)
					{
						// Horizontal surface - object resting flat on the surface, the top of the object should face:
						// - away from the camera when placing the object on a surface below the view level (i.e. floor)
						// - towards the camera when placing on a surface above the view level (i.e. ceiling)
						const FVector ParallelToSurface = FVector::CrossProduct(TowardsOriginPose, Result.Normal);
						Up = (IsFloor ? 1.0f : -1.0f) * FVector::CrossProduct(ParallelToSurface, Result.Normal);
					}
				}

				if ((OrientationType != EUxtTapToPlaceOrientBehavior::AlignToSurface) || KeepOrientationVertical)
				{
					// Perform a sweep query to prevent the target object from interpenetrating with other objects
					FHitResult HitResult;
					FCollisionShape CollisionShape;
					if (Target->IsA(UPrimitiveComponent::StaticClass()) && (Target->GetNumChildrenComponents() == 0))
					{
						// Use exact collision shape if the target is a UPrimitiveComponent
						CollisionShape = Cast<UPrimitiveComponent>(Target)->GetCollisionShape();
					}
					else
					{
						// Use the bounding box including all child components
						CollisionShape.SetBox(
							UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(Target, Target->GetComponentTransform()).BoxExtent);
					}

					if (GetWorld()->SweepSingleByChannel(
							HitResult, Start, Result.Location, GetOrientationQuat(Facing, Up), TraceChannel, CollisionShape, QueryParams))
					{
						FVector Displacement = UKismetMathLibrary::ProjectVectorOnToVector(HitResult.ImpactPoint - Result.Location, Facing);
						HitPosition =
							Result.Location + Displacement +
							HitResult.ImpactNormal * (bUseDefaultSurfaceNormalOffset ? DefaultSurfaceNormalOffset : SurfaceNormalOffset);
					}
				}
			}

			if (OrientationType == EUxtTapToPlaceOrientBehavior::MaintainOrientation)
			{
				Facing = Target->GetComponentRotation().Vector();
			}

			if (KeepOrientationVertical)
			{
				Facing.Z = 0;
				Up = FVector::UpVector;
			}

			FVector Position;
			FQuat Orientation;

			if (bInterpolatePose)
			{
				FVector TargetPos = Target->GetComponentLocation();
				FQuat TargetRot = Target->GetComponentRotation().Quaternion();

				float LerpAmount = LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime;
				LerpAmount = (LerpAmount > 1.0f) ? 1.0f : LerpAmount;
				Position = FMath::Lerp(TargetPos, HitPosition, LerpAmount);
				Orientation = FQuat::Slerp(TargetRot, GetOrientationQuat(Facing, Up), LerpAmount);
			}
			else
			{
				Position = HitPosition;
				Orientation = GetOrientationQuat(Facing, Up);
			}

			Target->SetWorldLocation(Position);
			Target->SetWorldRotation(Orientation);
		}
	}
}

void UUxtTapToPlaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Ensure that this is unregistered as a global event listener
	UUxtInputSubsystem::UnregisterHandler(this, UUxtFarHandler::StaticClass());

	Super::EndPlay(EndPlayReason);
}

bool UUxtTapToPlaceComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return UUxtInternalFunctionLibrary::IsPrimitiveEqualOrAttachedTo(GetTargetComponent(), Primitive);
}

bool UUxtTapToPlaceComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return bIsBeingPlaced ? true : UUxtInternalFunctionLibrary::IsPrimitiveEqualOrAttachedTo(GetTargetComponent(), Primitive);
}

void UUxtTapToPlaceComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (bIsBeingPlaced)
	{
		EndPlacement();
	}
	else if (Pointer == FocusLockedPointerWeak.Get())
	{
		StartPlacement();
	}
}

void UUxtTapToPlaceComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	bool bWasFocused = ++NumFocusedPointers > 1;
	OnBeginFocus.Broadcast(this, Pointer, bWasFocused);
}

void UUxtTapToPlaceComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

void UUxtTapToPlaceComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	bool bIsFocused = --NumFocusedPointers > 0;
	OnEndFocus.Broadcast(this, Pointer, bIsFocused);
}

void UUxtTapToPlaceComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!bIsBeingPlaced && !FocusLockedPointerWeak.IsValid())
	{
		Pointer->SetFocusLocked(true);
		FocusLockedPointerWeak = Pointer;
	}
}
