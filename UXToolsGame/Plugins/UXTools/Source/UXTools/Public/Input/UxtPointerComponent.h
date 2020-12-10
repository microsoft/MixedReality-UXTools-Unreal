// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "Components/ActorComponent.h"

#include "UxtPointerComponent.generated.h"

/**
 * Base component for UXT pointers.
 */
UCLASS(Abstract, ClassGroup = "UXTools")
class UXTOOLS_API UUxtPointerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUxtPointerComponent() = default;

	/** Get the lock state of the pointer. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pointer")
	bool GetFocusLocked() const;

	/**
	 * Set the lock state of the pointer.
	 * Locked pointers don't update their hit, remaining focused on the primitive they were hitting until unlocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pointer")
	virtual void SetFocusLocked(bool bLocked);

	/** Get the target currently being focused by the pointer, if any. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pointer")
	virtual UObject* GetFocusTarget() const PURE_VIRTUAL(UUxtPointerComponent::GetFocusTarget, return nullptr;);

	/** Get the cursor transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pointer")
	virtual FTransform GetCursorTransform() const PURE_VIRTUAL(UUxtPointerComponent::GetCursorTransform, return FTransform::Identity;);

public:
	/** The hand to be used for targeting. TODO: replace with generic input device. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Pointer")
	EControllerHand Hand = EControllerHand::AnyHand;

protected:
	/** The lock state of the pointer. */
	bool bFocusLocked = false;
};
