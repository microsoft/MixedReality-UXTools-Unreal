// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlConfig.h"

FVector FUxtAffordanceConfig::GetBoundsLocation() const
{
	switch (Placement)
	{
	case EUxtAffordancePlacement::Center:
		return FVector(0, 0, 0);

	case EUxtAffordancePlacement::FaceFront:
		return FVector(-1, 0, 0);
	case EUxtAffordancePlacement::FaceBack:
		return FVector(1, 0, 0);
	case EUxtAffordancePlacement::FaceRight:
		return FVector(0, 1, 0);
	case EUxtAffordancePlacement::FaceLeft:
		return FVector(0, -1, 0);
	case EUxtAffordancePlacement::FaceTop:
		return FVector(0, 0, 1);
	case EUxtAffordancePlacement::FaceBottom:
		return FVector(0, 0, -1);

	case EUxtAffordancePlacement::EdgeFrontRight:
		return FVector(-1, 1, 0);
	case EUxtAffordancePlacement::EdgeFrontLeft:
		return FVector(-1, -1, 0);
	case EUxtAffordancePlacement::EdgeFrontTop:
		return FVector(-1, 0, 1);
	case EUxtAffordancePlacement::EdgeFrontBottom:
		return FVector(-1, 0, -1);
	case EUxtAffordancePlacement::EdgeBackRight:
		return FVector(1, 1, 0);
	case EUxtAffordancePlacement::EdgeBackLeft:
		return FVector(1, -1, 0);
	case EUxtAffordancePlacement::EdgeBackTop:
		return FVector(1, 0, 1);
	case EUxtAffordancePlacement::EdgeBackBottom:
		return FVector(1, 0, -1);
	case EUxtAffordancePlacement::EdgeRightTop:
		return FVector(0, 1, 1);
	case EUxtAffordancePlacement::EdgeRightBottom:
		return FVector(0, 1, -1);
	case EUxtAffordancePlacement::EdgeLeftTop:
		return FVector(0, -1, 1);
	case EUxtAffordancePlacement::EdgeLeftBottom:
		return FVector(0, -1, -1);

	case EUxtAffordancePlacement::CornerFrontRightTop:
		return FVector(-1, 1, 1);
	case EUxtAffordancePlacement::CornerFrontRightBottom:
		return FVector(-1, 1, -1);
	case EUxtAffordancePlacement::CornerFrontLeftTop:
		return FVector(-1, -1, 1);
	case EUxtAffordancePlacement::CornerFrontLeftBottom:
		return FVector(-1, -1, -1);
	case EUxtAffordancePlacement::CornerBackRightTop:
		return FVector(1, 1, 1);
	case EUxtAffordancePlacement::CornerBackRightBottom:
		return FVector(1, 1, -1);
	case EUxtAffordancePlacement::CornerBackLeftTop:
		return FVector(1, -1, 1);
	case EUxtAffordancePlacement::CornerBackLeftBottom:
		return FVector(1, -1, -1);
	}

	return FVector::ZeroVector;
}

FRotator FUxtAffordanceConfig::GetBoundsRotation() const
{
	return FRotator::MakeFromEuler(Rotation);
}

EUxtAffordanceKind FUxtAffordanceConfig::GetAffordanceKind() const
{
	switch (Placement)
	{
	case EUxtAffordancePlacement::Center:
		return EUxtAffordanceKind::Center;

	case EUxtAffordancePlacement::FaceFront:
		return EUxtAffordanceKind::Face;
	case EUxtAffordancePlacement::FaceBack:
		return EUxtAffordanceKind::Face;
	case EUxtAffordancePlacement::FaceRight:
		return EUxtAffordanceKind::Face;
	case EUxtAffordancePlacement::FaceLeft:
		return EUxtAffordanceKind::Face;
	case EUxtAffordancePlacement::FaceTop:
		return EUxtAffordanceKind::Face;
	case EUxtAffordancePlacement::FaceBottom:
		return EUxtAffordanceKind::Face;

	case EUxtAffordancePlacement::EdgeFrontRight:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeFrontLeft:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeFrontTop:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeFrontBottom:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeBackRight:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeBackLeft:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeBackTop:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeBackBottom:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeRightTop:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeRightBottom:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeLeftTop:
		return EUxtAffordanceKind::Edge;
	case EUxtAffordancePlacement::EdgeLeftBottom:
		return EUxtAffordanceKind::Edge;

	case EUxtAffordancePlacement::CornerFrontRightTop:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerFrontRightBottom:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerFrontLeftTop:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerFrontLeftBottom:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerBackRightTop:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerBackRightBottom:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerBackLeftTop:
		return EUxtAffordanceKind::Corner;
	case EUxtAffordancePlacement::CornerBackLeftBottom:
		return EUxtAffordanceKind::Corner;
	}

	return EUxtAffordanceKind::Center;
}

EUxtAffordanceAction FUxtAffordanceConfig::GetAction() const
{
	switch (Placement)
	{
	default:
	case EUxtAffordancePlacement::Center:
	case EUxtAffordancePlacement::FaceFront:
	case EUxtAffordancePlacement::FaceBack:
	case EUxtAffordancePlacement::FaceRight:
	case EUxtAffordancePlacement::FaceLeft:
	case EUxtAffordancePlacement::FaceTop:
	case EUxtAffordancePlacement::FaceBottom:
		return EUxtAffordanceAction::Translate;
	case EUxtAffordancePlacement::EdgeFrontRight:
	case EUxtAffordancePlacement::EdgeFrontLeft:
	case EUxtAffordancePlacement::EdgeFrontTop:
	case EUxtAffordancePlacement::EdgeFrontBottom:
	case EUxtAffordancePlacement::EdgeBackRight:
	case EUxtAffordancePlacement::EdgeBackLeft:
	case EUxtAffordancePlacement::EdgeBackTop:
	case EUxtAffordancePlacement::EdgeBackBottom:
	case EUxtAffordancePlacement::EdgeRightTop:
	case EUxtAffordancePlacement::EdgeRightBottom:
	case EUxtAffordancePlacement::EdgeLeftTop:
	case EUxtAffordancePlacement::EdgeLeftBottom:
		return EUxtAffordanceAction::Rotate;
	case EUxtAffordancePlacement::CornerFrontRightTop:
	case EUxtAffordancePlacement::CornerFrontRightBottom:
	case EUxtAffordancePlacement::CornerFrontLeftTop:
	case EUxtAffordancePlacement::CornerFrontLeftBottom:
	case EUxtAffordancePlacement::CornerBackRightTop:
	case EUxtAffordancePlacement::CornerBackRightBottom:
	case EUxtAffordancePlacement::CornerBackLeftTop:
	case EUxtAffordancePlacement::CornerBackLeftBottom:
		return EUxtAffordanceAction::Scale;
	}
}

void FUxtAffordanceConfig::GetWorldLocationAndRotation(
	const FBox& Bounds, const FTransform& RootTransform, FVector& OutLocation, FQuat& OutRotation) const
{
	OutLocation = RootTransform.TransformPosition(Bounds.GetCenter() + Bounds.GetExtent() * GetBoundsLocation());
	OutRotation = RootTransform.TransformRotation(GetBoundsRotation().Quaternion());
}
