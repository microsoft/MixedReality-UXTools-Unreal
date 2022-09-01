// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "UObject/Object.h"

#include "XRSimulationRuntimeSettings.generated.h"

class UAnimBlueprint;
class UAnimInstance;
class USkeletalMesh;

struct XRSIMULATION_API FXRSimulationKeys
{
	static const FKey LeftSelect;
	static const FKey RightSelect;

	static const FKey RightGrip;
	static const FKey LeftGrip;
};

USTRUCT()
struct XRSIMULATION_API FXRSimulationHandPoseKeyMapping
{
	GENERATED_BODY()

	/** Hand that triggers the key. */
	UPROPERTY(EditAnywhere, Category = "XRSimulation")
	EControllerHand Hand;

	/** Simulated hand pose name. */
	UPROPERTY(EditAnywhere, Category = "XRSimulation")
	FName HandPose;

	/** Key that is triggered by the hand pose. */
	UPROPERTY(EditAnywhere, Category = "XRSimulation")
	FKey Key;
};

/**
 * Settings for XRSimulation.
 */
UCLASS(ClassGroup = "XRSimulation", config = EditorPerProjectUserSettings)
class XRSIMULATION_API UXRSimulationRuntimeSettings : public UObject
{
public:
	GENERATED_BODY()

	UXRSimulationRuntimeSettings(const FObjectInitializer& ObjectInitializer);

	static UXRSimulationRuntimeSettings* Get();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	/** Enable XR simulation by setting the simulated HMD to connected state. */
	// XXX When XRSimulationHMD is reinstated make sure to un-comment the ConfigRestartRequired flag
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta =
			(/*ConfigRestartRequired = true,*/ DisplayName = "Enable Simulation",
			 Tooltip = "Enable XR simulation by setting the simulated HMD to connected state."))
	bool bEnableSimulation = true;

	/** Enable positional head tracking on game start. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Start With Positional Head Tracking", Tooltip = "Enable positional head tracking on game start."))
	bool bStartWithPositionalHeadTracking = true;

	/** Maximum speed for HMD movement. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Max HMD Speed", Tooltip = "Maximum speed for HMD movement."))
	float HeadMovementMaxSpeed = 100.0f;

	/** Acceleration of the HMD when moving. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "HMD Acceleration", Tooltip = "Acceleration of the HMD when moving."))
	float HeadMovementAcceleration = 400.f;

	/** Deceleration of the HMD when moving. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "HMD Deceleration", Tooltip = "Deceleration of the HMD when moving."))
	float HeadMovementDeceleration = 1000.f;

	/** Boost to avoid losing speed when HMD is turning. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "HMD Turning Boost", Tooltip = "Boost to avoid losing speed when HMD is turning."))
	float HeadMovementTurningBoost = 8.0f;

	/** Start With Hands Enabled. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Start With Hands Enabled", Tooltip = "If true, hands will start with tracking enabled."))
	bool bStartWithHandsEnabled = true;

	/** Default position of the right hand in camera space. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Default Hand Position", Tooltip = "Default position of the right hand in camera space."))
	FVector DefaultHandPosition = FVector(40, 20, 0);

	/** Position of the right shoulder in camera space. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Shoulder Position", Tooltip = "Position of the right shoulder in camera space."))
	FVector ShoulderPosition = FVector(0, 10, -15);

	/** Default pose when no button is pressed. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Default Hand Pose", Tooltip = "Default pose when no button is pressed."))
	FName DefaultHandPose = TEXT("Relaxed");

	/** Pose mapped to the primary pose action. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Primary Hand Pose", Tooltip = "Pose mapped to the primary pose action."))
	FName PrimaryHandPose = TEXT("Pinch");

	/** Pose mapped to the secondary pose action. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Secondary Hand Pose", Tooltip = "Pose mapped to the secondary pose action."))
	FName SecondaryHandPose = TEXT("Poke");

	/** Pose mapped to the menu action. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Menu Hand Pose", Tooltip = "Pose mapped to the menu action."))
	FName MenuHandPose = TEXT("Flat");

	/** Input keys that are triggered by certain hand poses. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Hand Pose Keys", Tooltip = "Input keys that are triggered by hand poses."))
	TArray<FXRSimulationHandPoseKeyMapping> HandPoseKeys;

	/** Allowed range of hand movement in camera space. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Hand Movement Range", Tooltip = "Allowed range of hand movement in camera space."))
	FBox HandMovementBox = FBox(FVector(10, -40, -40), FVector(60, 40, 40));

	/** Orientation of the right hand in the default pose.
	 *  This is used to determine limits when rotating the hand.
	 */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Hand Rest Orientation", Tooltip = "Orientation of the right hand in the default pose."))
	FRotator HandRestOrientation = FRotator(58.991508, -45.549568, -44.187847);

	/** Skeletal mesh for animating hands.
	 *  The skeleton should contain bones that match the names in the EHandKeypoint enum.
	 */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Hand Mesh", Tooltip = "Skeletal mesh for animating hands."))
	TSoftObjectPtr<USkeletalMesh> HandMesh;

	/** Animation instance used for animating hand meshes. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "XRSimulation",
		Meta = (DisplayName = "Hand Animation", Tooltip = "Animation instance used for animating hand meshes."))
	TSoftObjectPtr<UAnimBlueprint> HandAnimBlueprint;

private:
	static class UXRSimulationRuntimeSettings* XRInputSimSettingsSingleton;
};
