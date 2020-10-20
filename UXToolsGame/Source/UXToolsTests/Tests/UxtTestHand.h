// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "InputCoreTypes.h"

#include "Interactions/UxtInteractionMode.h"
#include "Math/TransformNonVectorized.h"

class UUxtPointerComponent;

/**
 * Provides an easier interface for managing pointers for manipulation tests.
 * UxtTestHandTracker must be enabled before use.
 */
class FUxtTestHand
{
public:
	FUxtTestHand(EControllerHand TargetHand);

	/** Configure the hand for a test by spawning the appropriate pointer aiming at the TargetLocation. */
	void Configure(EUxtInteractionMode TargetInteractionMode, const FVector& TargetLocation);

	/** Reset the hand after a test. */
	void Reset();

	/** Get the pointer attached to the hand. */
	UUxtPointerComponent* GetPointer() const;

	/** Translate the hand by the given vector. */
	void Translate(const FVector& Translation);

	/** Rotate the hand by the given quaternion. */
	void Rotate(const FQuat& Rotation);

	/** Set the hand to the given translation. */
	void SetTranslation(const FVector& Translation);

	/** Set the hand to the given rotation. */
	void SetRotation(const FQuat& Rotation);

	/** Set the grab state of the hand. */
	void SetGrabbing(bool bIsGrabbing);

	/** Get the hand transform */
	FTransform GetTransform() const;

private:
	/** The hand. */
	EControllerHand Hand = EControllerHand::AnyHand;

	/** The interaction mode. Only Near and Far modes are supported. */
	EUxtInteractionMode InteractionMode = EUxtInteractionMode::None;

	/** The hand's transform. */
	FTransform Transform = FTransform::Identity;

	/** The active pointer attached to the hand. */
	UUxtPointerComponent* Pointer = nullptr;
};
