// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"

#include "UxtTooltipActor.generated.h"

class USplineMeshComponent;
class UWidgetComponent;
class UUxtBackPlateComponent;

/**
 * Tooltip Actor. This object represents a tooltip in the 3D world.
 *
 * The tooltip can point to an Actor/Component and it will draw a line between the two.
 * The tooltip will automatically parent itself to the target.
 *
 * The tooltip is designed for text but it allows the user to set any Blueprint Widget.
 * If you don't want to create a new Widget Blueprint, you can use the SetText function which will use a C++ slate widget.
 *
 * The Tooltip Actor's component hierarchy looks like this.
 * Tooltip
 *        |_SceneRoot
 *                   |_BackPlate
 *                   |_TooltipWidgetComponent
 *...................|_Anchor
 *                   |_SplineMeshComponent
 *
 * The SplineMeshComponent is used to draw the line between the tooltip and the target actor.
 *
 * The Anchor allows the user to provide an offset to the target to control the end of the spline.
 *
 * The tooltip with auto anchoring mode will try to bind the spline to the sides/corners of the widget.
 *
 * The tooltip is also "billboarded" to the camera.
 *
 * When no blueprint widget has been configured, the tooltip reverts to a slate widget printing some default text.
 *
 * There's a Margin property that can be used to add space between the text and the border of the back plate.
 */
UCLASS(
	ClassGroup = ("UXTools - Experimental"), meta = (BlueprintSpawnableComponent),
	HideCategories = (Object, LOD, Physics, Materials, StaticMesh, Default, Collision))
class UXTOOLS_API AUxtTooltipActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Function used to set the target actor/component at runtime.
	 * It is possible to only provide an actor without its component.
	 * This will result in pointing to the root component.
	 * The tooltip will automatically parent itself to the target actor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Tooltip - Experimental")
	void SetTarget(AActor* TargetActor, UActorComponent* TargetComponent);

	/** Function used to set the text without creating a new widget class.*/
	UFUNCTION(BlueprintCallable, Category = "Uxt Tooltip - Experimental")
	void SetText(const FText& Text);

	/** The widget rendered by this tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tooltip - Experimental")
	TSubclassOf<UUserWidget> WidgetClass = nullptr; // pointer to the widget class it will instantiate.

	/** An offset on the target position. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Uxt Tooltip - Experimental")
	USceneComponent* Anchor = nullptr;

protected:
	//
	// AActor interface

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	/** Used to tick in editor. */
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	//
	// UObject interface
#if WITH_EDITOR
	/** Editor update function - called by UE4*/
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	/** Editor update function - called by UE4 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** Updates the different properties of the class. */
	void UpdateComponent();

	/**
	 *  Update the start and the end of the spline so that it follows the tooltip and its target.
	 *  The Start and End point come from the SplineMeshComponent and are in local space to the component.
	 */
	void UpdateSpline();

	/** Update the the backplate to have a scale that matches the current widget and scale. */
	void UpdateBackPlate();

	/** Make sure that the tooltip is always facing the camera. */
	void UpdateBillboard();

	/** Make sure that the right widget is being rendered. */
	void UpdateWidget();

	/**
	 * It is assumed that a widget is square shaped, there's an anchor on each corner and on each sides of the square.
	 * This checks which anchor is closest to the end position (the target) and return the position of the closest.
	 */
	FVector GetClosestAnchorToTarget(FVector EndPosition) const;

	/** Actor root component */
	UPROPERTY(VisibleAnywhere, Category = "Uxt Tooltip - Experimental")
	USceneComponent* SceneRoot;

	/** The actual widget component that gets created.*/
	UPROPERTY(VisibleDefaultsOnly, Category = "Uxt Tooltip - Experimental", AdvancedDisplay)
	UWidgetComponent* TooltipWidgetComponent = nullptr;

	/** The back plate which frames the widget component.*/
	UPROPERTY(VisibleDefaultsOnly, Category = "Uxt Tooltip - Experimental", AdvancedDisplay)
	UUxtBackPlateComponent* BackPlate = nullptr;

	/** Whether the tooltip is always facing the camera. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip - Experimental", AdvancedDisplay)
	bool bIsBillboarding = true;

	/** Whether the tooltip uses edges/corners anchors instead of the center of the tooltip. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip - Experimental", AdvancedDisplay)
	bool bIsAutoAnchoring = true;

	UPROPERTY(VisibleDefaultsOnly, Category = "Uxt Tooltip - Experimental", AdvancedDisplay)
	class USplineMeshComponent* SplineMeshComponent;

	/** The target actor/component pointed at by this tooltip. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip - Experimental")
	FComponentReference TooltipTarget;

	/** Margin adds a small margin around the text. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip - Experimental")
	float Margin = 20.0f;

	/** Tightly coupled with those classes to keep the interface clean. */
	friend class UUxtTooltipSpawnerComponent;
	friend class TooltipSpec;
	friend class TooltipSpawnerSpec;
};
