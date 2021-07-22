// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtHandInteractionActor.h"

#include "ProceduralMeshComponent.h"

#include "Controls/UxtFarBeamComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "Controls/UxtFingerCursorComponent.h"
#include "HandTracking/IUxtHandTracker.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtHandProximityMesh.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeTarget.h"
#include "Utils/UxtFunctionLibrary.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogUxtHandTracking, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogUxtHandInteractionProximity, Log, All);

AUxtHandInteractionActor::AUxtHandInteractionActor(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	// Do not delay pointers' tick unnecessarily
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	SetActorEnableCollision(false);
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
	if (Primitive && Primitive->GetOwner())
	{
		for (const UActorComponent* Component : Primitive->GetOwner()->GetComponents())
		{
			if (Component->Implements<UUxtGrabTarget>() || Component->Implements<UUxtPokeTarget>())
			{
				return true;
			}
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
	FVector Position;
	FQuat Orientation;
	if (IUxtHandTracker::Get().GetGripPose(Hand, Orientation, Position))
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

bool AUxtHandInteractionActor::QueryProximityVolume(bool& OutHasNearTarget)
{
	OutHasNearTarget = false;

	if (IUxtHandTracker::Get().GetTrackingStatus(Hand) == ETrackingStatus::NotTracked)
	{
		return false;
	}

	// If controller is a hand we use the proximity detection volume,
	// otherwise near interaction is disabled and only far interaction used.
	if (IUxtHandTracker::Get().IsHandController(Hand))
	{
		FQuat IndexTipOrientation, PalmOrientation;
		FVector IndexTipPosition, PalmPosition;
		float IndexTipRadius, PalmRadius;
		const bool bIsIndexTipValid =
			IUxtHandTracker::Get().GetJointState(Hand, EHandKeypoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius);
		const bool bIsPalmValid =
			IUxtHandTracker::Get().GetJointState(Hand, EHandKeypoint::Palm, PalmOrientation, PalmPosition, PalmRadius);
		// We've checked for valid hand data above
		check(bIsIndexTipValid && bIsPalmValid);

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

#if ENABLE_VISUAL_LOG // VLog the proximity mesh
		VLogProximityQuery(ConeTip, ConeOrientation, Overlaps, OutHasNearTarget);
#endif // ENABLE_VISUAL_LOG

		// Only need to change transform for visualization purposes, scene query uses an explicit transform.
		if (bRenderProximityMesh)
		{
			ProximityTrigger->SetWorldTransform(FTransform(ConeOrientation, ConeTip));
		}
	}

	return true;
}

// Called every frame
void AUxtHandInteractionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if ENABLE_VISUAL_LOG
	VLogHandJoints();
#endif // ENABLE_VISUAL_LOG

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

	FQuat PalmOrientation;
	FVector PalmPosition;
	float PalmRadius;

	if (IUxtHandTracker::Get().GetJointState(Hand, EHandKeypoint::Palm, PalmOrientation, PalmPosition, PalmRadius))
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

#if ENABLE_VISUAL_LOG
namespace
{
	const FColor VLogColorHandJoints = FColor(195, 195, 195);
	const FColor VLogColorProximityNear = FColor(242, 80, 51);
	const FColor VLogColorProximityFar = FColor(62, 121, 232);
} // namespace

void AUxtHandInteractionActor::VLogHandJoints() const
{
	if (!FVisualLogger::IsRecording())
	{
		return;
	}

	IUxtHandTracker& HandTracker = IUxtHandTracker::Get();

	// Hand label at the wrist position
	{
		FVector WristPosition;
		FQuat WristOrientation;
		float WristRadius;
		if (HandTracker.GetJointState(Hand, EHandKeypoint::Wrist, WristOrientation, WristPosition, WristRadius))
		{
			FString VLogHand = (Hand == EControllerHand::Left) ? TEXT("Left") : TEXT("Right");
			UE_VLOG_LOCATION(this, LogUxtHandTracking, Log, WristPosition, 0.0f, VLogColorHandJoints, TEXT("%s Hand"), *VLogHand);
		}
	}

	// Coordinate axes of the pointer pose
	{
		FVector PointerOrigin;
		FQuat PointerOrientation;
		if (HandTracker.GetPointerPose(Hand, PointerOrientation, PointerOrigin))
		{
			UE_VLOG_SEGMENT(
				this, LogUxtHandTracking, Log, PointerOrigin, PointerOrigin + PointerOrientation.GetAxisX() * 15.0f, FColor::Red, TEXT(""));
			UE_VLOG_SEGMENT(
				this, LogUxtHandTracking, Log, PointerOrigin, PointerOrigin + PointerOrientation.GetAxisY() * 5.0f, FColor::Green,
				TEXT(""));
			UE_VLOG_SEGMENT(
				this, LogUxtHandTracking, Log, PointerOrigin, PointerOrigin + PointerOrientation.GetAxisZ() * 5.0f, FColor::Blue, TEXT(""));
		}
	};

	// Utility function for drawing a bone segment
	auto VlogJointSegment = [this, &HandTracker](EHandKeypoint JointA, EHandKeypoint JointB) {
		FVector PositionA, PositionB;
		FQuat OrientationA, OrientationB;
		float RadiusA, RadiusB;
		if (HandTracker.GetJointState(Hand, JointA, OrientationA, PositionA, RadiusA) &&
			HandTracker.GetJointState(Hand, JointB, OrientationB, PositionB, RadiusB))
		{
			UE_VLOG_SEGMENT_THICK(this, LogUxtHandTracking, Log, PositionA, PositionB, VLogColorHandJoints, 5.0f, TEXT(""));
		}
	};

	// Draw segments for finger bones

	VlogJointSegment(EHandKeypoint::ThumbMetacarpal, EHandKeypoint::ThumbProximal);
	VlogJointSegment(EHandKeypoint::ThumbProximal, EHandKeypoint::ThumbDistal);
	VlogJointSegment(EHandKeypoint::ThumbDistal, EHandKeypoint::ThumbTip);

	VlogJointSegment(EHandKeypoint::IndexMetacarpal, EHandKeypoint::IndexProximal);
	VlogJointSegment(EHandKeypoint::IndexProximal, EHandKeypoint::IndexIntermediate);
	VlogJointSegment(EHandKeypoint::IndexIntermediate, EHandKeypoint::IndexDistal);
	VlogJointSegment(EHandKeypoint::IndexDistal, EHandKeypoint::IndexTip);

	VlogJointSegment(EHandKeypoint::MiddleMetacarpal, EHandKeypoint::MiddleProximal);
	VlogJointSegment(EHandKeypoint::MiddleProximal, EHandKeypoint::MiddleIntermediate);
	VlogJointSegment(EHandKeypoint::MiddleIntermediate, EHandKeypoint::MiddleDistal);
	VlogJointSegment(EHandKeypoint::MiddleDistal, EHandKeypoint::MiddleTip);

	VlogJointSegment(EHandKeypoint::RingMetacarpal, EHandKeypoint::RingProximal);
	VlogJointSegment(EHandKeypoint::RingProximal, EHandKeypoint::RingIntermediate);
	VlogJointSegment(EHandKeypoint::RingIntermediate, EHandKeypoint::RingDistal);
	VlogJointSegment(EHandKeypoint::RingDistal, EHandKeypoint::RingTip);

	VlogJointSegment(EHandKeypoint::LittleMetacarpal, EHandKeypoint::LittleProximal);
	VlogJointSegment(EHandKeypoint::LittleProximal, EHandKeypoint::LittleIntermediate);
	VlogJointSegment(EHandKeypoint::LittleIntermediate, EHandKeypoint::LittleDistal);
	VlogJointSegment(EHandKeypoint::LittleDistal, EHandKeypoint::LittleTip);
}

void AUxtHandInteractionActor::VLogProximityQuery(
	const FVector& ConeTip, const FQuat& ConeOrientation, const TArray<FOverlapResult>& Overlaps, bool bHasNearTarget) const
{
	if (!FVisualLogger::IsRecording())
	{
		return;
	}

	const FColor VLogColor = bHasNearTarget ? VLogColorProximityNear : VLogColorProximityFar;
	const FColor VLogColorTransparent = FColor(VLogColor.R, VLogColor.G, VLogColor.B, 32);

	// Proximity detector cone
	{
		const FTransform VLogTransform(ConeOrientation, ConeTip);

		const TArray<FProcMeshVertex>& ProcVertices = ProximityTrigger->GetProcMeshSection(0)->ProcVertexBuffer;
		const TArray<uint32>& ProcTriangles = ProximityTrigger->GetProcMeshSection(0)->ProcIndexBuffer;
		TArray<FVector> VLogVertices;
		TArray<int32> VLogTriangles;
		VLogVertices.Reserve(ProcVertices.Num());
		VLogTriangles.Reserve(ProcTriangles.Num());
		for (const FProcMeshVertex& ProcVertex : ProcVertices)
		{
			VLogVertices.Emplace(VLogTransform.TransformPosition(ProcVertex.Position));
		}
		for (uint32 ProcIndex : ProcTriangles)
		{
			VLogTriangles.Emplace((int32)ProcIndex);
		}

		UE_VLOG_MESH(
			this, LogUxtHandInteractionProximity, Verbose, VLogVertices, VLogTriangles, VLogColorTransparent,
			TEXT("Proximity detector mesh"));
	}

	// Overlaps
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (IsNearTarget(Overlap.GetComponent()))
		{
			FBoxSphereBounds OverlapBounds = Overlap.GetComponent()->CalcLocalBounds();
			FString VLogBlocking = Overlap.bBlockingHit ? TEXT("blocking") : TEXT("non-blocking");
			UE_VLOG_OBOX(
				this, LogUxtHandInteractionProximity, Verbose, OverlapBounds.GetBox(),
				Overlap.GetComponent()->GetComponentTransform().ToMatrixWithScale(), VLogColor,
				TEXT("Near interaction target overlap: Actor %s, Component %s (%s)"), *Overlap.GetActor()->GetName(),
				*Overlap.GetComponent()->GetName(), *VLogBlocking);
		}
	}
}
#endif // ENABLE_VISUAL_LOG
