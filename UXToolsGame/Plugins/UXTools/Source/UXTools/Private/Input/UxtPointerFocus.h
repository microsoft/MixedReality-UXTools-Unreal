// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UActorComponent;
class UPrimitiveComponent;
class UUxtNearPointerComponent;

/** Result of closest point search functions. */
struct FUxtPointerFocusSearchResult
{
	bool IsValid() const;

	/** Closest object that implements the requires pointer target interface. */
	UObject* Target;

	/** Primitive that contains the closes point. */
	UPrimitiveComponent* Primitive;

	/** Closest point on the primitive to the pointer position. */
	FVector ClosestPointOnTarget;

	/** Distance of the closest point to the pointer position. */
	float MinDistance;
};

/** Utility class that is used by components to manage different pointers and their focus targets. */
struct FUxtPointerFocus
{
public:

	virtual ~FUxtPointerFocus() {}

	/** Get the closest point on the surface of the focused target */
	const FVector& GetClosestTargetPoint() const;

	/** Get the currently focused target object. */
	UObject* GetFocusedTarget() const;

	/** Get the currently focused primitive component */
	UPrimitiveComponent* GetFocusedPrimitive() const;

	/** Get the currently focused target object.
	 *  Returns null if the target does not implement the expected interface.
	 */
	UObject* GetFocusedTargetChecked() const;

	// TODO get hand joints from WMR => no need to pass PointerTransform

	/** Select and set the focused target among the list of overlaps. */
	void SelectClosestTarget(UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, const TArray<FOverlapResult>& Overlaps);

	/** Update the ClosestTargetPoint while focus is locked */
	void UpdateClosestTarget(const FTransform& PointerTransform);

	/** Select the closest primitive from the owner of the given target component.
	 *  The target component will be the new focus, unless no usable primitive can be found.
	 */
	void SelectClosestPointOnTarget(UUxtNearPointerComponent* Pointer, const FTransform& PointerTransform, UActorComponent* NewTarget);

	/** Clear the focused target. */
	void ClearFocus(UUxtNearPointerComponent* Pointer);

	/** Notify the focused target of a pointer update. */
	void UpdateFocus(UUxtNearPointerComponent* Pointer) const;

	/** Find a component of the actor that implements the required interface. */
	UActorComponent* FindInterfaceComponent(AActor* Owner) const;

protected:

	/** Set the focus to the given target object, primitive, and point on the target. */
	void SetFocus(
		UUxtNearPointerComponent* Pointer,
		const FTransform& PointerTransform,
		UObject* NewTarget,
		UPrimitiveComponent* NewPrimitive,
		const FVector& NewClosestPointOnTarget);

	/** Find the closest target object, primitive, and point among the overlaps. */
	FUxtPointerFocusSearchResult FindClosestTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point) const;

	/** Find the closest primitive and point on the owner of the given component. */
	FUxtPointerFocusSearchResult FindClosestPointOnComponent(UActorComponent* Target, const FVector& Point) const;

	/** Get the interface class that targets for the pointer must implement. */
	virtual UClass* GetInterfaceClass() const = 0;

	/** Returns true if the given object implements the required target interface. */
	virtual bool ImplementsTargetInterface(UObject* Target) const = 0;

	/** Find the closest point on the given primitive using the distance function of the target interface. */
	virtual bool GetClosestPointOnTarget(const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint) const = 0;

	/** Notify the target object that it has entered focus. */
	virtual void RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const = 0;
	/** Notify the focused target object that the pointer has been updated. */
	virtual void RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const = 0;
	/** Notify the target object that it has exited focus. */
	virtual void RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const = 0;

private:

	/** Weak reference to the currently focused target. */
	TWeakObjectPtr<UObject> FocusedTargetWeak;

	/** Weak reference to the focused grab target primitive. */
	TWeakObjectPtr<UPrimitiveComponent> FocusedPrimitiveWeak;

	/** Closest point on the surface of the focused target. */
	FVector ClosestTargetPoint = FVector::ZeroVector;
};


/** Focus implementation for the grab pointers. */
struct FUxtGrabPointerFocus : public FUxtPointerFocus
{
public:

	/** Notify the target object that grab has started. */
	void BeginGrab(UUxtNearPointerComponent* Pointer);
	/** Notify the grabbed target object that the pointer has been updated. */
	void UpdateGrab(UUxtNearPointerComponent* Pointer);
	/** Notify the target object that grab has ended. */
	void EndGrab(UUxtNearPointerComponent* Pointer);

	bool IsGrabbing() const;

protected:

	virtual UClass* GetInterfaceClass() const override;

	virtual bool ImplementsTargetInterface(UObject* Target) const override;

	virtual bool GetClosestPointOnTarget(const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint) const override;

	virtual void RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const override;
	virtual void RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const override;
	virtual void RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const override;

private:

	bool bIsGrabbing = false;
};


/** Focus implementation for the poke pointers. */
struct FUxtPokePointerFocus : public FUxtPointerFocus
{
public:

	/** Notify the target object that poke has started. */
	void BeginPoke(UUxtNearPointerComponent* Pointer);
	/** Notify the poked target object that the pointer has been updated. */
	void UpdatePoke(UUxtNearPointerComponent* Pointer);
	/** Notify the target object that poke has ended. */
	void EndPoke(UUxtNearPointerComponent* Pointer);

	bool IsPoking() const;

protected:

	virtual UClass* GetInterfaceClass() const override;

	virtual bool ImplementsTargetInterface(UObject* Target) const override;

	virtual bool GetClosestPointOnTarget(const UActorComponent* Target, const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint) const override;

	virtual void RaiseEnterFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const override;
	virtual void RaiseUpdateFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const override;
	virtual void RaiseExitFocusEvent(UObject* Target, UUxtNearPointerComponent* Pointer) const override;

private:

	bool bIsPoking = false;
};
