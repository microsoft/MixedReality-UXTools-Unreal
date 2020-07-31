// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.


#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Controls/UxtFingerCursorComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "Controls/UxtFarBeamComponent.h"
#include "HandTracking/IUxtHandTracker.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeTarget.h"
#include "Utils/UxtFunctionLibrary.h"


AUxtHandInteractionActor::AUxtHandInteractionActor(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PostPhysics;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));

	NearPointer = CreateDefaultSubobject<UUxtNearPointerComponent>(TEXT("NearPointer"));
	NearPointer->PrimaryComponentTick.bStartWithTickEnabled = false;
	NearPointer->AddTickPrerequisiteActor(this);

	FarPointer = CreateDefaultSubobject<UUxtFarPointerComponent>(TEXT("FarPointer"));
	FarPointer->PrimaryComponentTick.bStartWithTickEnabled = false;
	FarPointer->AddTickPrerequisiteActor(this);
}

// Called when the game starts or when spawned
void AUxtHandInteractionActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply actor settings to pointers
	NearPointer->Hand = Hand;
	NearPointer->TraceChannel = TraceChannel;
	NearPointer->PokeRadius = PokeRadius;
	FarPointer->Hand = Hand;
	FarPointer->TraceChannel = TraceChannel;
	FarPointer->RayStartOffset = RayStartOffset;
	FarPointer->RayLength = RayLength;

	if (bUseDefaultNearCursor)
	{
		UUxtFingerCursorComponent* NearCursor = NewObject<UUxtFingerCursorComponent>(this);
		NearCursor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		NearCursor->bShowOnGrabTargets = bShowNearCursorOnGrabTargets;
		NearCursor->RegisterComponent();
	}

	if (bUseDefaultFarCursor)
	{
		UUxtFarCursorComponent* FarCursor = NewObject<UUxtFarCursorComponent>(this);
		FarCursor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		FarCursor->RegisterComponent();
	}

	if (bUseDefaultFarBeam)
	{
		UUxtFarBeamComponent* FarBeam = NewObject<UUxtFarBeamComponent>(this);
		
		// Prevent self Transform from affecting the FarBeam's one
		FarBeam->SetAbsolute(true, true, true);

		FarBeam->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
		FarBeam->RegisterComponent();
	}
}

// Returns true if the given primitive is part of a near target
static bool IsNearTarget(const UPrimitiveComponent* Primitive)
{
	for (const UActorComponent* Component : Primitive->GetOwner()->GetComponents())
	{
		if (Component->Implements<UUxtGrabTarget>() || Component->Implements<UUxtPokeTarget>())
		{
			return true;
		}
	}

	return false;
}

// Called every frame
void AUxtHandInteractionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		FQuat FingerTipOrientation;
		FVector FingerTipPosition;
		float JointRadius;
		bool bIsTracked = HandTracker->GetJointState(Hand, EUxtHandJoint::IndexTip, FingerTipOrientation, FingerTipPosition, JointRadius);

		if (bIsTracked)
		{
			const FVector Forward = FingerTipOrientation.GetForwardVector();
			const FVector FingerTipPositionOnSkin = FingerTipPosition + Forward * JointRadius;

			const float SphereRadius = 0.5f * NearActivationDistance;
			FVector QueryPosition = FingerTipPositionOnSkin + Forward * SphereRadius;

			// Only switch between near and far if none of the pointers is locked
			if (bHadTracking && !NearPointer->GetFocusLocked() && !FarPointer->GetFocusLocked())
			{
				// Near-far activation query
				TArray<FHitResult> Overlaps;
				// Disable complex collision to enable overlap from inside primitives
				FCollisionQueryParams QueryParams(NAME_None, false);
				FCollisionShape QuerySphere = FCollisionShape::MakeSphere(SphereRadius);
				GetWorld()->SweepMultiByChannel(Overlaps, PrevQueryPosition, QueryPosition, FQuat::Identity, TraceChannel, QuerySphere, QueryParams);

				// Look for a near target in the overlaps
				bool bHasNearTarget = false;
				for (const FHitResult& Overlap : Overlaps)
				{
					if (IsNearTarget(Overlap.GetComponent()))
					{
						bHasNearTarget = true;
						break;
					}
				}

				// Disable the pointers if the hand is facing upwards
				if (IsInPointingPose())
				{
					// Update pointers activation state
					if (bHasNearTarget != NearPointer->IsActive())
					{
						NearPointer->SetActive(bHasNearTarget);
					}
					if (bHasNearTarget == FarPointer->IsActive())
					{
						FarPointer->SetActive(!bHasNearTarget);
					}
				}
				else
				{
					NearPointer->SetActive(false);
					FarPointer->SetActive(false);
				}
			}

			bHadTracking = true;
			PrevQueryPosition = QueryPosition;
		}
		else
		{
			// Hand not tracked
			bHadTracking = false;

			if (NearPointer->IsActive())
			{
				NearPointer->SetActive(false);
			}
			if (FarPointer->IsActive())
			{
				FarPointer->SetActive(false);
			}
		}
	}
}

void AUxtHandInteractionActor::SetHand(EControllerHand NewHand)
{
	Hand = NewHand;
	NearPointer->Hand = NewHand;
	FarPointer->Hand = NewHand;
}

void AUxtHandInteractionActor::SetTraceChannel(ECollisionChannel NewTraceChannel)
{
	TraceChannel = NewTraceChannel;
	NearPointer->TraceChannel = NewTraceChannel;
	FarPointer->TraceChannel = NewTraceChannel;
}

void AUxtHandInteractionActor::SetPokeRadius(float NewPokeRadius)
{
	PokeRadius = NewPokeRadius;
	NearPointer->PokeRadius = NewPokeRadius;
}

void AUxtHandInteractionActor::SetRayStartOffset(float NewRayStartOffset)
{
	RayStartOffset = NewRayStartOffset;
	FarPointer->RayStartOffset = NewRayStartOffset;
}

void AUxtHandInteractionActor::SetRayLength(float NewRayLength)
{
	RayLength = NewRayLength;
	FarPointer->RayLength = NewRayLength;
}

bool AUxtHandInteractionActor::IsInPointingPose() const
{
	constexpr float PointerBeamBackwardTolerance = 0.5f;
	constexpr float PointerBeamUpwardTolerance = 0.8f;

	IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker();
	if (!HandTracker)
	{
		return false;
	}

	FQuat PalmOrientation;
	FVector PalmPosition;
	float PalmRadius;

	if (HandTracker->GetJointState(Hand, EUxtHandJoint::Palm, PalmOrientation, PalmPosition, PalmRadius))
	{
		FVector PalmNormal = PalmOrientation * FVector::DownVector;
		PalmNormal.Normalize();

		if (PointerBeamBackwardTolerance >= 0)
		{
			FVector CameraBackward = -UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetRotation().GetForwardVector();
			if (FVector::DotProduct(PalmNormal, CameraBackward) > PointerBeamBackwardTolerance)
			{
				return false;
			}
		}

		if (PointerBeamUpwardTolerance >= 0)
		{
			if (FVector::DotProduct(PalmNormal, FVector::UpVector) > PointerBeamUpwardTolerance)
			{
				return false;
			}
		}
	}

	return true;
}
