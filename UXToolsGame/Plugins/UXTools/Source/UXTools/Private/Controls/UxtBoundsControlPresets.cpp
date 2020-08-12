// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlPresets.h"
#include "Controls/UxtBoundsControlComponent.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"


enum class EBoundsControlAffordancePreset : uint8
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

static FVector GetAffordancePresetLocation(EBoundsControlAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundsControlAffordancePreset::Center:					return FVector(0, 0, 0);

	case EBoundsControlAffordancePreset::FaceFront:				return FVector(1, 0, 0);
	case EBoundsControlAffordancePreset::FaceBack:				return FVector(-1, 0, 0);
	case EBoundsControlAffordancePreset::FaceRight:				return FVector(0, 1, 0);
	case EBoundsControlAffordancePreset::FaceLeft:				return FVector(0, -1, 0);
	case EBoundsControlAffordancePreset::FaceTop:					return FVector(0, 0, 1);
	case EBoundsControlAffordancePreset::FaceBottom:				return FVector(0, 0, -1);

	case EBoundsControlAffordancePreset::EdgeFrontRight:			return FVector(1, 1, 0);
	case EBoundsControlAffordancePreset::EdgeFrontLeft:			return FVector(1, -1, 0);
	case EBoundsControlAffordancePreset::EdgeFrontTop:			return FVector(1, 0, 1);
	case EBoundsControlAffordancePreset::EdgeFrontBottom:			return FVector(1, 0, -1);
	case EBoundsControlAffordancePreset::EdgeBackRight:			return FVector(-1, 1, 0);
	case EBoundsControlAffordancePreset::EdgeBackLeft:			return FVector(-1, -1, 0);
	case EBoundsControlAffordancePreset::EdgeBackTop:				return FVector(-1, 0, 1);
	case EBoundsControlAffordancePreset::EdgeBackBottom:			return FVector(-1, 0, -1);
	case EBoundsControlAffordancePreset::EdgeRightTop:			return FVector(0, 1, 1);
	case EBoundsControlAffordancePreset::EdgeRightBottom:			return FVector(0, 1, -1);
	case EBoundsControlAffordancePreset::EdgeLeftTop:				return FVector(0, -1, 1);
	case EBoundsControlAffordancePreset::EdgeLeftBottom:			return FVector(0, -1, -1);

	case EBoundsControlAffordancePreset::CornerFrontRightTop:		return FVector(1, 1, 1);
	case EBoundsControlAffordancePreset::CornerFrontRightBottom:	return FVector(1, 1, -1);
	case EBoundsControlAffordancePreset::CornerFrontLeftTop:		return FVector(1, -1, 1);
	case EBoundsControlAffordancePreset::CornerFrontLeftBottom:	return FVector(1, -1, -1);
	case EBoundsControlAffordancePreset::CornerBackRightTop:		return FVector(-1, 1, 1);
	case EBoundsControlAffordancePreset::CornerBackRightBottom:	return FVector(-1, 1, -1);
	case EBoundsControlAffordancePreset::CornerBackLeftTop:		return FVector(-1, -1, 1);
	case EBoundsControlAffordancePreset::CornerBackLeftBottom:	return FVector(-1, -1, -1);
	}

	return FVector::ZeroVector;
}

