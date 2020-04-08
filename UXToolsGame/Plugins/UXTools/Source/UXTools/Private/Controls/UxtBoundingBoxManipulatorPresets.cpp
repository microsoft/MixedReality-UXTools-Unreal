// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundingBoxManipulatorPresets.h"
#include "Controls/UxtBoundingBoxManipulatorComponent.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"


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

static EUxtBoundingBoxAffordanceKind GetAffordancePresetKind(EBoundingBoxAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundingBoxAffordancePreset::Center:						return EUxtBoundingBoxAffordanceKind::Center;

	case EBoundingBoxAffordancePreset::FaceFront:					return EUxtBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceBack:					return EUxtBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceRight:					return EUxtBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceLeft:					return EUxtBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceTop:						return EUxtBoundingBoxAffordanceKind::Face;
	case EBoundingBoxAffordancePreset::FaceBottom:					return EUxtBoundingBoxAffordanceKind::Face;

	case EBoundingBoxAffordancePreset::EdgeFrontRight:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeFrontLeft:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeFrontTop:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeFrontBottom:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackRight:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackLeft:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackTop:					return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeBackBottom:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeRightTop:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeRightBottom:				return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeLeftTop:					return EUxtBoundingBoxAffordanceKind::Edge;
	case EBoundingBoxAffordancePreset::EdgeLeftBottom:				return EUxtBoundingBoxAffordanceKind::Edge;

	case EBoundingBoxAffordancePreset::CornerFrontRightTop:			return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerFrontRightBottom:		return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerFrontLeftTop:			return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerFrontLeftBottom:		return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackRightTop:			return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackRightBottom:		return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackLeftTop:			return EUxtBoundingBoxAffordanceKind::Corner;
	case EBoundingBoxAffordancePreset::CornerBackLeftBottom:		return EUxtBoundingBoxAffordanceKind::Corner;
	}

	return EUxtBoundingBoxAffordanceKind::Center;
}

/** True if the action supports uniform constraints */
static bool GetUniformConstraintSupport(EUxtBoundingBoxAffordanceAction Action)
{
	switch (Action)
	{
	case EUxtBoundingBoxAffordanceAction::Rotate: return false;
	}
	return true;
}

static FUxtBoundingBoxAffordanceInfo MakeAffordanceFromPreset(EBoundingBoxAffordancePreset Preset, EUxtBoundingBoxAffordanceAction Action, bool uniformAction = true)
{
	FUxtBoundingBoxAffordanceInfo affordance;
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


static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_Empty = TArray<FUxtBoundingBoxAffordanceInfo>();

static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_CornerResizeEdgeRotate = TArray<FUxtBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
};

static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_Slate2D = TArray<FUxtBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EUxtBoundingBoxAffordanceAction::Resize),
};

static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_AllResize = TArray<FUxtBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EUxtBoundingBoxAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EUxtBoundingBoxAffordanceAction::Resize),
};

static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_AllTranslate = TArray<FUxtBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EUxtBoundingBoxAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EUxtBoundingBoxAffordanceAction::Translate),
};

static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_AllScale = TArray<FUxtBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EUxtBoundingBoxAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EUxtBoundingBoxAffordanceAction::Scale),
};

static const TArray<FUxtBoundingBoxAffordanceInfo> Preset_AllRotate = TArray<FUxtBoundingBoxAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::Center,					EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceFront,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBack,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceRight,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceLeft,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceTop,					EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::FaceBottom,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightTop,		EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontRightBottom,	EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftTop,		EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerFrontLeftBottom,	EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightTop,		EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackRightBottom,	EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftTop,		EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::CornerBackLeftBottom,	EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontRight,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontLeft,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontTop,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeFrontBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackRight,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackLeft,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackTop,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeBackBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightTop,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeRightBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftTop,				EUxtBoundingBoxAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundingBoxAffordancePreset::EdgeLeftBottom,			EUxtBoundingBoxAffordanceAction::Rotate),
};

const TArray<FUxtBoundingBoxAffordanceInfo> &FUxtBoundingBoxPresetUtils::GetPresetAffordances(EUxtBoundingBoxManipulatorPreset Preset)
{
	switch (Preset)
	{
	case EUxtBoundingBoxManipulatorPreset::Default:	return Preset_CornerResizeEdgeRotate;
	case EUxtBoundingBoxManipulatorPreset::Slate2D:					return Preset_Slate2D;
	case EUxtBoundingBoxManipulatorPreset::AllResize:					return Preset_AllResize;
	case EUxtBoundingBoxManipulatorPreset::AllTranslate:				return Preset_AllTranslate;
	case EUxtBoundingBoxManipulatorPreset::AllScale:					return Preset_AllScale;
	case EUxtBoundingBoxManipulatorPreset::AllRotate:					return Preset_AllRotate;
	}
	return Preset_Empty;
}

