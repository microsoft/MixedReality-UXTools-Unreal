// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "UxtUIElement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtUIElementActivatedDelegate, UUxtUIElement*, UIElement);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtUIElementDeactivatedDelegate, UUxtUIElement*, UIElement);

/**
 * Controls presence of a UI element in the scene.
 *
 * Parent-child relationships are managed via actor attachments. If the parent is not active, all of its children will be inactive.
 * If the element is re-attached to a new parent actor, RefreshElement() will need to be called to update the element to match its new parent's state.
 *
 * Note: Manually changing actor visibility will not affect child UI elements and may lead to unwanted behavior.
 */
UCLASS(ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtUIElement : public USceneComponent
{
	GENERATED_BODY()

public:

	UUxtUIElement() = default;

	/** Get if this element is active. This does not reflect if the element is active in the scene. */
	UFUNCTION(BlueprintCallable)
	bool IsElementActiveSelf() const;

	/** Get if the element is active in the scene. */
	UFUNCTION(BlueprintCallable)
	bool IsElementActiveInHierarchy() const;

	/** Set element's active state. The element will not be active in the scene if it's parent is inactive. */
	UFUNCTION(BlueprintCallable)
	void SetElementActive(bool bNewActive);

	/** Refresh the element's state. This is only necessary after changing the element's parent actor. */
	UFUNCTION(BlueprintCallable)
	void RefreshElement();

	/** Event raised when the UI element is activated. */
	UPROPERTY(BlueprintAssignable, Category = "UIElement")
	FUxtUIElementActivatedDelegate OnElementActivated;

	/** Event raised when the UI element is deactivated. */
	UPROPERTY(BlueprintAssignable, Category = "UIElement")
	FUxtUIElementDeactivatedDelegate OnElementDeactivated;

protected:

	virtual void BeginPlay() override;

private:

	/** Get if the parent is active in the scene. */
	bool IsParentActive() const;

	/** Update the element and its children's visibility. */
	void UpdateVisibility(bool bParentIsActive = true);

	/** If false, the element and its children will be hidden from the scene. */
	UPROPERTY(EditAnywhere, Category = "UIElement")
	bool bIsElementActive = true;
};
