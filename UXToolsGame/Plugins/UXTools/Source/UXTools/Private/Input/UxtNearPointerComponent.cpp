// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtNearPointerComponent.h"

#include "UXTools.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Input/UxtPointerFocus.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeTarget.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogUxtGrabPointer, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogUxtPokePointer, Log, All);

namespace
{
	bool IsBoxShape(UPrimitiveComponent& Primitive)
	{
		if (UBodySetup* BodySetup = Primitive.GetBodySetup())
		{
			FKAggregateGeom AggGeom = BodySetup->AggGeom;
			const int32 ElementCount = AggGeom.GetElementCount();
			if (ElementCount != 0 && ElementCount == AggGeom.BoxElems.Num())
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Used for checking on which side of a front face pokable's front face the pointer
	 * sphere is. This is important as BeginPoke can only be called if the pointer sphere
	 * was not behind in the previous tick and is now behind in this tick.
	 *
	 * This function assumes that the given primitive has a box collider.
	 */
	bool IsBehindFrontFace(UPrimitiveComponent* Primitive, FVector PointerPosition, float Radius)
	{
		check(Primitive != nullptr);

		// Front face pokables must have a box-shaped collider
		if (!IsBoxShape(*Primitive))
		{
			UE_LOG(
				UXTools, Warning,
				TEXT("Primitive %s has some collision "
					 "shape other than box"),
				*Primitive->GetFName().ToString());
			return false;
		}

		const FMatrix ComponentTransform = Primitive->GetComponentTransform().ToMatrixWithScale();

		FVector LocalPosition = ComponentTransform.InverseTransformPosition(PointerPosition);

		FBoxSphereBounds Bounds = Primitive->CalcLocalBounds();

		float ScaledRadius = Radius / ComponentTransform.GetScaleVector().X;

		if (LocalPosition.X - ScaledRadius < Bounds.BoxExtent.X)
		{
			return true;
		}

		return false;
	}

	bool IsFacingPrimitive(const FVector& PointerForward, const UPrimitiveComponent* const Primitive)
	{
		check(Primitive);
		FVector PrimitiveForward = Primitive->GetComponentTransform().GetUnitAxis(EAxis::X);
		if (FVector::DotProduct(PointerForward, PrimitiveForward) >= 0)
		{
			return false;
		}
		return true;
	}

	/**
	 * Used to determine whether if poke has ended with a front face pokable. A poke
	 * ends if:
	 * - The pointer sphere moves back in front of the front face of the pokable
	 * - The pointer spher moves left/right/up/down beyond the pokable primitive
	 *   extents
	 * - The perpendicular distance from the pointer sphere to the front face exceeds the
	 *   given depth.
	 *
	 * This function assumes that the given primitive has a box collider.
	 */
	bool IsFrontFacePokeEnded(UPrimitiveComponent* Primitive, FVector PointerPosition, float Radius, float Depth)
	{
		check(Primitive != nullptr);

		// Front face pokables must have a box-shaped collider
		if (!IsBoxShape(*Primitive))
		{
			UE_LOG(
				UXTools, Warning,
				TEXT("Primitive %s has some collision "
					 "shape other than box"),
				*Primitive->GetFName().ToString());
			return false;
		}

		const FMatrix ComponentTransform = Primitive->GetComponentTransform().ToMatrixNoScale();

		FVector LocalPosition = ComponentTransform.InverseTransformPosition(PointerPosition);

		FBoxSphereBounds Bounds = Primitive->CalcLocalBounds();

		FVector Max = Bounds.BoxExtent * Primitive->GetComponentTransform().GetScale3D();

		FVector Min = -Max;
		Min.X = Max.X - Depth; // depth is measured from the front face

		FBox PokableVolume(Min, Max);
		FSphere PokeSphere(LocalPosition, Radius);

		return !FMath::SphereAABBIntersection(PokeSphere, PokableVolume);
	}

#if ENABLE_VISUAL_LOG
	static FName VLogCategoryGrabPointer("LogUxtGrabPointer");
	static FName VLogCategoryPokePointer("LogUxtPokePointer");
	static const FColor VLogGrabFocusColor = FColor(27, 233, 130);
	static const FColor VLogPokeFocusColor = FColor(80, 140, 241);
#endif // ENABLE_VISUAL_LOG
} // namespace

UUxtNearPointerComponent::UUxtNearPointerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick before controls
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;

	GrabFocus = new FUxtGrabPointerFocus();
	PokeFocus = new FUxtPokePointerFocus();
#if ENABLE_VISUAL_LOG
	GrabFocus->VLogOwnerWeak = this;
	PokeFocus->VLogOwnerWeak = this;
	GrabFocus->VLogCategory = VLogCategoryGrabPointer;
	PokeFocus->VLogCategory = VLogCategoryPokePointer;
	GrabFocus->VLogColor = VLogGrabFocusColor;
	PokeFocus->VLogColor = VLogPokeFocusColor;
#endif // ENABLE_VISUAL_LOG

	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Finder(TEXT("/UXTools/Materials/MPC_UXSettings"));
	ParameterCollection = Finder.Object;
}

UUxtNearPointerComponent::~UUxtNearPointerComponent()
{
	delete GrabFocus;
	delete PokeFocus;
}

void UUxtNearPointerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Set initial finger tip position to an unlikely value
	UpdateParameterCollection(FVector(FLT_MAX));
}

void UUxtNearPointerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GrabFocus->IsGrabbing())
	{
		GrabFocus->EndGrab(this);
	}

	GrabFocus->ClearFocus(this);
	PokeFocus->ClearFocus(this);

	Super::EndPlay(EndPlayReason);
}

static FTransform CalcGrabPointerTransform(EControllerHand Hand)
{
	FQuat IndexTipOrientation, ThumbTipOrientation;
	FVector IndexTipPosition, ThumbTipPosition;
	float IndexTipRadius, ThumbTipRadius;
	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(
			Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius) &&
		UUxtHandTrackingFunctionLibrary::GetHandJointState(
			Hand, EUxtHandJoint::ThumbTip, ThumbTipOrientation, ThumbTipPosition, ThumbTipRadius))
	{
		// Use the midway point between the thumb and index finger tips for grab
		const float LerpFactor = 0.5f;
		return FTransform(
			FMath::Lerp(IndexTipOrientation, ThumbTipOrientation, LerpFactor), FMath::Lerp(IndexTipPosition, ThumbTipPosition, LerpFactor));
	}
	return FTransform::Identity;
}

static FTransform CalcPokePointerTransform(EControllerHand Hand)
{
	FQuat IndexTipOrientation;
	FVector IndexTipPosition;
	float IndexTipRadius;
	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(
			Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius))
	{
		return FTransform(IndexTipOrientation, IndexTipPosition);
	}
	return FTransform(FQuat::Identity, FVector(FLT_MAX));
}

void UUxtNearPointerComponent::UpdateParameterCollection(FVector IndexTipPosition)
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

void UUxtNearPointerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Update cached transforms
	GrabPointerTransform = CalcGrabPointerTransform(Hand);
	PokePointerTransform = CalcPokePointerTransform(Hand);
	UpdateParameterCollection(PokePointerTransform.GetLocation());

	// Unlock focus if targets have been removed,
	// e.g. if target actors are destroyed while focus locked.
	if (bFocusLocked)
	{
		if (!GrabFocus->GetFocusedTarget() && !PokeFocus->GetFocusedTarget())
		{
			bFocusLocked = false;
		}
	}

	// Don't change the focused target if focus is locked
	if (bFocusLocked)
	{
		GrabFocus->UpdateClosestTarget(GrabPointerTransform);
		PokeFocus->UpdateClosestTarget(PokePointerTransform);
	}
	else
	{
		const FVector ProximityCenter = GrabPointerTransform.GetLocation();

		// Disable complex collision to enable overlap from inside primitives
		FCollisionQueryParams QueryParams(NAME_None, false);

		TArray<FOverlapResult> Overlaps;
		/*bool HasBlockingOverlap = */ GetWorld()->OverlapMultiByChannel(
			Overlaps, ProximityCenter, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(ProximityRadius), QueryParams);

		GrabFocus->SelectClosestTarget(this, GrabPointerTransform, Overlaps);
		PokeFocus->SelectClosestTarget(this, PokePointerTransform, Overlaps);
	}

	// Update poking state based on poke target
	UpdatePokeInteraction();

	// Update focused targets
	GrabFocus->UpdateFocus(this);
	if (IsGrabbing())
	{
		GrabFocus->UpdateGrab(this);
	}

	PokeFocus->UpdateFocus(this);

	// Update the grab state

	bool bHandIsGrabbing;
	if (UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(Hand, bHandIsGrabbing))
	{
		if (bHandIsGrabbing != bHandWasGrabbing && bHandIsGrabbing != GrabFocus->IsGrabbing())
		{
			if (bHandIsGrabbing)
			{
				GrabFocus->BeginGrab(this);
				PokeFocus->ClearFocus(this);
			}
			else
			{
				GrabFocus->EndGrab(this);
			}
		}

		bHandWasGrabbing = bHandIsGrabbing;
	}

