// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/SceneComponent.h"

#include "UxtUIElementComponent.generated.h"

UENUM(BlueprintType)
enum class EUxtUIElementVisibility : uint8
{
	/** The element is shown */
	Show,
	/** The element is hidden */
	Hide,
	/** The element is hidden but should affect layout */
	LayoutOnly
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtUIElementShowDelegate, UUxtUIElementComponent*, UIElement);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtUIElementHideDelegate, UUxtUIElementComponent*, UIElement, bool, bShouldAffectLayout);

/**
 * Controls visibility of a UI element in the scene.
 *
 * Parent-child relationships are managed via actor attachments. If the parent is hidden, all of its children will be hidden.
 * It is recommended to have the UxtUIElementComponent as the root component as the actor as this allows it to automatically update
 * if the actor is attached to a new parent actor. If it is not the root component, RefreshUIElement() will need to be called manually
 * after attaching a new parent actor.
 *
 * Note: Manually changing actor visibility will not affect child UI elements and may lead to unwanted behavior.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtUIElementComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUxtUIElementComponent() = default;

	/** Get the element's visibility. This does not reflect if the element is visible in the scene. */
	UFUNCTION(BlueprintCallable, Category = "Uxt UI Element", DisplayName = "Get UI Visibility Self")
	EUxtUIElementVisibility GetUIVisibilitySelf() const;

	/** Get the element's visibility in the scene. */
	UFUNCTION(BlueprintCallable, Category = "Uxt UI Element", DisplayName = "Get UI Visibility in Hierarchy")
	EUxtUIElementVisibility GetUIVisibilityInHierarchy() const;

	/** Set the element's visibility. The element will not be visible in the scene if it's parent is hidden. */
	UFUNCTION(BlueprintCallable, Category = "Uxt UI Element", DisplayName = "Set UI Visibility")
	void SetUIVisibility(EUxtUIElementVisibility NewVisibility);

	/** Refresh the element's visibility. This is only necessary after changing the element's parent actor when this is not the root
	 * component of the actor. */
	UFUNCTION(BlueprintCallable, Category = "Uxt UI Element", DisplayName = "Refresh UI Element")
	void RefreshUIElement();

	/** Event raised when the element is shown. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt UI Element")
	FUxtUIElementShowDelegate OnShowElement;

	/** Event raised when the element is hidden. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt UI Element")
	FUxtUIElementHideDelegate OnHideElement;

protected:
	virtual void BeginPlay() override;

	virtual void OnAttachmentChanged() override;

private:
	/** Get if the parent is visible in the scene. */
	EUxtUIElementVisibility GetParentVisibility() const;

	/** Update the element and its children's visibility. */
	void UpdateVisibility(EUxtUIElementVisibility ParentVisibility = EUxtUIElementVisibility::Show);

	/** The element's visibility. */
	UPROPERTY(EditAnywhere, Category = "Uxt UI Element", DisplayName = "UI Visibility")
	EUxtUIElementVisibility Visibility = EUxtUIElementVisibility::Show;
};
