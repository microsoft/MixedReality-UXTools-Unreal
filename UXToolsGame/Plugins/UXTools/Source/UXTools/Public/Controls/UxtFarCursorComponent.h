// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtRingCursorComponent.h"

#include "UxtFarCursorComponent.generated.h"

class UUxtFarPointerComponent;

/**
 * When added to an actor with a far pointer this component displays a flat ring cursor at the pointer's hit point oriented
 * following the hit normal.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFarCursorComponent : public UUxtRingCursorComponent
{
	GENERATED_BODY()

public:
	UUxtFarCursorComponent();

	/** Distance over the hit surface to place the cursor at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Far Cursor")
	float HoverDistance = 0.5f;

	/**
	 * Cursor radius when idle at 1m from the camera.
	 * The actual radius will scale with the distance to the camera to keep a constant screen size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Far Cursor")
	float IdleRadius = 0.6f;

	/**
	 * Cursor radius when pressed at 1m from the camera.
	 * The actual radius will scale with the distance to the camera to keep a constant screen size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Far Cursor")
	float PressedRadius = 0.42f;

protected:
	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Category = "Uxt Far Cursor")
	void OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer);

	UFUNCTION(Category = "Uxt Far Cursor")
	void OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer);

	void SetPressed(bool bNewPressed);

	/** Far pointer in use. */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	bool bPressed = false;

	/** Cached ring thicknes for the idle state. Captured when transitioning to pressed. */
	float IdleRingThickness;
};
