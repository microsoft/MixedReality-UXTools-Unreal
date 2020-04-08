// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Controls/UxtBoundingBoxManipulatorPresets.h"
#include "UxtBoundingBoxManipulatorComponent.generated.h"


/** Defines the kind of actor that should be spawned for an affordance. */
UENUM()
enum class EUxtBoundingBoxAffordanceKind : uint8
{
	Center,
	Face,
	Edge,
	Corner,
};


/** Defines which effect moving an affordance has on the bounding box. */
UENUM()
enum class EUxtBoundingBoxAffordanceAction : uint8
{
	/** Move only one side of the bounding box. */
	Resize,
	/** Move both sides of the bounding box. */
	Translate,
	/** Scale the bounding box, moving both sides in opposite directions. */
	Scale,
	/** Rotate the bounding box about its center point. */
	Rotate,
};


/** Affordances are grabbable actors placed on the bounding box which enable interaction. */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtBoundingBoxAffordanceInfo
{
	GENERATED_BODY()

	/**
	 * Transform from affordance local space to world space, based on the root transform.
	 * Root transform scale is not included in the transform.
	 */
	FTransform GetWorldTransform(const FBox &Bounds, const FTransform &RootTransform) const;

	/** Action to perform when the affordance is grabbed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounding Box")
	EUxtBoundingBoxAffordanceAction Action = EUxtBoundingBoxAffordanceAction::Resize;

	/** Location of the affordance in normalized bounding box space (-1..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounding Box")
	FVector BoundsLocation;

	/** Rotation of the affordance in bounding box space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounding Box")
	FRotator BoundsRotation;

	/**
	 * Constraint matrix defining possible movement directions or rotation axes.
	 * Drag vectors during interaction are multiplied with this matrix.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounding Box")
	FMatrix ConstraintMatrix;

	/** Actor that will be spawned to represent the affordance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounding Box")
	TSubclassOf<class AActor> ActorClass;

	/**
	 * Kind of actor class to use if no explicit actor class is set.
	 * In this case the matching actor class from the bounding box component will be used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounding Box")
	EUxtBoundingBoxAffordanceKind Kind = EUxtBoundingBoxAffordanceKind::Center;
};


/**
 * Manages a set of affordances that can be manipulated for changing the actor transform.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtBoundingBoxManipulatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UUxtBoundingBoxManipulatorComponent();

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	const TArray<FUxtBoundingBoxAffordanceInfo>& GetCustomAffordances() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	TSubclassOf<class AActor> GetCenterAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	TSubclassOf<class AActor> GetFaceAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	TSubclassOf<class AActor> GetEdgeAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	TSubclassOf<class AActor> GetCornerAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	bool UseCustomAffordances() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	EUxtBoundingBoxManipulatorPreset GetPreset() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	bool GetInitBoundsFromActor() const;

	UFUNCTION(BlueprintGetter, Category = "Bounding Box")
	const FBox& GetBounds() const;

	/**
	 * Get the list of affordances that will be used for the bounding box.
	 * This can be a based on a preset or a custom set of affordances.
	 */
	UFUNCTION(BlueprintPure, Category = "Bounding Box")
	const TArray<FUxtBoundingBoxAffordanceInfo>& GetUsedAffordances() const;

	/** Actor class that will be instantiated for the given kind of affordance. */
	UFUNCTION(BlueprintPure, Category = "Bounding Box")
	TSubclassOf<class AActor> GetAffordanceKindActorClass(EUxtBoundingBoxAffordanceKind Kind) const;

	/** Compute the bounding box based on the components of the bounding box actor. */
	UFUNCTION(BlueprintCallable, Category = "Bounding Box")
	void ComputeBoundsFromComponents();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Callback when an affordance is being grabbed. */
	UFUNCTION()
	void OnPointerBeginGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being grabbed. */
	UFUNCTION()
	void OnPointerUpdateGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being released. */
	UFUNCTION()
	void OnPointerEndGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer);

	/**
	 * Try to activate the given grab pointer on the bounding box.
	 * Returns true when the grab activation was successful and the pointer will update the bounding box.
	 */
	bool TryActivateGrabPointer(const FUxtBoundingBoxAffordanceInfo &Affordance, const FUxtGrabPointerData &GrabPointer);
	/**
	 * Release the grab pointer.
	 * Returns true if the pointer was grabbing and has been released.
	 */
	bool TryReleaseGrabPointer(const FUxtBoundingBoxAffordanceInfo &Affordance);
	/**
	 * Look up the grab pointer data for an affordance.
	 * Returns null if the affordance is not currently grabbed.
	 */
	FUxtGrabPointerData* FindGrabPointer(const FUxtBoundingBoxAffordanceInfo& Affordance);

	/** Compute new bounding box and rotation based on the currently active grab pointers. */
	void ComputeModifiedBounds(const FUxtBoundingBoxAffordanceInfo &Affordance, const FUxtGrabPointerData &GrabPointer, FBox &OutBounds, FQuat &OutDeltaRotation) const;

	/** Update the world transforms of affordance actors to match the current bounding box. */
	void UpdateAffordanceTransforms();

	/**
	 * Compute the relative translation and scale between two boxes.
	 * Returns false if relative scale can not be computed.
	 */
	static bool GetRelativeBoxTransform(const FBox &Box, const FBox &RelativeTo, FTransform &OutTransform);

