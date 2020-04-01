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
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UXTools.h"


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

	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Finder(TEXT("/UXTools/Pointers/PointerPositions"));
	check(Finder.Object);
	ParameterCollection = Finder.Object;
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

	// Create default visuals
	if (bUseDefaultVisuals)
	{
		UUxtFingerCursorComponent* NearCursor = NewObject<UUxtFingerCursorComponent>(this);
		NearCursor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		NearCursor->RegisterComponent();

		UUxtFarCursorComponent* FarCursor = NewObject<UUxtFarCursorComponent>(this);
		FarCursor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		FarCursor->RegisterComponent();

		UUxtFarBeamComponent* FarBeam = NewObject<UUxtFarBeamComponent>(this);
		FarBeam->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
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
			// Update finger tip position in material parameter collection
			if (ParameterCollection)
			{
				UMaterialParameterCollectionInstance* ParameterCollectionInstance = GetWorld()->GetParameterCollectionInstance(ParameterCollection);
				static FName ParameterNames[] = { "LeftPointerPosition", "RightPointerPosition" };
				FName ParameterName = Hand == EControllerHand::Left ? ParameterNames[0] : ParameterNames[1];
				const bool bFoundParameter = ParameterCollectionInstance->SetVectorParameterValue(ParameterName, FingerTipPosition);
				if (!bFoundParameter)
				{
					UE_LOG(UXTools, Warning, TEXT("Unable to find %s parameter in material parameter collection %s."), *ParameterName.ToString(), *ParameterCollection->GetPathName());
				}
			}

			const FVector Forward = FingerTipOrientation.GetForwardVector();
			const FVector FingerTipPositionOnSkin = FingerTipPosition + Forward * JointRadius;

			// Only switch between near and far if none of the pointers is locked
			if (!NearPointer->GetFocusLocked() && !FarPointer->GetFocusLocked())
			{
				// Near-far activation query
				TArray<FOverlapResult> Overlaps;
				const float SphereRadius = 0.5f * NearActivationDistance;
				FVector QueryPosition = FingerTipPositionOnSkin + Forward * SphereRadius;
				FCollisionShape QuerySphere = FCollisionShape::MakeSphere(SphereRadius);
				GetWorld()->OverlapMultiByChannel(Overlaps, QueryPosition, FQuat::Identity, TraceChannel, QuerySphere);

				// Look for a near target in the overlaps
				bool bHasNearTarget = false;
				for (const FOverlapResult& Overlap : Overlaps)
				{
					if (IsNearTarget(Overlap.GetComponent()))
					{
						bHasNearTarget = true;
						break;
					}
				}

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
		}
		else
		{
			// Hand not tracked
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