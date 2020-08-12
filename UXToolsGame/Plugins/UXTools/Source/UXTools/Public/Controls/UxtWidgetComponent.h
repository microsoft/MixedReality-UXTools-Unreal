// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtPokeTarget.h"

#include "UxtWidgetComponent.generated.h"

class UUxtPointerComponent;
class FSlateVirtualUserHandle;

/**
 * Widget Component that is interactable with near and far interaction.
 */
UCLASS( ClassGroup = UXTools, meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtWidgetComponent : public UWidgetComponent, public IUxtPokeTarget, public IUxtFarTarget
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
	virtual bool GetClosestPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const override;
	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface

	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:

	void PointerMove(const FVector& ClosestPoint, UUxtPointerComponent* Pointer);
	void PointerDown(const FVector& ClosestPoint, UUxtPointerComponent* Pointer);
	void PointerUp(const FVector& ClosestPoint, UUxtPointerComponent* Pointer);
	void GetEventAndPath(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, FKey Key, FPointerEvent& Event, FWidgetPath& Path);

public:
	
	/**
	 * Represents the Virtual User Index.  Each virtual user should be represented by a different 
	 * index number, this will maintain separate capture and focus states for them.  Each
	 * controller or finger-tip should get a unique PointerIndex.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=( ClampMin = "0", ExposeOnSpawn = true ))
	int32 VirtualUserIndex = 0;

private:

	TMap<UUxtPointerComponent*, FVector2D> Pointers;

	bool bIsPressed = false;
	TSharedPtr<FSlateVirtualUserHandle> VirtualUser;
	TSet<FKey> PressedKeys;
	FModifierKeysState ModifierKeys;
};
