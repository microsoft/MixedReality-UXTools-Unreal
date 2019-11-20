// Fill out your copyright notice in the Description page of Project Settings.

#include "BoundingBoxManipulatorPresets.h"
#include "BoundingBoxManipulatorComponent.h"
#include "MixedRealityMathUtilsFunctionLibrary.h"


enum class EBoundingBoxAffordancePreset : uint8
{
	Center,
	FaceFront,
	FaceBack,
	FaceRight,
	FaceLeft,
	FaceTop,
	FaceBottom,
	EdgeFrontRight,
	EdgeFrontLeft,
	EdgeFrontTop,
	EdgeFrontBottom,
	EdgeBackRight,
	EdgeBackLeft,
	EdgeBackTop,
	EdgeBackBottom,
	EdgeRightTop,
	EdgeRightBottom,
	EdgeLeftTop,
	EdgeLeftBottom,
	CornerFrontRightTop,
	CornerFrontRightBottom,
	CornerFrontLeftTop,
	CornerFrontLeftBottom,
	CornerBackRightTop,
	CornerBackRightBottom,
	CornerBackLeftTop,
	CornerBackLeftBottom,
};


static FMatrix MakeDiagonalMatrix(float x, float y, float z)
{
	float norm2 = x * x + y * y + z * z;
	float invNorm = norm2 > 0.0f ? 1.0f / norm2 : 0.0f;
	return FMatrix(FVector(x*x, 0, 0), FVector(0, y*y, 0), FVector(0, 0, z*z), FVector4(0, 0, 0, 1)) * invNorm;
}

static FMatrix MakeUniformMatrix(float x, float y, float z)
{
	float norm2 = x * x + y * y + z * z;
	float invNorm = norm2 > 0.0f ? 1.0f / norm2 : 0.0f;
	return FMatrix(FVector(x*x, x*y, x*z), FVector(y*x, y*y, y*z), FVector(z*x, z*y, z*z), FVector4(0, 0, 0, 1)) * invNorm;
}

static FMatrix MakeAxialConstraintMatrix(const FVector &axis)
{
	float norm2 = axis.SizeSquared();
	float invNorm2 = norm2 > 0.0f ? 1.0f / norm2 : 0.0f;
	// Projection on the axis by outer product: M = outer(a, a)
	float x = axis.X;
	float y = axis.Y;
	float z = axis.Z;
	return FMatrix(FVector(x*x, x*y, x*z), FVector(y*x, y*y, y*z), FVector(z*x, z*y, z*z), FVector4(0, 0, 0, 1)) * invNorm2;
}

static FMatrix MakePlanarConstraintMatrix(const FVector &normal)
{
	float norm2 = normal.SizeSquared();
	float invNorm2 = norm2 > 0.0f ? 1.0f / norm2 : 0.0f;
	// Projection on the plane by outer product: M = I - outer(n, n)
	float x = normal.X;
	float y = normal.Y;
	float z = normal.Z;
	return FMatrix(FVector(1.0f - x * x, x*y, x*z), FVector(y*x, 1.0f - y * y, y*z), FVector(z*x, z*y, 1.0f - z * z), FVector4(0, 0, 0, 1)) * invNorm2;
}

