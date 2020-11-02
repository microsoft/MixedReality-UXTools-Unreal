// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableButtonComponent.h"

#include "UXTools.h"

#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#include <DrawDebugHelpers.h>

#include <Components/BoxComponent.h>
#include <Components/ShapeComponent.h>
#include <Components/StaticMeshComponent.h>
#include <GameFramework/Actor.h>

// Sets default values for this component's properties
UUxtPressableButtonComponent::UUxtPressableButtonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

float UUxtPressableButtonComponent::GetFrontFaceCollisionFraction() const
{
	return FrontFaceCollisionFraction;
}

void UUxtPressableButtonComponent::SetFrontFaceCollisionFraction(float Distance)
{
	Distance = FMath::Max(0.0f, Distance);

	FrontFaceCollisionFraction = Distance;
	if (BoxComponent)
	{
		if (UStaticMeshComponent* Touchable = Cast<UStaticMeshComponent>(GetVisuals()))
		{
			ConfigureBoxComponent(Touchable);
		}
	}
}

USceneComponent* UUxtPressableButtonComponent::GetVisuals() const
{
	return Cast<USceneComponent>(VisualsReference.GetComponent(GetOwner()));
}

void UUxtPressableButtonComponent::SetVisuals(USceneComponent* Visuals)
{
	VisualsReference.OverrideComponent = Visuals;

	if (Visuals && BoxComponent)
	{
		ConfigureBoxComponent(Visuals);
	}
}

void UUxtPressableButtonComponent::SetVisuals(const FComponentReference& ComponentReference)
{
	VisualsReference = ComponentReference;

	USceneComponent* Visuals = GetVisuals();

	if (Visuals && BoxComponent)
	{
		ConfigureBoxComponent(Visuals);
	}
}

void UUxtPressableButtonComponent::SetCollisionProfile(FName Profile)
{
	CollisionProfile = Profile;
	if (BoxComponent)
	{
		BoxComponent->SetCollisionProfileName(CollisionProfile);
	}
}

void UUxtPressableButtonComponent::SetUseAbsolutePushDistance(bool bAbsolute)
{
	if (bAbsolute != bUseAbsolutePushDistance)
	{
		bUseAbsolutePushDistance = bAbsolute;
		UpdateButtonDistancesScale();
	}
}

void UUxtPressableButtonComponent::SetEnabled(bool Enabled)
{
	if (Enabled && bIsDisabled)
	{
		bIsDisabled = false;
		OnButtonEnabled.Broadcast(this);
	}
	else if (!Enabled && !bIsDisabled)
	{
		// Release the button if it's pressed
		if (bIsPressed)
		{
			SetPressed(false, nullptr, false);
		}

		// Free any locked pointers
		if (FarPointerWeak.Get())
		{
			FarPointerWeak->SetFocusLocked(false);
			FarPointerWeak = nullptr;
		}
		PokePointers.Empty();
		CurrentPushDistance = 0;

		bIsDisabled = true;
		OnButtonDisabled.Broadcast(this);
	}
}

EUxtButtonState UUxtPressableButtonComponent::GetState() const
{
	if (bIsDisabled)
	{
		return EUxtButtonState::Disabled;
	}
	else if (bIsPressed)
	{
		return EUxtButtonState::Pressed;
	}
	else if (IsContacted())
	{
		return EUxtButtonState::Contacted;
	}
	else if (IsFocused())
	{
		return EUxtButtonState::Focused;
	}

	return EUxtButtonState::Default;
}

float UUxtPressableButtonComponent::GetScaleAdjustedMaxPushDistance() const
{
	return bUseAbsolutePushDistance ? MaxPushDistance : MaxPushDistance * GetComponentTransform().GetScale3D().X;
}

EUxtPushBehavior UUxtPressableButtonComponent::GetPushBehavior() const
{
	return PushBehavior;
}

void UUxtPressableButtonComponent::SetPushBehavior(EUxtPushBehavior Behavior)
{
	PushBehavior = Behavior;

	if (BoxComponent)
	{
		UpdateMaxPushDistance();
	}
}

float UUxtPressableButtonComponent::GetMaxPushDistance() const
{
	return MaxPushDistance;
}

