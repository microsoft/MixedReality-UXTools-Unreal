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
#include "ProceduralMeshComponent.h"
#include "Input/UxtHandProximityMesh.h"

AUxtHandInteractionActor::AUxtHandInteractionActor(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	// Do not delay pointers' tick unnecessarily
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));

	NearPointer = CreateDefaultSubobject<UUxtNearPointerComponent>(TEXT("NearPointer"));
	NearPointer->PrimaryComponentTick.bStartWithTickEnabled = false;
	NearPointer->AddTickPrerequisiteActor(this);

	FarPointer = CreateDefaultSubobject<UUxtFarPointerComponent>(TEXT("FarPointer"));
	FarPointer->PrimaryComponentTick.bStartWithTickEnabled = false;
	FarPointer->AddTickPrerequisiteActor(this);

	ProximityTrigger = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProximityTrigger"));
	ProximityTrigger->bUseComplexAsSimpleCollision = false;
	ProximityTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProximityTrigger->SetCollisionProfileName(TEXT("UI"));
	ProximityTrigger->SetupAttachment(GetRootComponent());

	// Zero out the velocity caches.
	// FVector's default constructor doesn't zero initialize it.
	for (int i = 0; i < VelocityUpdateInterval; ++i)
	{
		VelocityPositionsCache[i] = FVector::ZeroVector;
		VelocityNormalsCache[i] = FVector::ZeroVector;
	}
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

	UpdateProximityMesh();
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

void AUxtHandInteractionActor::UpdateProximityMesh()
{
	FUxtHandProximityMeshData MeshData;
	MeshData.bEnableLighting = bRenderProximityMesh;

	MeshData.Build(ProximityConeAngle, ProximityConeOffset, ProximityConeSideLength, 36);
	MeshData.UpdateMesh(ProximityTrigger, 0);

	ProximityTrigger->SetVisibility(bRenderProximityMesh);
	// Only need to change transform for visualization purposes, scene query uses an explicit transform.
	ProximityTrigger->SetMobility(bRenderProximityMesh ? EComponentMobility::Movable : EComponentMobility::Stationary);
}

void AUxtHandInteractionActor::UpdateVelocity(float DeltaTime)
{
	if (const IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		FVector Position;
		FQuat Orientation;
		float Radius;

		if (HandTracker->GetJointState(Hand, EUxtHandJoint::Palm, Orientation, Position, Radius))
		{
			const FVector Normal = -Orientation.GetUpVector();

			const int FrameIndex = CurrentFrame % VelocityUpdateInterval;

			const FVector NewPositionsSum = VelocityPositionsSum - VelocityPositionsCache[FrameIndex] + Position;
			const FVector NewNormalsSum = VelocityNormalsSum - VelocityNormalsCache[FrameIndex] + Normal;

			const int CurrentInterval = CurrentFrame < VelocityUpdateInterval ? CurrentFrame : VelocityUpdateInterval;
			Velocity = (NewPositionsSum - VelocityPositionsSum) / DeltaTime / CurrentInterval;

			const FQuat Rotation = ((NewNormalsSum / CurrentInterval) - (VelocityNormalsSum / CurrentInterval)).ToOrientationQuat();
			const FVector RotationRate = FMath::DegreesToRadians(Rotation.Euler());
			AngularVelocity = RotationRate / DeltaTime;

			VelocityPositionsCache[FrameIndex] = Position;
			VelocityPositionsSum = NewPositionsSum;
			VelocityNormalsCache[FrameIndex] = Normal;
			VelocityNormalsSum = NewNormalsSum;
		}

		++CurrentFrame;
	}
}

bool AUxtHandInteractionActor::QueryProximityVolume(bool& OutHasNearTarget)
{
	OutHasNearTarget = false;

	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		FQuat IndexTipOrientation, PalmOrientation;
		FVector IndexTipPosition, PalmPosition;
		float IndexTipRadius, PalmRadius;
		const bool bIsIndexTipValid = HandTracker->GetJointState(Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius);
		const bool bIsPalmValid = HandTracker->GetJointState(Hand, EUxtHandJoint::Palm, PalmOrientation, PalmPosition, PalmRadius);
		if (bIsIndexTipValid && bIsPalmValid)
		{
			const FVector PalmForward = PalmOrientation.GetForwardVector();
			const FVector PalmToIndex = IndexTipPosition - PalmPosition;
			const FVector ConeDirection = FMath::Lerp(PalmForward, PalmToIndex, ProximityConeAngleLerp).GetSafeNormal();
			const FQuat ConeOrientation = FRotationMatrix::MakeFromXZ(ConeDirection, PalmOrientation.GetUpVector()).ToQuat();
			const FVector ConeTip = PalmPosition - ConeDirection * ProximityConeOffset;

			// Near-far activation query
			TArray<FOverlapResult> Overlaps;
			FComponentQueryParams QueryParams(NAME_None);
			// Disable complex collision to enable overlap from inside primitives
			QueryParams.bTraceComplex = false;

			GetWorld()->ComponentOverlapMulti(Overlaps, ProximityTrigger, ConeTip, ConeOrientation, QueryParams);

			// Look for a near target in the overlaps
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (IsNearTarget(Overlap.GetComponent()))
				{
					OutHasNearTarget = true;
					break;
				}
			}

			// Only need to change transform for visualization purposes, scene query uses an explicit transform.
			if (bRenderProximityMesh)
			{
				ProximityTrigger->SetWorldTransform(FTransform(ConeOrientation, ConeTip));
			}

			return true;
		}
	}

	return false;
}

// Called every frame
void AUxtHandInteractionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const bool bHasFocusLock = NearPointer->GetFocusLocked() || FarPointer->GetFocusLocked();

	bool bNearInteractionFlag = InteractionMode & static_cast<int32>(EUxtInteractionMode::Near);
	bool bFarInteractionFlag = InteractionMode & static_cast<int32>(EUxtInteractionMode::Far);

	if (!bNearInteractionFlag && !bFarInteractionFlag)
	{
		return;
	}

	bool bNewNearPointerActive = NearPointer->IsActive();
	bool bNewFarPointerActive = FarPointer->IsActive();
	bool bHasNearTarget;
	if (QueryProximityVolume(bHasNearTarget))
	{
		// Only switch between near and far if none of the pointers is locked,
		// otherwise pointer active state remains unchanged.
		if (!bHasFocusLock)
		{
			if (IsInPointingPose())
			{
				// Update pointers activation state
				bNewNearPointerActive = bHasNearTarget;
				bNewFarPointerActive = !bHasNearTarget;
			}
			else
			{
				// Hand not in pointing pose, deactivate both pointers
				bNewNearPointerActive = false;
				bNewFarPointerActive = false;
			}
		}
	}
	else
	{
		// Hand not tracked, deactivate both pointers
		bNewNearPointerActive = false;
		bNewFarPointerActive = false;
	}

	bNewNearPointerActive &= bNearInteractionFlag;
	bNewFarPointerActive &= bFarInteractionFlag;

	// Update pointer active state
	if (bNewNearPointerActive != NearPointer->IsActive())
	{
		NearPointer->SetActive(bNewNearPointerActive);
	}
	if (bNewFarPointerActive != FarPointer->IsActive())
	{
		FarPointer->SetActive(bNewFarPointerActive);
	}

	UpdateVelocity(DeltaTime);
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
