// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UxtHandInteractionActor.generated.h"

class UUxtNearPointerComponent;
class UUxtFarPointerComponent;


/**
 * Actor that drives hand interactions with components that implement the far, grab and poke target interfaces.
 * A hand has two interaction modes:
 * - Near: interactions performed by poking or grabbing targets directly.
 * - Far: interactions performed by pointing at far targets from a distance via a hand ray.
 * The actor transitions between modes depending on whether there is a grab or poke target within the near activation distance.
 */
UCLASS(ClassGroup = UXTools)
class UXTOOLS_API AUxtHandInteractionActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AUxtHandInteractionActor(const FObjectInitializer& ObjectInitializer);

	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintGetter)
	EControllerHand GetHand() const { return Hand; }
	UFUNCTION(BlueprintSetter)
	void SetHand(EControllerHand NewHand);

	UFUNCTION(BlueprintGetter)
	ECollisionChannel GetTraceChannel() const { return TraceChannel; }
	UFUNCTION(BlueprintSetter)
	void SetTraceChannel(ECollisionChannel NewTraceChannel);

	UFUNCTION(BlueprintGetter)
	float GetPokeRadius() const { return PokeRadius; }
	UFUNCTION(BlueprintSetter)
	void SetPokeRadius(float NewPokeRadius);

	UFUNCTION(BlueprintGetter)
	float GetRayStartOffset() const { return RayStartOffset; }
	UFUNCTION(BlueprintSetter)
	void SetRayStartOffset(float NewRayStartOffset);

	UFUNCTION(BlueprintGetter)
	float GetRayLength() const { return RayLength; }
	UFUNCTION(BlueprintSetter)
	void SetRayLength(float NewRayLength);

	/** Distance from the hand to the closest grab or poke target at which near interaction activates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Interaction", meta = (DisplayAfter = "PokeRadius"))
	float NearActivationDistance = 20.0f;

	/** Create default visuals for the near cursor. Changes to this value after BeginPlay have no effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Hand Interaction")
	bool bUseDefaultNearCursor = true;

	/** Create default visuals for the far cursor. Changes to this value after BeginPlay have no effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Hand Interaction")
	bool bUseDefaultFarCursor = true;

	/** Create default visuals for the far beam. Changes to this value after BeginPlay have no effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Hand Interaction")
	bool bUseDefaultFarBeam = true;

	/** Show the near cursor on grab targets. Changes to this value after BeginPlay have no effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Hand Interaction", meta = (ExposeOnSpawn = true))
	bool bShowNearCursorOnGrabTargets = false;

private:

	/** Determine if the hand pose is valid for making selections. */
	bool IsInPointingPose() const;

private:

	/** Articulated hand used to drive interactions. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetHand", BlueprintSetter = "SetHand", Category = "Hand Interaction", meta = (ExposeOnSpawn = true))
	EControllerHand Hand;

	/** Offset from the hand ray origin at which the far ray used for far target selection starts. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetRayStartOffset", BlueprintSetter = "SetRayStartOffset", Category = "Hand Interaction")
	float RayStartOffset = 5.0f;

	/** Length of the far ray */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetRayLength", BlueprintSetter = "SetRayLength", Category = "Hand Interaction")
	float RayLength = 500.0f;

	/** Maximum distance around the finger tip at which we look for poke targets. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetPokeRadius", BlueprintSetter = "SetPokeRadius", Category = "Hand Interaction")
	float PokeRadius = 20.0f;

	/** Trace channel used for targeting queries. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetTraceChannel", BlueprintSetter = "SetTraceChannel", Category = "Hand Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(Transient)
	UUxtNearPointerComponent* NearPointer;

	UPROPERTY(Transient)
	UUxtFarPointerComponent* FarPointer;

	bool bHadTracking = false;
	FVector PrevQueryPosition;
};