void UUxtPressableButtonComponent::SetMaxPushDistance(float Distance)
{
	// The push distance is automatically calculated based on the button visuals when the behavior is compress. But, if this method is
	// called at "edit time" allow the push distance to be mutated in case it is used for visualizations.
	TEnumAsByte<EWorldType::Type> WorldType = GetWorld()->WorldType;
	if ((WorldType == EWorldType::Editor) || (WorldType == EWorldType::EditorPreview) || PushBehavior != EUxtPushBehavior::Compress)
	{
		MaxPushDistance = Distance;
	}
}

bool UUxtPressableButtonComponent::VisualBoundsFilter(const USceneComponent* Component)
{
	// Allow mesh and shape components to be considered in bounds calculations.
	return (Cast<const UMeshComponent>(Component) != nullptr || Cast<const UShapeComponent>(Component) != nullptr);
}

// Called when the game starts
void UUxtPressableButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent = NewObject<UBoxComponent>(this);

	BoxComponent->SetupAttachment(this);
	BoxComponent->RegisterComponent();

	if (USceneComponent* Visuals = GetVisuals())
	{
		ConfigureBoxComponent(Visuals);
	}
}

// Called every frame
void UUxtPressableButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update poke if we're not currently pressed via a far pointer
	if (!FarPointerWeak.IsValid())
	{
		// Update button logic with all known pointers
		UUxtNearPointerComponent* NewPokingPointer = nullptr;
		float TargetDistance = 0;

		for (UUxtNearPointerComponent* Pointer : PokePointers)
		{
			float PushDistance = CalculatePushDistance(Pointer);
			if (PushDistance > TargetDistance)
			{
				NewPokingPointer = Pointer;
				TargetDistance = PushDistance;
			}
		}

		check(TargetDistance >= 0 && TargetDistance <= MaxPushDistance);

		const float PreviousPushDistance = CurrentPushDistance;

		// Update push distance and raise events
		if (TargetDistance > CurrentPushDistance)
		{
			CurrentPushDistance = TargetDistance;
			float PressedDistance = GetPressedDistance();

			if (!bIsPressed && CurrentPushDistance >= PressedDistance && PreviousPushDistance < PressedDistance)
			{
				SetPressed(true, NewPokingPointer);
			}
		}
		else
		{
			CurrentPushDistance = FMath::Max(TargetDistance, CurrentPushDistance - DeltaTime * RecoverySpeed);
			float ReleasedDistance = GetReleasedDistance();

			// Raise button released if we're pressed and crossed the released distance
			if (bIsPressed && (CurrentPushDistance <= ReleasedDistance && PreviousPushDistance > ReleasedDistance))
			{
				SetPressed(false, NewPokingPointer);
			}
		}
	}

	// Update visuals behaviors
	if (USceneComponent* Visuals = GetVisuals())
	{
		switch (PushBehavior)
		{
		default:
		case EUxtPushBehavior::Translate:
		{
			const FVector VisualsOffset = GetComponentTransform().TransformVector(VisualsOffsetLocal);
			FVector NewVisualsLocation = VisualsOffset + GetCurrentButtonLocation();
			Visuals->SetWorldLocation(NewVisualsLocation);
		}
		break;
		case EUxtPushBehavior::Compress:
		{
			float CompressionScale = (MaxPushDistance != 0.0f) ? 1.0f - (CurrentPushDistance / MaxPushDistance) : 1.0f;
			CompressionScale = FMath::Clamp(CompressionScale, PressedFraction, 1.0f);
			Visuals->SetRelativeScale3D(FVector(VisualsScaleLocal.X * CompressionScale, VisualsScaleLocal.Y, VisualsScaleLocal.Z));
		}
		break;
		}
	}

#if 0
	// Debug display
	{
		// Button face
		{
			FVector Position = GetCurrentButtonLocation();
			FPlane Plane(Position, -GetComponentTransform().GetUnitAxis(EAxis::X));
			FVector2D HalfExtents = GetButtonExtents();
			DrawDebugSolidPlane(GetWorld(), Plane, Position, HalfExtents, FColor::Blue);
		}

		// Pointers
		for (const auto& Pointer : GetPokePointers())
		{
			auto Position = Pointer.Key->GetPokePointerTransform().GetLocation();

			// Shift it up a bit so it is not hidden by the pointer visuals.
			Position.Z += 2;

			DrawDebugPoint(GetWorld(), Position, 10, FColor::Yellow);
		}
	}
#endif
}

