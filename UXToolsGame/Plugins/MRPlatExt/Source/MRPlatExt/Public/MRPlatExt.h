// Copyright (c) Microsoft Corporation.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/InputComponent.h"

#include "MRPlatExt.generated.h"

USTRUCT(BlueprintType, Category = "MRPlatExt|OpenXR")
struct FKeywordInput
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Keyword;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FInputActionHandlerDynamicSignature Callback;
};

UENUM(BlueprintType, Category = "MRPlatExt|OpenXR")
enum class EHandMeshStatus : uint8
{
	NotInitialised = 0  UMETA(Hidden),
	Disabled = 1, 
	EnabledTrackingGeometry = 2, 
	EnabledXRVisualization = 3
};

UCLASS(ClassGroup = OpenXR)
class MRPLATEXT_API UMRPlatExtFunctionLibrary :
	public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	Turn Hand Mesh

	@param On true if enable
	@return true if the command successes
	*/
	UFUNCTION(BlueprintCallable, Category = "MRPlatExt|OpenXR")
	static bool TurnHandMesh(EHandMeshStatus Mode);

	/**
	Is QR Tracking enabled

	@return true if the command successes
	*/
	UFUNCTION(BlueprintPure, Category = "MRPlatExt|OpenXR")
	static bool IsQREnabled();

	/**
	 * Get the transform from PV camera space to Unreal world space.
	 */
	UFUNCTION(BlueprintPure, Category = "MRPlatExt|OpenXR")
	static FTransform GetPVCameraToWorldTransform();

	/**
	 * Get the PV Camera intrinsics.
	 */
	UFUNCTION(BlueprintPure, Category = "MRPlatExt|OpenXR")
	static bool GetPVCameraIntrinsics(FVector2D& focalLength, int& width, int& height, FVector2D& principalPoint, FVector& radialDistortion, FVector2D& tangentialDistortion);

	/**
	 * Get a ray into the scene from a camera point.
	 * X is left/right
	 * Y is up/down
	 */
	UFUNCTION(BlueprintPure, Category = "MRPlatExt|OpenXR")
	static FVector GetWorldSpaceRayFromCameraPoint(FVector2D pixelCoordinate);

	/**
	Check if the current platform supports speech recognition.
	*/
	UFUNCTION(BlueprintPure, Category = "MRPlatExt|OpenXR")
	static bool IsSpeechRecognitionAvailable();

	/**
	Add new speech keywords with associated callbacks.

	@param Keywords list of keyword and callbacks to add.
	*/
	UFUNCTION(BlueprintCallable, Category = "MRPlatExt|OpenXR")
	static void AddKeywords(TArray<FKeywordInput> Keywords);

	/**
	Remove speech keywords.

	@param Keywords list of keyword to remove.
	*/
	UFUNCTION(BlueprintCallable, Category = "MRPlatExt|OpenXR")
	static void RemoveKeywords(TArray<FString> Keywords);

};

