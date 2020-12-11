// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "Components/ActorComponent.h"
#include "GenericPlatform/GenericApplication.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"

#include "UxtWidgetComponent.generated.h"

class UUxtPointerComponent;
class FSlateVirtualUserHandle;
class UWidgetComponent;
class FWidgetPath;
struct FPointerEvent;

/**
 * Widget Component that is interactable with near and far interaction.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtWidgetComponent
	: public UActorComponent
	, public IUxtPokeTarget
	, public IUxtPokeHandler
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

protected:
	//
	// UActorComponent interface
	virtual void BeginPlay() override;

	//
	// IUxtPokeTarget interface
	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;
	virtual EUxtPokeBehaviour GetPokeBehaviour_Implementation() const override;
	virtual bool GetClosestPoint_Implementation(
		const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const override;

	//
	// IUxtPokeHandler interface
	virtual bool CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:
	void PointerMove(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget);
	void PointerDown(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget);
	void PointerUp(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget);
	void GetEventAndPath(
		const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget, FKey Key, FPointerEvent& Event,
		FWidgetPath& Path);

public:
	/**
	 * Represents the Virtual User Index.  Each virtual user should be represented by a different
	 * index number, this will maintain separate capture and focus states for them.  Each
	 * controller or finger-tip should get a unique PointerIndex.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Widget", meta = (ClampMin = "0", ExposeOnSpawn = true))
	int32 VirtualUserIndex = 0;

private:
	TMap<UUxtPointerComponent*, FVector2D> Pointers;

	bool bIsPressed = false;
	TSharedPtr<FSlateVirtualUserHandle> VirtualUser;
	TSet<FKey> PressedKeys;
	FModifierKeysState ModifierKeys;
};
