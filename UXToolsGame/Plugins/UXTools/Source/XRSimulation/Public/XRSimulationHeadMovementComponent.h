// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/MovementComponent.h"

#include "XRSimulationHeadMovementComponent.generated.h"

/** Movement component that applies user input.
 *  Works similar to UFloatingPawnMovement, but does not require a APawn actor.
 */
UCLASS(ClassGroup = "XRSimulation")
class XRSIMULATION_API UXRSimulationHeadMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Add cumulative rotation input relative to current world space orientation. */
	void AddRotationInput(const FRotator& Rotation);
	/** Add cumulative movement input in world space. */
	void AddMovementInput(const FVector& Movement);

	UFUNCTION(BlueprintGetter, Category = "XRSimulation")
	bool IsHeadMovementEnabled() const;
	UFUNCTION(BlueprintSetter, Category = "XRSimulation")
	void SetHeadMovementEnabled(bool bEnable);

private:
	/** Apply and reset accumulated rotation input. */
	void ApplyRotationInput(float DeltaTime);
	/** Apply and reset accumulated movement input. */
	void ApplyMovementInput(float DeltaTime);

private:
	/** Current view orientation */
	FRotator ViewOrientation;
	/** Current view position */
	FVector ViewPosition;

	/** Input axes values, accumulated each tick. */
	FRotator RotationInput;
	/** Input axes values, accumulated each tick. */
	FVector MovementInput;

	/** Enable linear movement of the head position. */
	UPROPERTY(EditAnywhere, Category = "XRSimulation", BlueprintGetter = "IsHeadMovementEnabled", BlueprintSetter = "SetHeadMovementEnabled")
	bool bEnableHeadMovement = true;
};
