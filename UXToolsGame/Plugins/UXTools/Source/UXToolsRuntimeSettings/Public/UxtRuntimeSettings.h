// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "WindowsMixedRealityFunctionLibrary.h"

#include "Animation/AnimInstance.h"
#include "UObject/Object.h"

#include "UxtRuntimeSettings.generated.h"

class UAnimInstance;
class USkeletalMesh;

USTRUCT()
struct UXTOOLSRUNTIMESETTINGS_API FUxtRuntimeSettingsButtonSet
{
	GENERATED_BODY()

	UPROPERTY(
		EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Buttons", Tooltip = "Buttons that are triggered by a simulated hand gesture."))
	TSet<EHMDInputControllerButtons> Buttons;
};

/**
 * Settings for UXTools.
 */
UCLASS(ClassGroup = "UXTools", config = EditorPerProjectUserSettings)
class UXTOOLSRUNTIMESETTINGS_API UUxtRuntimeSettings : public UObject
{
public:
	GENERATED_BODY()

	UUxtRuntimeSettings(const FObjectInitializer& ObjectInitializer);

	static UUxtRuntimeSettings* Get();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	//
	// Input Simulation Settings

	/** Enable positional head tracking on game start. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Start With Positional Head Tracking", Tooltip = "Enable positional head tracking on game start."))
	bool bStartWithPositionalHeadTracking = true;

	/** Start With Hands Enabled. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Start With Hands Enabled", Tooltip = "If true, hands will start with tracking enabled."))
	bool bStartWithHandsEnabled = true;

	/** Default position of the right hand in camera space. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Default Hand Position", Tooltip = "Default position of the right hand in camera space."))
	FVector DefaultHandPosition = FVector(40, 20, 0);

	/** Position of the right shoulder in camera space. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Shoulder Position", Tooltip = "Position of the right shoulder in camera space."))
	FVector ShoulderPosition = FVector(0, 10, -15);

	/** Default pose when no button is pressed. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Default Hand Pose", Tooltip = "Default pose when no button is pressed."))
	FName DefaultHandPose = TEXT("Relaxed");

	/** Pose mapped to the primary pose action. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Primary Hand Pose", Tooltip = "Pose mapped to the primary pose action."))
	FName PrimaryHandPose = TEXT("Pinch");

	/** Pose mapped to the secondary pose action. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Secondary Hand Pose", Tooltip = "Pose mapped to the secondary pose action."))
	FName SecondaryHandPose = TEXT("Poke");

	/** Pose mapped to the menu action. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Menu Hand Pose", Tooltip = "Pose mapped to the menu action."))
	FName MenuHandPose = TEXT("Flat");

	/** Controller buttons that are pressed along with certain hand poses. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Hand Pose Buttons", Tooltip = "Controller buttons that are pressed along with certain hand poses."))
	TMap<FName, FUxtRuntimeSettingsButtonSet> HandPoseButtonMappings;

	/** Allowed range of hand movement in camera space. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Hand Movement Range", Tooltip = "Allowed range of hand movement in camera space."))
	FBox HandMovementBox = FBox(FVector(10, -40, -40), FVector(60, 40, 40));

	/** Orientation of the right hand in the default pose.
	 *  This is used to determine limits when rotating the hand.
	 */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Hand Rest Orientation", Tooltip = "Orientation of the right hand in the default pose."))
	FRotator HandRestOrientation = FRotator(58.991508, -45.549568, -44.187847);

	/** Skeletal mesh for animating hands.
	 *  The skeleton should contain bones that match the names in the EWMRHandKeypoint enum.
	 *  For more details see the documentation on input simulation.
	 */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Hand Mesh", Tooltip = "Skeletal mesh for animating hands."))
	TSoftObjectPtr<USkeletalMesh> HandMesh;

	/** Animation instance used for animating hand meshes. */
	UPROPERTY(
		GlobalConfig, EditAnywhere, Category = "Uxt Runtime Settings",
		Meta = (DisplayName = "Hand Animation", Tooltip = "Animation instance used for animating hand meshes."))
	TSubclassOf<UAnimInstance> HandAnimInstance;

private:
	static class UUxtRuntimeSettings* UXToolsSettingsSingleton;
};