#if WITH_EDITOR
void UUxtPressableButtonComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UUxtPressableButtonComponent, bUseAbsolutePushDistance))
	{
		UpdateButtonDistancesScale();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UUxtPressableButtonComponent::CanEditChange(const FProperty* Property) const
{
	bool IsEditable = Super::CanEditChange(Property);

	if (IsEditable && Property != nullptr)
	{
		// When a button's push behavior is compressible the max push distance is auto-calculated and should not be
		// edited by the user.
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UUxtPressableButtonComponent, MaxPushDistance))
		{
			IsEditable = PushBehavior != EUxtPushBehavior::Compress;
		}
	}

	return IsEditable;
}

#endif

bool UUxtPressableButtonComponent::IsContacted() const
{
	return PokePointers.Num() > 0;
}

bool UUxtPressableButtonComponent::IsFocused() const
{
	return NumPointersFocusing > 0;
}

void UUxtPressableButtonComponent::OnEnterFocus(UUxtPointerComponent* Pointer)
{
	const bool bWasFocused = ++NumPointersFocusing > 1;
	OnBeginFocus.Broadcast(this, Pointer, bWasFocused);
}

void UUxtPressableButtonComponent::OnExitFocus(UUxtPointerComponent* Pointer)
{
	--NumPointersFocusing;
	const bool bIsFocused = IsFocused();

	if (!bIsFocused)
	{
		if (bIsPressed)
		{
			SetPressed(false, Pointer);
		}
	}

	OnEndFocus.Broadcast(this, Pointer, bIsFocused);
}

bool UUxtPressableButtonComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return !bIsDisabled && (Primitive == BoxComponent);
}

bool UUxtPressableButtonComponent::GetClosestPoint_Implementation(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	OutNormal = GetComponentTransform().GetUnitAxis(EAxis::X);

	float NotUsed;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed);
}

bool UUxtPressableButtonComponent::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	return !bIsDisabled && (Primitive == BoxComponent);
}

void UUxtPressableButtonComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtPressableButtonComponent::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

void UUxtPressableButtonComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtPressableButtonComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!bIsDisabled)
	{
		// Lock the poking pointer so we remain the focused target as it moves.
		Pointer->SetFocusLocked(true);

		PokePointers.Add(Pointer);
		OnBeginPoke.Broadcast(this, Pointer);
	}
}

void UUxtPressableButtonComponent::OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!bIsDisabled)
	{
		OnUpdatePoke.Broadcast(this, Pointer);
	}
}

void UUxtPressableButtonComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (bIsPressed && NumPointersFocusing == 0)
	{
		SetPressed(false, Pointer);
	}

	// Unlock the pointer focus so that another target can be selected.
	Pointer->SetFocusLocked(false);

	PokePointers.Remove(Pointer);

	if (!bIsDisabled)
	{
		OnEndPoke.Broadcast(this, Pointer);
	}
}

EUxtPokeBehaviour UUxtPressableButtonComponent::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::FrontFace;
}

float UUxtPressableButtonComponent::CalculatePushDistance(const UUxtNearPointerComponent* pointer) const
{
	FVector PointerPos = pointer->GetPokePointerTransform().GetLocation();
	PointerPos.X += pointer->GetPokePointerRadius();
	const FVector PointerLocal = GetComponentTransform().InverseTransformPosition(PointerPos);
	const float EndDistance = RestPositionLocal.X - PointerLocal.X;

	return EndDistance > 0 ? FMath::Min(EndDistance, MaxPushDistance) : 0;
}

FVector UUxtPressableButtonComponent::GetCurrentButtonLocation() const
{
	FVector Axis =
		bUseAbsolutePushDistance ? GetComponentTransform().GetUnitAxis(EAxis::X) : GetComponentTransform().GetScaledAxis(EAxis::X);
	return GetRestPosition() - (Axis * CurrentPushDistance);
}

FVector UUxtPressableButtonComponent::GetRestPosition() const
{
	return GetComponentTransform().TransformPosition(RestPositionLocal);
}

void UUxtPressableButtonComponent::SetPressed(bool bPressedState, UUxtPointerComponent* Pointer, bool bRaiseEvents)
{
	if (bPressedState != bIsPressed)
	{
		bIsPressed = bPressedState;

		if (bRaiseEvents)
		{
			if (bIsPressed)
			{
				OnButtonPressed.Broadcast(this, Pointer);
			}
			else
			{
				OnButtonReleased.Broadcast(this, Pointer);
			}
		}
	}
}