static FRotator GetAffordancePresetRotation(EBoundsControlAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundsControlAffordancePreset::Center:						return FRotator(0, 0, 0);
	
	case EBoundsControlAffordancePreset::FaceFront:					return FRotator(0, 0, 0);
	case EBoundsControlAffordancePreset::FaceBack:					return FRotator(0, 180, 0);
	case EBoundsControlAffordancePreset::FaceRight:					return FRotator(0, 90, 0);
	case EBoundsControlAffordancePreset::FaceLeft:					return FRotator(0, 270, 0);
	case EBoundsControlAffordancePreset::FaceTop:						return FRotator(90, 0, 0);
	case EBoundsControlAffordancePreset::FaceBottom:					return FRotator(270, 0, 0);
	
	case EBoundsControlAffordancePreset::EdgeFrontRight:				return FRotator(0, 0, 0);
	case EBoundsControlAffordancePreset::EdgeFrontLeft:				return FRotator(0, 270, 0);
	case EBoundsControlAffordancePreset::EdgeFrontTop:				return FRotator(90, 0, 90);
	case EBoundsControlAffordancePreset::EdgeFrontBottom:				return FRotator(0, 0, 90);
	case EBoundsControlAffordancePreset::EdgeBackRight:				return FRotator(0, 90, 0);
	case EBoundsControlAffordancePreset::EdgeBackLeft:				return FRotator(0, 180, 0);
	case EBoundsControlAffordancePreset::EdgeBackTop:					return FRotator(180, 0, 90);
	case EBoundsControlAffordancePreset::EdgeBackBottom:				return FRotator(270, 0, 90);
	case EBoundsControlAffordancePreset::EdgeRightTop:				return FRotator(90, 0, 0);
	case EBoundsControlAffordancePreset::EdgeRightBottom:				return FRotator(270, 0, 0);
	case EBoundsControlAffordancePreset::EdgeLeftTop:					return FRotator(90, 90, 270);
	case EBoundsControlAffordancePreset::EdgeLeftBottom:				return FRotator(270, 0, 180);
	
	case EBoundsControlAffordancePreset::CornerFrontRightTop:			return FRotator(0, 0, 0);
	case EBoundsControlAffordancePreset::CornerFrontRightBottom:		return FRotator(0, 0, 90);
	case EBoundsControlAffordancePreset::CornerFrontLeftTop:			return FRotator(0, 270, 0);
	case EBoundsControlAffordancePreset::CornerFrontLeftBottom:		return FRotator(0, 270, 90);
	case EBoundsControlAffordancePreset::CornerBackRightTop:			return FRotator(0, 90, 0);
	case EBoundsControlAffordancePreset::CornerBackRightBottom:		return FRotator(0, 90, 90);
	case EBoundsControlAffordancePreset::CornerBackLeftTop:			return FRotator(0, 180, 0);
	case EBoundsControlAffordancePreset::CornerBackLeftBottom:		return FRotator(0, 180, 90);
	}

	return FRotator::ZeroRotator;
}

static EUxtBoundsControlAffordanceKind GetAffordancePresetKind(EBoundsControlAffordancePreset Preset)
{
	switch (Preset)
	{
	case EBoundsControlAffordancePreset::Center:						return EUxtBoundsControlAffordanceKind::Center;

	case EBoundsControlAffordancePreset::FaceFront:					return EUxtBoundsControlAffordanceKind::Face;
	case EBoundsControlAffordancePreset::FaceBack:					return EUxtBoundsControlAffordanceKind::Face;
	case EBoundsControlAffordancePreset::FaceRight:					return EUxtBoundsControlAffordanceKind::Face;
	case EBoundsControlAffordancePreset::FaceLeft:					return EUxtBoundsControlAffordanceKind::Face;
	case EBoundsControlAffordancePreset::FaceTop:						return EUxtBoundsControlAffordanceKind::Face;
	case EBoundsControlAffordancePreset::FaceBottom:					return EUxtBoundsControlAffordanceKind::Face;

	case EBoundsControlAffordancePreset::EdgeFrontRight:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeFrontLeft:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeFrontTop:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeFrontBottom:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeBackRight:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeBackLeft:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeBackTop:					return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeBackBottom:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeRightTop:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeRightBottom:				return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeLeftTop:					return EUxtBoundsControlAffordanceKind::Edge;
	case EBoundsControlAffordancePreset::EdgeLeftBottom:				return EUxtBoundsControlAffordanceKind::Edge;

	case EBoundsControlAffordancePreset::CornerFrontRightTop:			return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerFrontRightBottom:		return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerFrontLeftTop:			return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerFrontLeftBottom:		return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerBackRightTop:			return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerBackRightBottom:		return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerBackLeftTop:			return EUxtBoundsControlAffordanceKind::Corner;
	case EBoundsControlAffordancePreset::CornerBackLeftBottom:		return EUxtBoundsControlAffordanceKind::Corner;
	}

	return EUxtBoundsControlAffordanceKind::Center;
}

/** True if the action supports uniform constraints */
static bool GetUniformConstraintSupport(EUxtBoundsControlAffordanceAction Action)
{
	switch (Action)
	{
	case EUxtBoundsControlAffordanceAction::Rotate: return false;
	}
	return true;
}

