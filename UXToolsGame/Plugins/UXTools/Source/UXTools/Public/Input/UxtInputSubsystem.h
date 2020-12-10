// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "UxtInputSubsystem.generated.h"

class UUxtFarHandler;
class UUxtFarPointerComponent;
class UUxtGrabHandler;
class UUxtNearPointerComponent;
class UUxtPokeHandler;

/** Subsystem for dispatching events to interested handlers. */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API UUxtInputSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Register the given handler as interested in events for a given handler interface. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static bool RegisterHandler(UObject* Handler, TSubclassOf<UInterface> Interface);

	/** Unregister the given handler as interested in events for a given handler interface. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static bool UnregisterHandler(UObject* Handler, TSubclassOf<UInterface> Interface);

	/** Raised when a far pointer starts focusing a primitive. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseEnterFarFocus(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is updated. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseUpdatedFarFocus(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer);

	/** Raised when a far pointer stops focusing a primitive. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseExitFarFocus(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is pressed. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseFarPressed(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is dragged. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseFarDragged(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is released. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseFarReleased(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer);

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseEnterGrabFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseUpdateGrabFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseExitGrabFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer starts grabbing while overlapping the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseBeginGrab(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while grabbing. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseUpdateGrab(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops grabbing or stops overlapping the actor while grabbing. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseEndGrab(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseEnterPokeFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseUpdatePokeFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseExitPokeFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer poke volume starts overlapping the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseBeginPoke(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised while a pointer poke volume is overlapping the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseUpdatePoke(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer poke volume stops overlapping the actor. */
	UFUNCTION(BlueprintCallable, Category = "UXTools|Input")
	static void RaiseEndPoke(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer);

private:
	/** Dispatch the given event to interested handlers. */
	template <typename HandlerType, typename FuncType>
	void RaiseEvent(UPrimitiveComponent* Target, const FuncType& Callback) const;

	/** Can the given handler handle events for the given primitive. */
	template <typename HandlerType>
	bool CanHandle(UObject* Handler, UPrimitiveComponent* Primitive) const;

	template <>
	bool CanHandle<UUxtFarHandler>(UObject* Handler, UPrimitiveComponent* Primitive) const;
	template <>
	bool CanHandle<UUxtGrabHandler>(UObject* Handler, UPrimitiveComponent* Primitive) const;
	template <>
	bool CanHandle<UUxtPokeHandler>(UObject* Handler, UPrimitiveComponent* Primitive) const;

	/** Dispatch the given event to interested handlers that share a parent Actor with Target. */
	template <typename HandlerType, typename FuncType>
	void ExecuteHierarchy(UPrimitiveComponent* Target, const FuncType& Callback, const TSet<UObject*>& Handled) const;

private:
	// Map contains array of listeners for each type of handler registered
	TMap<UClass*, TSet<UObject*>> Listeners;
};

template <typename HandlerType, typename FuncType>
void UUxtInputSubsystem::RaiseEvent(UPrimitiveComponent* Target, const FuncType& Callback) const
{
	// If a global listener is under the same actor as Target, dispatching an event to it
	// would duplicate the event, as it will also be dispatched here and in ExecuteHierarchy.
	// In these situations, in order to only dispatch once, we keep track of a set of handlers
	// that have already received this event.
	TSet<UObject*> Handled;

	for (UObject* Handler : Listeners.FindRef(HandlerType::StaticClass()))
	{
		if (CanHandle<HandlerType>(Handler, Target))
		{
			Callback(Handler);
			Handled.Add(Handler);
		}
	}

	ExecuteHierarchy<HandlerType>(Target, Callback, Handled);
}

template <typename HandlerType, typename FuncType>
void UUxtInputSubsystem::ExecuteHierarchy(UPrimitiveComponent* Target, const FuncType& Callback, const TSet<UObject*>& Handled) const
{
	if (Target)
	{
		for (UActorComponent* Child : Target->GetOwner()->GetComponents())
		{
			if (Child->Implements<HandlerType>() && CanHandle<HandlerType>(Child, Target) && !Handled.Contains(Child))
			{
				Callback(Child);
			}
		}
	}
}
