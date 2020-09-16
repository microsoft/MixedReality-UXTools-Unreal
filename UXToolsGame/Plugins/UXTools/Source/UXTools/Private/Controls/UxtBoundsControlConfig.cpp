// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlConfig.h"

namespace
{
	FMatrix MakeDiagonalMatrix(float X, float Y, float Z)
	{
		float Norm2 = X * X + Y * Y + Z * Z;
		float InvNorm = Norm2 > 0.0f ? 1.0f / Norm2 : 0.0f;
		return FMatrix(FVector(X * X, 0, 0), FVector(0, Y * Y, 0), FVector(0, 0, Z * Z), FVector4(0, 0, 0, 1)) * InvNorm;
	}

	FMatrix MakeUniformMatrix(float X, float Y, float Z)
	{
		float Norm2 = X * X + Y * Y + Z * Z;
		float InvNorm = Norm2 > 0.0f ? 1.0f / Norm2 : 0.0f;
		return FMatrix(FVector(X * X, X * Y, X * Z), FVector(Y * X, Y * Y, Y * Z), FVector(Z * X, Z * Y, Z * Z), FVector4(0, 0, 0, 1)) *
			   InvNorm;
	}

	FMatrix MakeAxialConstraintMatrix(const FVector& Axis)
	{
		float Norm2 = Axis.SizeSquared();
		float InvNorm2 = Norm2 > 0.0f ? 1.0f / Norm2 : 0.0f;
		// Projection on the axis by outer product: M = outer(a, a)
		float X = Axis.X;
		float Y = Axis.Y;
		float Z = Axis.Z;
		return FMatrix(FVector(X * X, X * Y, X * Z), FVector(Y * X, Y * Y, Y * Z), FVector(Z * X, Z * Y, Z * Z), FVector4(0, 0, 0, 1)) *
			   InvNorm2;
	}

	FMatrix MakePlanarConstraintMatrix(const FVector& Normal)
	{
		float Norm2 = Normal.SizeSquared();
		float InvNorm2 = Norm2 > 0.0f ? 1.0f / Norm2 : 0.0f;
		// Projection on the plane by outer product: M = I - outer(n, n)
		float X = Normal.X;
		float Y = Normal.Y;
		float Z = Normal.Z;
		return FMatrix(
				   FVector(1.0f - X * X, X * Y, X * Z), FVector(Y * X, 1.0f - Y * Y, Y * Z), FVector(Z * X, Z * Y, 1.0f - Z * Z),
				   FVector4(0, 0, 0, 1)) *
			   InvNorm2;
	}

	/** True if the action supports uniform constraints */
	bool IsUniformConstraintSupported(EUxtAffordanceAction Action)
	{
		switch (Action)
		{
		case EUxtAffordanceAction::Rotate:
			return false;
		}
		return true;
	}
} // namespace

FVector FUxtAffordanceConfig::GetBoundsLocation() const
{
	switch (Placement)
	{
	case EUxtAffordancePlacement::Center:
		return FVector(0, 0, 0);

	case EUxtAffordancePlacement::FaceFront:
		return FVector(1, 0, 0);
	case EUxtAffordancePlacement::FaceBack:
		return FVector(-1, 0, 0);
	case EUxtAffordancePlacement::FaceRight:
		return FVector(0, 1, 0);
	case EUxtAffordancePlacement::FaceLeft:
		return FVector(0, -1, 0);
	case EUxtAffordancePlacement::FaceTop:
		return FVector(0, 0, 1);
	case EUxtAffordancePlacement::FaceBottom:
		return FVector(0, 0, -1);

	case EUxtAffordancePlacement::EdgeFrontRight:
		return FVector(1, 1, 0);
	case EUxtAffordancePlacement::EdgeFrontLeft:
		return FVector(1, -1, 0);
	case EUxtAffordancePlacement::EdgeFrontTop:
		return FVector(1, 0, 1);
	case EUxtAffordancePlacement::EdgeFrontBottom:
		return FVector(1, 0, -1);
	case EUxtAffordancePlacement::EdgeBackRight:
		return FVector(-1, 1, 0);
	case EUxtAffordancePlacement::EdgeBackLeft:
		return FVector(-1, -1, 0);
	case EUxtAffordancePlacement::EdgeBackTop:
		return FVector(-1, 0, 1);
	case EUxtAffordancePlacement::EdgeBackBottom:
		return FVector(-1, 0, -1);
	case EUxtAffordancePlacement::EdgeRightTop:
		return FVector(0, 1, 1);
	case EUxtAffordancePlacement::EdgeRightBottom:
		return FVector(0, 1, -1);
	case EUxtAffordancePlacement::EdgeLeftTop:
		return FVector(0, -1, 1);
	case EUxtAffordancePlacement::EdgeLeftBottom:
		return FVector(0, -1, -1);

	case EUxtAffordancePlacement::CornerFrontRightTop:
		return FVector(1, 1, 1);
	case EUxtAffordancePlacement::CornerFrontRightBottom:
		return FVector(1, 1, -1);
	case EUxtAffordancePlacement::CornerFrontLeftTop:
		return FVector(1, -1, 1);
	case EUxtAffordancePlacement::CornerFrontLeftBottom:
		return FVector(1, -1, -1);
	case EUxtAffordancePlacement::CornerBackRightTop:
		return FVector(-1, 1, 1);
	case EUxtAffordancePlacement::CornerBackRightBottom:
		return FVector(-1, 1, -1);
	case EUxtAffordancePlacement::CornerBackLeftTop:
		return FVector(-1, -1, 1);
	case EUxtAffordancePlacement::CornerBackLeftBottom:
		return FVector(-1, -1, -1);
	}

	return FVector::ZeroVector;
}

