// Copyright (c) 2022 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Engine.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "AzureObjectAnchorTypes.generated.h"

UENUM(BlueprintType, Category = "MicrosoftOpenXR|Azure Object Anchors")
enum class EObjectInstanceTrackingMode : uint8
{
	LowLatencyCoarsePosition,
	HighLatencyAccuratePosition
};

/*How to render the tracked object with the MRMesh*/
UENUM(BlueprintType, Category = "MicrosoftOpenXR|Azure Object Anchors")
enum class EObjectRenderMode : uint8
{
	/*Do not render the mesh*/
	None,
	/*Use the full model mesh data*/
	Mesh,
	/*Render just the bounding box*/
	BoundingBox
};

UCLASS(BlueprintType, Blueprintable, ClassGroup = AzureObjectAnchors, Category = "MicrosoftOpenXR|Azure Object Anchors")
class MICROSOFTOPENXR_API UAzureObjectAnchorQueryModifiers : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle), Category = "MicrosoftOpenXR|Azure Object Anchors")
	bool bUseExpectedMaxVerticalOrientationInDegrees = false;

	/*Expected maximum angle in degrees between up direction of an object instance and gravity, from 0 to 180.
	Small value indicates object is expected to be up-right, while large value allows more variation on the layout.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bUseExpectedMaxVerticalOrientationInDegrees"), Category = "MicrosoftOpenXR|Azure Object Anchors")
	float ExpectedMaxVerticalOrientationInDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle), Category = "MicrosoftOpenXR|Azure Object Anchors")
	bool bUseExpectedToBeStandingOnGroundPlane = false;

	/*If true, only look for objects on the ground.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseExpectedToBeStandingOnGroundPlane"), Category = "MicrosoftOpenXR|Azure Object Anchors")
	bool ExpectedToBeStandingOnGroundPlane = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle), Category = "MicrosoftOpenXR|Azure Object Anchors")
	bool bUseMaxScaleChange = false;

	/*Maximum scale change from 1*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUseMaxScaleChange"), Category = "MicrosoftOpenXR|Azure Object Anchors")
	float MaxScaleChange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (InlineEditConditionToggle), Category = "MicrosoftOpenXR|Azure Object Anchors")
	bool bUseMinSurfaceCoverage = false;

	/*Minimum required surface coverage ratio to consider an object instance as true positive, from 0 to 1.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bUseMinSurfaceCoverage"), Category = "MicrosoftOpenXR|Azure Object Anchors")
	float MinSurfaceCoverage = 0.0f;
};

/*Configuration for Azure Object Anchors session.*/
USTRUCT(BlueprintType, Category = "MicrosoftOpenXR|Azure Object Anchors")
struct FAzureObjectAnchorSessionConfiguration
{
	GENERATED_BODY()

	/*Object Anchors Account ID.*/
	UPROPERTY(BlueprintReadWrite, Category = "MicrosoftOpenXR|Azure Object Anchors")
	FString AccountID;

	/*Object Anchors Account Key.*/
	UPROPERTY(BlueprintReadWrite, Category = "MicrosoftOpenXR|Azure Object Anchors")
	FString AccountKey;

	/*Object Anchors Account Domain.*/
	UPROPERTY(BlueprintReadWrite, Category = "MicrosoftOpenXR|Azure Object Anchors")
	FString AccountDomain;

	/*Radius around head to search for models.*/
	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "MicrosoftOpenXR|Azure Object Anchors")
	float SearchRadius = 500;

	/*Tracking mode for tracked objects.*/
	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "MicrosoftOpenXR|Azure Object Anchors")
	EObjectInstanceTrackingMode TrackingMode = EObjectInstanceTrackingMode::LowLatencyCoarsePosition;

	/*How to render the model with the MRMesh. BoundingBox or None can improve performance if the mesh is large.*/
	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "MicrosoftOpenXR|Azure Object Anchors")
	EObjectRenderMode ObjectRenderMode = EObjectRenderMode::Mesh;

	/*Optional query modifiers to use when locating objects.*/
	UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category = "MicrosoftOpenXR|Azure Object Anchors")
	TObjectPtr<UAzureObjectAnchorQueryModifiers> QueryModifiers = nullptr;
};

DECLARE_LOG_CATEGORY_EXTERN(LogAOA, All, All);
