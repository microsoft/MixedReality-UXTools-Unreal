// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"

#include "FarTargetTestComponent.generated.h"

/** Component used to count the far interaction events raised on an actor. */
UCLASS(ClassGroup = "UXToolsTests")
class UXTOOLSTESTS_API UFarTargetTestComponent
	: public UActorComponent
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	//
	// IUxtFarTarget interface

	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override { return true; }

	//
	// IUxtFarHandler interface

	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override { return true; }

	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override { NumEnter++; }

	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override { NumUpdated++; }

	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override { NumExit++; }

	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override { NumPressed++; }

	virtual void OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer) override { NumDragged++; }

	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override { NumReleased++; }

public:
	int NumEnter = 0;
	int NumUpdated = 0;
	int NumExit = 0;
	int NumPressed = 0;
	int NumDragged = 0;
	int NumReleased = 0;
};