static FVector GetAffordancePresetLocation(EBoundingBoxAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundingBoxAffordancePreset::Center:					return FVector(0, 0, 0);

	case EBoundingBoxAffordancePreset::FaceFront:				return FVector(1, 0, 0);
	case EBoundingBoxAffordancePreset::FaceBack:				return FVector(-1, 0, 0);
	case EBoundingBoxAffordancePreset::FaceRight:				return FVector(0, 1, 0);
	case EBoundingBoxAffordancePreset::FaceLeft:				return FVector(0, -1, 0);
	case EBoundingBoxAffordancePreset::FaceTop:					return FVector(0, 0, 1);
	case EBoundingBoxAffordancePreset::FaceBottom:				return FVector(0, 0, -1);

	case EBoundingBoxAffordancePreset::EdgeFrontRight:			return FVector(1, 1, 0);
	case EBoundingBoxAffordancePreset::EdgeFrontLeft:			return FVector(1, -1, 0);
	case EBoundingBoxAffordancePreset::EdgeFrontTop:			return FVector(1, 0, 1);
	case EBoundingBoxAffordancePreset::EdgeFrontBottom:			return FVector(1, 0, -1);
	case EBoundingBoxAffordancePreset::EdgeBackRight:			return FVector(-1, 1, 0);
	case EBoundingBoxAffordancePreset::EdgeBackLeft:			return FVector(-1, -1, 0);
	case EBoundingBoxAffordancePreset::EdgeBackTop:				return FVector(-1, 0, 1);
	case EBoundingBoxAffordancePreset::EdgeBackBottom:			return FVector(-1, 0, -1);
	case EBoundingBoxAffordancePreset::EdgeRightTop:			return FVector(0, 1, 1);
	case EBoundingBoxAffordancePreset::EdgeRightBottom:			return FVector(0, 1, -1);
	case EBoundingBoxAffordancePreset::EdgeLeftTop:				return FVector(0, -1, 1);
	case EBoundingBoxAffordancePreset::EdgeLeftBottom:			return FVector(0, -1, -1);

	case EBoundingBoxAffordancePreset::CornerFrontRightTop:		return FVector(1, 1, 1);
	case EBoundingBoxAffordancePreset::CornerFrontRightBottom:	return FVector(1, 1, -1);
	case EBoundingBoxAffordancePreset::CornerFrontLeftTop:		return FVector(1, -1, 1);
	case EBoundingBoxAffordancePreset::CornerFrontLeftBottom:	return FVector(1, -1, -1);
	case EBoundingBoxAffordancePreset::CornerBackRightTop:		return FVector(-1, 1, 1);
	case EBoundingBoxAffordancePreset::CornerBackRightBottom:	return FVector(-1, 1, -1);
	case EBoundingBoxAffordancePreset::CornerBackLeftTop:		return FVector(-1, -1, 1);
	case EBoundingBoxAffordancePreset::CornerBackLeftBottom:	return FVector(-1, -1, -1);
	}

	return FVector::ZeroVector;
}

static FRotator GetAffordancePresetRotation(EBoundingBoxAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundingBoxAffordancePreset::Center:						return FRotator(0, 0, 0);
	
	case EBoundingBoxAffordancePreset::FaceFront:					return FRotator(0, 0, 0);
	case EBoundingBoxAffordancePreset::FaceBack:					return FRotator(0, 180, 0);
	case EBoundingBoxAffordancePreset::FaceRight:					return FRotator(0, 90, 0);
	case EBoundingBoxAffordancePreset::FaceLeft:					return FRotator(0, 270, 0);
	case EBoundingBoxAffordancePreset::FaceTop:						return FRotator(90, 0, 0);
	case EBoundingBoxAffordancePreset::FaceBottom:					return FRotator(270, 0, 0);
	
	case EBoundingBoxAffordancePreset::EdgeFrontRight:				return FRotator(0, 0, 0);
	case EBoundingBoxAffordancePreset::EdgeFrontLeft:				return FRotator(0, 270, 0);
	case EBoundingBoxAffordancePreset::EdgeFrontTop:				return FRotator(90, 0, 90);
	case EBoundingBoxAffordancePreset::EdgeFrontBottom:				return FRotator(0, 0, 90);
	case EBoundingBoxAffordancePreset::EdgeBackRight:				return FRotator(0, 90, 0);
	case EBoundingBoxAffordancePreset::EdgeBackLeft:				return FRotator(0, 180, 0);
	case EBoundingBoxAffordancePreset::EdgeBackTop:					return FRotator(180, 0, 90);
	case EBoundingBoxAffordancePreset::EdgeBackBottom:				return FRotator(270, 0, 90);
	case EBoundingBoxAffordancePreset::EdgeRightTop:				return FRotator(90, 0, 0);
	case EBoundingBoxAffordancePreset::EdgeRightBottom:				return FRotator(270, 0, 0);
	case EBoundingBoxAffordancePreset::EdgeLeftTop:					return FRotator(90, 90, 270);
	case EBoundingBoxAffordancePreset::EdgeLeftBottom:				return FRotator(270, 0, 180);
	
	case EBoundingBoxAffordancePreset::CornerFrontRightTop:			return FRotator(0, 0, 0);
	case EBoundingBoxAffordancePreset::CornerFrontRightBottom:		return FRotator(0, 0, 90);
	case EBoundingBoxAffordancePreset::CornerFrontLeftTop:			return FRotator(0, 270, 0);
	case EBoundingBoxAffordancePreset::CornerFrontLeftBottom:		return FRotator(0, 270, 90);
	case EBoundingBoxAffordancePreset::CornerBackRightTop:			return FRotator(0, 90, 0);
	case EBoundingBoxAffordancePreset::CornerBackRightBottom:		return FRotator(0, 90, 90);
	case EBoundingBoxAffordancePreset::CornerBackLeftTop:			return FRotator(0, 180, 0);
	case EBoundingBoxAffordancePreset::CornerBackLeftBottom:		return FRotator(0, 180, 90);
	}

	return FRotator::ZeroRotator;
}

