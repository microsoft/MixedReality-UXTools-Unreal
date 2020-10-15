// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtInputSubsystem.h"

#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtGrabHandler.h"
#include "Interactions/UxtPokeHandler.h"
#include "Templates/SubclassOf.h"

namespace
{
	UUxtInputSubsystem* GetInputSubsystem(UObject* WorldContextObject)
	{
		return WorldContextObject->GetWorld()->GetGameInstance()->GetSubsystem<UUxtInputSubsystem>();
	}
} // namespace

bool UUxtInputSubsystem::RegisterHandler(UObject* Handler, TSubclassOf<UInterface> Interface)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Handler);

	const UClass* Class = Handler->GetClass();
	if (Class && Class->ImplementsInterface(Interface))
	{
		InputSubsystem->Listeners.FindOrAdd(Interface).Add(Handler);
		return true;
	}

	return false;
}

bool UUxtInputSubsystem::UnregisterHandler(UObject* Handler, TSubclassOf<UInterface> Interface)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Handler);

	const UClass* Class = Handler->GetClass();
	if (Class && Class->ImplementsInterface(Interface))
	{
		if (InputSubsystem->Listeners.Contains(Interface))
		{
			InputSubsystem->Listeners.Find(Interface)->Remove(Handler);

			if (InputSubsystem->Listeners.Find(Interface)->Num() == 0)
			{
				InputSubsystem->Listeners.Remove(Interface);
			}

			return true;
		}
	}

	return false;
}

void UUxtInputSubsystem::RaiseEnterFarFocus(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtFarHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtFarHandler::Execute_OnEnterFarFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseUpdatedFarFocus(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtFarHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtFarHandler::Execute_OnUpdatedFarFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseExitFarFocus(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtFarHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtFarHandler::Execute_OnExitFarFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseFarPressed(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtFarHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtFarHandler::Execute_OnFarPressed(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseFarDragged(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtFarHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtFarHandler::Execute_OnFarDragged(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseFarReleased(UPrimitiveComponent* Target, UUxtFarPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtFarHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtFarHandler::Execute_OnFarReleased(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseEnterGrabFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtGrabHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtGrabHandler::Execute_OnEnterGrabFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseUpdateGrabFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtGrabHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtGrabHandler::Execute_OnUpdateGrabFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseExitGrabFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtGrabHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtGrabHandler::Execute_OnExitGrabFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseBeginGrab(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtGrabHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtGrabHandler::Execute_OnBeginGrab(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseUpdateGrab(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtGrabHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtGrabHandler::Execute_OnUpdateGrab(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseEndGrab(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtGrabHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtGrabHandler::Execute_OnEndGrab(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseEnterPokeFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtPokeHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtPokeHandler::Execute_OnEnterPokeFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseUpdatePokeFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtPokeHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtPokeHandler::Execute_OnUpdatePokeFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseExitPokeFocus(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtPokeHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtPokeHandler::Execute_OnExitPokeFocus(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseBeginPoke(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtPokeHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtPokeHandler::Execute_OnBeginPoke(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseUpdatePoke(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtPokeHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtPokeHandler::Execute_OnUpdatePoke(Handler, Pointer); });
}

void UUxtInputSubsystem::RaiseEndPoke(UPrimitiveComponent* Target, UUxtNearPointerComponent* Pointer)
{
	UUxtInputSubsystem* InputSubsystem = GetInputSubsystem(Pointer);
	InputSubsystem->RaiseEvent<UUxtPokeHandler>(
		Target, [&Pointer](UObject* Handler) { IUxtPokeHandler::Execute_OnEndPoke(Handler, Pointer); });
}

template <>
bool UUxtInputSubsystem::CanHandle<UUxtFarHandler>(UObject* Handler, UPrimitiveComponent* Primitive) const
{
	return IUxtFarHandler::Execute_CanHandleFar(Handler, Primitive);
}

template <>
bool UUxtInputSubsystem::CanHandle<UUxtGrabHandler>(UObject* Handler, UPrimitiveComponent* Primitive) const
{
	return IUxtGrabHandler::Execute_CanHandleGrab(Handler, Primitive);
}

template <>
bool UUxtInputSubsystem::CanHandle<UUxtPokeHandler>(UObject* Handler, UPrimitiveComponent* Primitive) const
{
	return IUxtPokeHandler::Execute_CanHandlePoke(Handler, Primitive);
}