private:

	/** Actor class to instantiate for a center affordance. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetCenterAffordanceClass", Category = "Bounding Box")
	TSubclassOf<class AActor> CenterAffordanceClass;

	/** Actor class to instantiate for a face affordances. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetFaceAffordanceClass", Category = "Bounding Box")
	TSubclassOf<class AActor> FaceAffordanceClass;

	/** Actor class to instantiate for a edge affordances. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetEdgeAffordanceClass", Category = "Bounding Box")
	TSubclassOf<class AActor> EdgeAffordanceClass;

	/** Actor class to instantiate for a corner affordances. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetCornerAffordanceClass", Category = "Bounding Box")
	TSubclassOf<class AActor> CornerAffordanceClass;

	/** Use a custom set of affordances instead of a preset. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "UseCustomAffordances", Category = "Bounding Box")
	bool bUseCustomAffordances = false;

	/**
	 * Preset to use for the bounding box.
	 * When set to Custom the list of affordances must be created by the user.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetPreset", meta = (EditCondition = "!bUseCustomAffordances"), Category = "Bounding Box")
	EUxtBoundingBoxManipulatorPreset Preset = EUxtBoundingBoxManipulatorPreset::Default;

	/**
	 * List of custom affordances.
	 * These affordances will only be used when using the Custom preset.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetCustomAffordances", meta = (EditCondition = "bUseCustomAffordances"), Category = "Bounding Box")
	TArray<FUxtBoundingBoxAffordanceInfo> CustomAffordances;

	/** Initialize bounds from actor content. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetInitBoundsFromActor", Category = "Bounding Box")
	bool bInitBoundsFromActor = true;

	/** Current bounding box in the local space of the actor. */
	UPROPERTY(Transient, BlueprintGetter = "GetBounds", Category = "Bounding Box")
	FBox Bounds;

	/**
	 * Maps actors to the affordances they represent.
	 * This is used for looking up the correct affordance settings when grab events are handled.
	 */
	TMap<AActor*, const FUxtBoundingBoxAffordanceInfo*> ActorAffordanceMap;

	/**
	 * Contains the currently active affordances being moved by grab pointers.
	 * 
	 * Note:
	 * Currently should only ever contain a single grab pointer.
	 * In the future multiple simultaneous grabs may be supported.
	 */
	TArray<TPair<const FUxtBoundingBoxAffordanceInfo*, FUxtGrabPointerData>> ActiveAffordanceGrabPointers;

	/** Initial bounding box at the start of interaction. */
	FBox InitialBounds;
	/** Initial transform at the start of interaction. */
	FTransform InitialTransform;
};