static FUxtBoundsControlAffordanceInfo MakeAffordanceFromPreset(EBoundsControlAffordancePreset Preset, EUxtBoundsControlAffordanceAction Action, bool uniformAction = true)
{
	FUxtBoundsControlAffordanceInfo affordance;
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


static const TArray<FUxtBoundsControlAffordanceInfo> Preset_Empty = TArray<FUxtBoundsControlAffordanceInfo>();

static const TArray<FUxtBoundsControlAffordanceInfo> Preset_CornerResizeEdgeRotate = TArray<FUxtBoundsControlAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontRight,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontLeft,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontTop,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontBottom,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackRight,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackLeft,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackTop,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackBottom,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightTop,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightBottom,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftTop,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftBottom,			EUxtBoundsControlAffordanceAction::Rotate),
};

static const TArray<FUxtBoundsControlAffordanceInfo> Preset_Slate2D = TArray<FUxtBoundsControlAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontRight,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontLeft,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontTop,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontBottom,			EUxtBoundsControlAffordanceAction::Resize),
};

static const TArray<FUxtBoundsControlAffordanceInfo> Preset_AllResize = TArray<FUxtBoundsControlAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::Center,					EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceFront,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBack,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceRight,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceLeft,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceTop,					EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBottom,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftTop,		EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftBottom,	EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontRight,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontLeft,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontTop,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontBottom,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackRight,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackLeft,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackTop,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackBottom,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightTop,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightBottom,			EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftTop,				EUxtBoundsControlAffordanceAction::Resize),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftBottom,			EUxtBoundsControlAffordanceAction::Resize),
};

static const TArray<FUxtBoundsControlAffordanceInfo> Preset_AllTranslate = TArray<FUxtBoundsControlAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::Center,					EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceFront,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBack,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceRight,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceLeft,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceTop,					EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBottom,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightTop,		EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightBottom,	EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftTop,		EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftBottom,	EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightTop,		EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightBottom,	EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftTop,		EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftBottom,	EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontRight,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontLeft,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontTop,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontBottom,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackRight,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackLeft,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackTop,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackBottom,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightTop,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightBottom,			EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftTop,				EUxtBoundsControlAffordanceAction::Translate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftBottom,			EUxtBoundsControlAffordanceAction::Translate),
};

static const TArray<FUxtBoundsControlAffordanceInfo> Preset_AllScale = TArray<FUxtBoundsControlAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::Center,					EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceFront,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBack,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceRight,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceLeft,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceTop,					EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBottom,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightTop,		EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightBottom,	EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftTop,		EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftBottom,	EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightTop,		EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightBottom,	EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftTop,		EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftBottom,	EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontRight,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontLeft,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontTop,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontBottom,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackRight,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackLeft,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackTop,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackBottom,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightTop,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightBottom,			EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftTop,				EUxtBoundsControlAffordanceAction::Scale),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftBottom,			EUxtBoundsControlAffordanceAction::Scale),
};

static const TArray<FUxtBoundsControlAffordanceInfo> Preset_AllRotate = TArray<FUxtBoundsControlAffordanceInfo>
{
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::Center,					EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceFront,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBack,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceRight,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceLeft,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceTop,					EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::FaceBottom,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightTop,		EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontRightBottom,	EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftTop,		EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerFrontLeftBottom,	EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightTop,		EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackRightBottom,	EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftTop,		EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::CornerBackLeftBottom,	EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontRight,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontLeft,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontTop,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeFrontBottom,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackRight,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackLeft,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackTop,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeBackBottom,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightTop,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeRightBottom,			EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftTop,				EUxtBoundsControlAffordanceAction::Rotate),
	MakeAffordanceFromPreset(EBoundsControlAffordancePreset::EdgeLeftBottom,			EUxtBoundsControlAffordanceAction::Rotate),
};

const TArray<FUxtBoundsControlAffordanceInfo> &FUxtBoundsControlPresetUtils::GetPresetAffordances(EUxtBoundsControlPreset Preset)
{
	switch (Preset)
	{
	case EUxtBoundsControlPreset::Default:						return Preset_CornerResizeEdgeRotate;
	case EUxtBoundsControlPreset::Slate2D:						return Preset_Slate2D;
	case EUxtBoundsControlPreset::AllResize:					return Preset_AllResize;
	case EUxtBoundsControlPreset::AllTranslate:				return Preset_AllTranslate;
	case EUxtBoundsControlPreset::AllScale:					return Preset_AllScale;
	case EUxtBoundsControlPreset::AllRotate:					return Preset_AllRotate;
	}
	return Preset_Empty;
}