static EBoundingBoxAffordanceKind GetAffordancePresetKind(EBoundingBoxAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundingBoxAffordancePreset::Center:						return EBoundingBoxAffordanceKind::Center;

	case EBoundingBoxAffordancePreset::FaceFront:					return EBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceBack:					return EBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceRight:					return EBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceLeft:					return EBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceTop:						return EBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceBottom:					return EBoundingBoxAffordanceKind::Face;

	case EBoundingBoxAffordancePreset::EdgeFrontRight:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeFrontLeft:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeFrontTop:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeFrontBottom:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackRight:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackLeft:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackTop:					return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackBottom:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeRightTop:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeRightBottom:				return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeLeftTop:					return EBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeLeftBottom:				return EBoundingBoxAffordanceKind::Edge;

	case EBoundingBoxAffordancePreset::CornerFrontRightTop:			return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerFrontRightBottom:		return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerFrontLeftTop:			return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerFrontLeftBottom:		return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackRightTop:			return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackRightBottom:		return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackLeftTop:			return EBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackLeftBottom:		return EBoundingBoxAffordanceKind::Corner;
	}

	return EBoundingBoxAffordanceKind::Center;
}

/** True if the action supports uniform constraints */
static bool GetUniformConstraintSupport(EBoundingBoxAffordanceAction Action)
{
	switch (Action)
	{
	case EBoundingBoxAffordanceAction::Rotate: return false;
	}
	return true;
}

static FBoundingBoxAffordanceInfo MakeAffordanceFromPreset(EBoundingBoxAffordancePreset Preset, EBoundingBoxAffordanceAction Action, bool uniformAction = true)
{
	FBoundingBoxAffordanceInfo affordance;
	affordance.Kind = GetAffordancePresetKind(Preset);
	affordance.Action = Action;
	affordance.BoundsLocation = GetAffordancePresetLocation(Preset);

	// Compute rotation from the reference direction
	affordance.BoundsRotation = GetAffordancePresetRotation(Preset);

	// Allow movement on non-aligned axes
	float cx = affordance.BoundsLocation.X;
	float cy = affordance.BoundsLocation.Y;
	float cz = affordance.BoundsLocation.Z;
	if (GetUniformConstraintSupport(Action) && uniformAction)
	{
		affordance.ConstraintMatrix = MakeUniformMatrix(cx, cy, cz);
	}
	else
	{
		affordance.ConstraintMatrix = MakeDiagonalMatrix(cx, cy, cz);
	}

	return affordance;
}


static const TArray<FBoundingBoxAffordanceInfo> Preset_Empty = TArray<FBoundingBoxAffordanceInfo>();

static const TArray<FBoundingBoxAffordanceInfo> Preset_CornerResizeEdgeRotate = TArray<FBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EBoundingBoxAffordanceAction::Rotate),
};

static const TArray<FBoundingBoxAffordanceInfo> Preset_Slate2D = TArray<FBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EBoundingBoxAffordanceAction::Resize),
};

static const TArray<FBoundingBoxAffordanceInfo> Preset_AllResize = TArray<FBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EBoundingBoxAffordanceAction::Resize),
};

static const TArray<FBoundingBoxAffordanceInfo> Preset_AllTranslate = TArray<FBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EBoundingBoxAffordanceAction::Translate),
};

static const TArray<FBoundingBoxAffordanceInfo> Preset_AllScale = TArray<FBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EBoundingBoxAffordanceAction::Scale),
};

static const TArray<FBoundingBoxAffordanceInfo> Preset_AllRotate = TArray<FBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EBoundingBoxAffordanceAction::Rotate),
};

const TArray<FBoundingBoxAffordanceInfo> &BoundingBoxPresetUtils::GetPresetAffordances(EBoundingBoxManipulatorPreset Preset)
{
	switch (Preset)
	{
	case EBoundingBoxManipulatorPreset::Default:	return Preset_CornerResizeEdgeRotate;
	case EBoundingBoxManipulatorPreset::Slate2D:					return Preset_Slate2D;
	case EBoundingBoxManipulatorPreset::AllResize:					return Preset_AllResize;
	case EBoundingBoxManipulatorPreset::AllTranslate:				return Preset_AllTranslate;
	case EBoundingBoxManipulatorPreset::AllScale:					return Preset_AllScale;
	case EBoundingBoxManipulatorPreset::AllRotate:					return Preset_AllRotate;
	}
	return Preset_Empty;
}