void UUxtPressableButtonComponent::UpdateButtonDistancesScale()
{
	if (bUseAbsolutePushDistance)
	{
		MaxPushDistance *= GetComponentScale().X;
	}
	else
	{
		MaxPushDistance /= GetComponentScale().X;
	}
}

void UUxtPressableButtonComponent::UpdateMaxPushDistance()
{
	if (PushBehavior == EUxtPushBehavior::Compress)
	{
		MaxPushDistance = BoxComponent->GetScaledBoxExtent().X * 2.0f;
	}
}

bool UUxtPressableButtonComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return !bIsDisabled && (Primitive == BoxComponent);
}

bool UUxtPressableButtonComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return !bIsDisabled && (Primitive == BoxComponent);
}

void UUxtPressableButtonComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtPressableButtonComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

float UUxtPressableButtonComponent::GetPressedDistance() const
{
	return MaxPushDistance * PressedFraction;
}

float UUxtPressableButtonComponent::GetReleasedDistance() const
{
	return MaxPushDistance * ReleasedFraction;
}

void UUxtPressableButtonComponent::ConfigureBoxComponent(USceneComponent* Parent)
{
	if (!BoxComponent)
	{
		UE_LOG(
			UXTools, Error,
			TEXT("Attempting to configure the box component for '%s' before it is initialised, the button will not work properly."),
			*GetOwner()->GetName());
		return;
	}

	// Disable collision on all primitive components.
	TArray<USceneComponent*> SceneComponents;
	Parent->GetChildrenComponents(true, SceneComponents);
	SceneComponents.Add(Parent);

	// If the actor's collision is disabled, we need to enable it before disabling collision on the primitive components.
	// This is because the it will restore the previous collision state when re-enabled (i.e. re-enabling collision on the components)
	const bool bActorCollisionDisabled = !GetOwner()->GetActorEnableCollision();
	if (bActorCollisionDisabled)
	{
		GetOwner()->SetActorEnableCollision(true);
	}

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(SceneComponent))
		{
			Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// Get bounds local to button, not visuals
	FTransform LocalToTarget = Parent->GetComponentTransform() * GetComponentTransform().Inverse();
	FBoxSphereBounds LocalBounds = UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(Parent, LocalToTarget, VisualBoundsFilter);
	FBox LocalBoxBounds = LocalBounds.GetBox();

	// Expand box to include the front face margin
	float MarginDist = GetScaleAdjustedMaxPushDistance() * FrontFaceCollisionFraction;
	MarginDist /= GetComponentScale().X;
	LocalBoxBounds = LocalBoxBounds.ExpandBy(FVector::ZeroVector, FVector::ForwardVector * MarginDist);

	FTransform BoxTransform = FTransform(LocalBoxBounds.GetCenter()) * GetComponentTransform();
	BoxComponent->SetWorldTransform(BoxTransform);
	BoxComponent->SetBoxExtent(LocalBoxBounds.GetExtent());
	BoxComponent->SetCollisionProfileName(CollisionProfile);
	BoxComponent->AttachToComponent(Parent, FAttachmentTransformRules::KeepWorldTransform);

	FVector RestPosition = BoxTransform.GetLocation() + BoxTransform.GetUnitAxis(EAxis::X) * BoxComponent->GetScaledBoxExtent().X;
	RestPositionLocal = GetComponentTransform().InverseTransformPosition(RestPosition);

	const FVector VisualsOffset = Parent->GetComponentLocation() - GetRestPosition();
	VisualsOffsetLocal = GetComponentTransform().InverseTransformVector(VisualsOffset);
	VisualsScaleLocal = Parent->GetRelativeScale3D();

	UpdateMaxPushDistance();

	// Disable the actor's collision if we enabled it earlier.
	if (bActorCollisionDisabled)
	{
		GetOwner()->SetActorEnableCollision(false);
	}
}

void UUxtPressableButtonComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtPressableButtonComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!FarPointerWeak.IsValid() && !bIsDisabled)
	{
		CurrentPushDistance = GetPressedDistance();
		FarPointerWeak = Pointer;
		Pointer->SetFocusLocked(true);
		SetPressed(true, Pointer);
	}
}

void UUxtPressableButtonComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get();
	if (Pointer == FarPointer)
	{
		CurrentPushDistance = 0;
		FarPointerWeak = nullptr;
		Pointer->SetFocusLocked(false);

		if (!bIsDisabled)
		{
			SetPressed(false, Pointer);
		}
	}
}