#if ENABLE_VISUAL_LOG
	VLogPointer(VLogCategoryGrabPointer, VLogGrabFocusColor, "Grab Pointer", GrabPointerTransform.GetLocation(), GrabRadius, GrabFocus);
	VLogPointer(VLogCategoryPokePointer, VLogPokeFocusColor, "Poke Pointer", PokePointerTransform.GetLocation(), PokeRadius, PokeFocus);
#endif // ENABLE_VISUAL_LOG
}

void UUxtNearPointerComponent::SetActive(bool bNewActive, bool bReset)
{
	bool bOldActive = IsActive();
	Super::SetActive(bNewActive, bReset);

	if (!UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(Hand, bHandWasGrabbing))
	{
		bHandWasGrabbing = false;
	}

	if (bOldActive && !bNewActive)
	{
		if (GrabFocus->IsGrabbing())
		{
			GrabFocus->EndGrab(this);
		}

		if (PokeFocus->IsPoking())
		{
			PokeFocus->EndPoke(this);
		}

		GrabFocus->ClearFocus(this);
		PokeFocus->ClearFocus(this);
		bFocusLocked = false;

		// Set finger tip position to an unlikely value
		UpdateParameterCollection(FVector(FLT_MAX));
	}
}

UObject* UUxtNearPointerComponent::GetFocusTarget() const
{
	UObject* FocusTarget = GrabFocus->GetFocusedTarget();
	if (!FocusTarget)
	{
		FocusTarget = PokeFocus->GetFocusedTarget();
	}

	return FocusTarget;
}

FTransform UUxtNearPointerComponent::GetCursorTransform() const
{
	if (GrabFocus->IsGrabbing())
	{
		return GetGrabPointerTransform();
	}

	return GetPokePointerTransform();
}

void UUxtNearPointerComponent::UpdatePokeInteraction()
{
	FVector PokePointerLocation = GetPokePointerTransform().GetLocation();
	UActorComponent* Target = Cast<UActorComponent>(PokeFocus->GetFocusedTarget());
	UPrimitiveComponent* Primitive = PokeFocus->GetFocusedPrimitive();

	if (PokeFocus->IsPoking())
	{
		if (Primitive && Target)
		{
			bool endedPoking = false;

			switch (IUxtPokeTarget::Execute_GetPokeBehaviour(Target))
			{
			case EUxtPokeBehaviour::FrontFace:
				endedPoking = IsFrontFacePokeEnded(Primitive, PokePointerLocation, GetPokePointerRadius() + DebounceDepth, PokeDepth);
				break;
			case EUxtPokeBehaviour::Volume:
				endedPoking = !Primitive->OverlapComponent(
					PokePointerLocation, FQuat::Identity, FCollisionShape::MakeSphere(GetPokePointerRadius() + DebounceDepth));
				break;
			}

			if (endedPoking)
			{
				PokeFocus->EndPoke(this);

				bWasBehindFrontFace = IsBehindFrontFace(Primitive, PokePointerLocation, GetPokePointerRadius());
			}
			else
			{
				PokeFocus->UpdatePoke(this);
			}
		}
		else
		{
			PokeFocus->EndPoke(this);

			bFocusLocked = false;
			bWasBehindFrontFace = false;
		}
	}
	else if (Target)
	{
		FVector Start = PreviousPokePointerLocation;
		FVector End = PokePointerLocation;

		bool isBehind = bWasBehindFrontFace;
		if (Primitive)
		{
			isBehind = IsBehindFrontFace(Primitive, End, GetPokePointerRadius());
		}

		FHitResult HitResult;
		GetWorld()->SweepSingleByChannel(
			HitResult, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(GetPokePointerRadius()));

		if (HitResult.GetComponent() == Primitive)
		{
			bool startedPoking = false;

			switch (IUxtPokeTarget::Execute_GetPokeBehaviour(Target))
			{
			case EUxtPokeBehaviour::FrontFace:
			{
				bool bIsFacingPrimitive = IsFacingPrimitive(PokePointerTransform.GetUnitAxis(EAxis::X), Primitive);
				startedPoking = !bWasBehindFrontFace && isBehind && bIsFacingPrimitive;
				break;
			}
			case EUxtPokeBehaviour::Volume:
				startedPoking = true;
				break;
			}

			if (startedPoking)
			{
				PokeFocus->BeginPoke(this);
			}
		}

		bWasBehindFrontFace = isBehind;
	}

	PreviousPokePointerLocation = PokePointerLocation;
}

