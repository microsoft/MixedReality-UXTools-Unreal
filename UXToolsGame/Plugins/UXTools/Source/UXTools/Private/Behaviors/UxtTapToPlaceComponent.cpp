#include "Behaviors/UxtTapToPlaceComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtInputSubsystem.h"
#include "Utils/UxtFunctionLibrary.h"

UUxtTapToPlaceComponent::UUxtTapToPlaceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

UPrimitiveComponent* UUxtTapToPlaceComponent::GetTargetComponent() const
{
	return Cast<UPrimitiveComponent>(TargetComponent.GetComponent(GetOwner()));
}

void UUxtTapToPlaceComponent::SetTargetComponent(UPrimitiveComponent* Target)
{
	TargetComponent.OverrideComponent = Target;
}

void UUxtTapToPlaceComponent::StartPlacement()
{
	if (!bIsBeingPlaced)
	{
		if (FocusLockedPointerWeak.IsValid())
		{
			FocusLockedPointerWeak->SetFocusLocked(false);
			FocusLockedPointerWeak = nullptr;
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
		bIsBeingPlaced = false;
		UUxtInputSubsystem::UnregisterHandler(this, UUxtFarHandler::StaticClass());
		OnEndPlacing.Broadcast(this);
	}
}

void UUxtTapToPlaceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetTargetComponent())
	{
		SetTargetComponent(Cast<UPrimitiveComponent>(GetOwner()->GetComponentByClass(UPrimitiveComponent::StaticClass())));
	}

	if (UPrimitiveComponent* Target = GetTargetComponent())
	{
		FBoxSphereBounds Bounds = GetTargetComponent()->CalcLocalBounds();
		SurfaceNormalOffset = Bounds.BoxExtent.X * Target->GetComponentTransform().GetScale3D().X;
	}
}

void UUxtTapToPlaceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsBeingPlaced)
	{
		if (UPrimitiveComponent* Target = GetTargetComponent())
		{
			FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());

			// Ignore the target being placed
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredComponent(Target);

			FHitResult Result;
			FVector Start = HeadPose.GetLocation();
			FVector End = Start + HeadPose.GetUnitAxis(EAxis::X) * MaxRaycastDistance;
			GetWorld()->LineTraceSingleByChannel(Result, Start, End, TraceChannel, QueryParams);

			FVector HitPosition = Start + HeadPose.GetUnitAxis(EAxis::X) * DefaultPlacementDistance;
			FVector Facing = HeadPose.GetLocation() - HitPosition;
			if (Result.GetComponent())
			{
				// Add SurfaceNormalOffset so object is placed touching the surface, rather than overlapping with it
				HitPosition = Result.Location + Result.Normal * SurfaceNormalOffset;

				if (OrientationType == EUxtTapToPlaceOrientBehavior::AlignToSurface)
				{
					Facing = Result.Normal;
				}
				else
				{
					Facing = HeadPose.GetLocation() - HitPosition;
				}
			}

			if (KeepOrientationVertical)
			{
				Facing.Z = 0;
			}

			FVector Position;
			FQuat Orientation;

			if (bInterpolatePose)
			{
				FVector TargetPos = Target->GetComponentLocation();
				FQuat TargetRot = Target->GetComponentRotation().Quaternion();

				float LerpAmount = LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime;
				Position = FMath::Lerp(TargetPos, HitPosition, LerpAmount);
				Orientation = FQuat::Slerp(TargetRot, Facing.ToOrientationQuat(), LerpAmount);
			}
			else
			{
				Position = HitPosition;
				Orientation = Facing.ToOrientationQuat();
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
	return Primitive == GetTargetComponent();
}

bool UUxtTapToPlaceComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return bIsBeingPlaced ? true : Primitive == GetTargetComponent();
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