FRotator FUxtAffordanceConfig::GetBoundsRotation() const
{
	switch (Placement)
	{
	case EUxtAffordancePlacement::Center:
		return FRotator(0, 0, 0);

	case EUxtAffordancePlacement::FaceFront:
		return FRotator(0, 0, 0);
	case EUxtAffordancePlacement::FaceBack:
		return FRotator(0, 180, 0);
	case EUxtAffordancePlacement::FaceRight:
		return FRotator(0, 90, 0);
	case EUxtAffordancePlacement::FaceLeft:
		return FRotator(0, 270, 0);
	case EUxtAffordancePlacement::FaceTop:
		return FRotator(90, 0, 0);
	case EUxtAffordancePlacement::FaceBottom:
		return FRotator(270, 0, 0);

	case EUxtAffordancePlacement::EdgeFrontRight:
		return FRotator(0, 0, 0);
	case EUxtAffordancePlacement::EdgeFrontLeft:
		return FRotator(0, 270, 0);
	case EUxtAffordancePlacement::EdgeFrontTop:
		return FRotator(90, 0, 90);
	case EUxtAffordancePlacement::EdgeFrontBottom:
		return FRotator(0, 0, 90);
	case EUxtAffordancePlacement::EdgeBackRight:
		return FRotator(0, 90, 0);
	case EUxtAffordancePlacement::EdgeBackLeft:
		return FRotator(0, 180, 0);
	case EUxtAffordancePlacement::EdgeBackTop:
		return FRotator(180, 0, 90);
	case EUxtAffordancePlacement::EdgeBackBottom:
		return FRotator(270, 0, 90);
	case EUxtAffordancePlacement::EdgeRightTop:
		return FRotator(90, 0, 0);
	case EUxtAffordancePlacement::EdgeRightBottom:
		return FRotator(270, 0, 0);
	case EUxtAffordancePlacement::EdgeLeftTop:
		return FRotator(90, 90, 270);
	case EUxtAffordancePlacement::EdgeLeftBottom:
		return FRotator(270, 0, 180);

	case EUxtAffordancePlacement::CornerFrontRightTop:
		return FRotator(0, 0, 0);
	case EUxtAffordancePlacement::CornerFrontRightBottom:
		return FRotator(0, 0, 90);
	case EUxtAffordancePlacement::CornerFrontLeftTop:
		return FRotator(0, 270, 0);
	case EUxtAffordancePlacement::CornerFrontLeftBottom:
		return FRotator(0, 270, 90);
	case EUxtAffordancePlacement::CornerBackRightTop:
		return FRotator(0, 90, 0);
	case EUxtAffordancePlacement::CornerBackRightBottom:
		return FRotator(0, 90, 90);
	case EUxtAffordancePlacement::CornerBackLeftTop:
		return FRotator(0, 180, 0);
	case EUxtAffordancePlacement::CornerBackLeftBottom:
		return FRotator(0, 180, 90);
	}

	return FRotator::ZeroRotator;
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

FMatrix FUxtAffordanceConfig::GetConstraintMatrix() const
{
	// Allow movement on non-aligned axes
	const FVector P = GetBoundsLocation();
	if (bUniformAction && IsUniformConstraintSupported(Action))
	{
		return MakeUniformMatrix(P.X, P.Y, P.Z);
	}
	else
	{
		return MakeDiagonalMatrix(P.X, P.Y, P.Z);
	}
}

FTransform FUxtAffordanceConfig::GetWorldTransform(const FBox& Bounds, const FTransform& RootTransform) const
{
	FVector Location = Bounds.GetCenter() + Bounds.GetExtent() * GetBoundsLocation();
	FRotator Rotation = GetBoundsRotation();
	FVector Scale = FVector::OneVector;

	return FTransform(RootTransform.TransformRotation(FQuat(Rotation)), RootTransform.TransformPosition(Location), Scale);
}