UObject* UUxtNearPointerComponent::GetFocusedGrabTarget(FVector& OutClosestPointOnTarget, FVector& OutNormal) const
{
	OutClosestPointOnTarget = GrabFocus->GetClosestTargetPoint();
	OutNormal = GrabFocus->GetClosestTargetNormal();
	return GrabFocus->GetFocusedTarget();
}

UObject* UUxtNearPointerComponent::GetFocusedPokeTarget(FVector& OutClosestPointOnTarget, FVector& OutNormal) const
{
	OutClosestPointOnTarget = PokeFocus->GetClosestTargetPoint();
	OutNormal = PokeFocus->GetClosestTargetNormal();
	return PokeFocus->GetFocusedTarget();
}

UPrimitiveComponent* UUxtNearPointerComponent::GetFocusedGrabPrimitive(FVector& OutClosestPointOnTarget, FVector& OutNormal) const
{
	OutClosestPointOnTarget = GrabFocus->GetClosestTargetPoint();
	OutNormal = GrabFocus->GetClosestTargetNormal();
	return GrabFocus->GetFocusedPrimitive();
}

UPrimitiveComponent* UUxtNearPointerComponent::GetFocusedPokePrimitive(FVector& OutClosestPointOnTarget, FVector& OutNormal) const
{
	OutClosestPointOnTarget = PokeFocus->GetClosestTargetPoint();
	OutNormal = PokeFocus->GetClosestTargetNormal();
	return PokeFocus->GetFocusedPrimitive();
}

bool UUxtNearPointerComponent::SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		GrabFocus->SelectClosestPointOnTarget(this, GetGrabPointerTransform(), NewFocusedTarget);

		bFocusLocked = (NewFocusedTarget != nullptr && bEnableFocusLock);

		return true;
	}
	return false;
}

bool UUxtNearPointerComponent::SetFocusedPokeTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		PokeFocus->SelectClosestPointOnTarget(this, GetPokePointerTransform(), NewFocusedTarget);

		bFocusLocked = (NewFocusedTarget != nullptr && bEnableFocusLock);

		return true;
	}
	return false;
}

bool UUxtNearPointerComponent::IsGrabbing() const
{
	return GrabFocus->IsGrabbing();
}

bool UUxtNearPointerComponent::GetIsPoking() const
{
	return PokeFocus->IsPoking();
}

FTransform UUxtNearPointerComponent::GetGrabPointerTransform() const
{
	return GrabPointerTransform;
}

FTransform UUxtNearPointerComponent::GetPokePointerTransform() const
{
	return PokePointerTransform;
}

float UUxtNearPointerComponent::GetPokePointerRadius() const
{
	FQuat IndexTipOrientation;
	FVector IndexTipPosition;
	float IndexTipRadius;
	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(
			Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius))
	{
		return IndexTipRadius;
	}
	return 0;
}

#if ENABLE_VISUAL_LOG
void UUxtNearPointerComponent::VLogPointer(
	const FName& LogCategoryName, const FColor& LogColor, const FString& Label, const FVector& PointerLocation, float PointerRadius,
	const FUxtPointerFocus* Focus) const
{
	if (!FVisualLogger::IsRecording())
	{
		return;
	}

	UE_VLOG_LOCATION(this, LogCategoryName, Verbose, PointerLocation, PointerRadius, LogColor, TEXT("%s"), *Label);

	const FVector PointOnTarget = Focus->GetClosestTargetPoint();
	UE_VLOG_SEGMENT(
		this, LogCategoryName, Verbose, PointOnTarget, PointerLocation, LogColor, TEXT("%.2f"),
		FVector::Distance(PointOnTarget, PointerLocation));
	UE_VLOG_SEGMENT(
		this, LogCategoryName, Verbose, PointOnTarget, PointOnTarget + Focus->GetClosestTargetNormal() * 5.0f, FColor::Yellow, TEXT(""));
}
#endif // ENABLE_VISUAL_LOG
