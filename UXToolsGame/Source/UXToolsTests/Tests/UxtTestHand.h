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

	/**
	 * Set the hand to the given translation.
	 *
	 * If @ref bApplyOffset is true, the appropriate offset (@ref NearLocationOffset or @ref FarLocationOffset) will be added in order to
	 * place the hand at the appropriate distance.
	 */
	void SetTranslation(const FVector& Translation, bool bApplyOffset = false);

	/** Set the hand to the given rotation. */
	void SetRotation(const FQuat& Rotation);

	/** Set the grab state of the hand. */
	void SetGrabbing(bool bIsGrabbing);

	/** Get the hand transform */
	FTransform GetTransform() const;

	/**
	 * Sets the offset that will be applied to locations when requested.
	 *
	 * When Mode == None (default value), the modified offset is the one currently in use (depending on @ref InteractionMode).
	 */
	void SetLocationOffset(const FVector& NewOffset, EUxtInteractionMode Mode = EUxtInteractionMode::None);

private:
	/** The hand. */
	EControllerHand Hand = EControllerHand::AnyHand;

	/** The interaction mode. Only Near and Far modes are supported. */
	EUxtInteractionMode InteractionMode = EUxtInteractionMode::None;

	/** The hand's transform. */
	FTransform Transform = FTransform::Identity;

	/** The active pointer attached to the hand. */
	UUxtPointerComponent* Pointer = nullptr;

	FVector NearLocationOffset = FVector(-5, 0, 0);
	FVector FarLocationOffset = FVector(-200, 0, 0);
};
