# UX Tools 0.10.0 release notes

- [What's new](#whats-new)
- [Breaking changes](#breaking-changes)
- [Known issues](#known-issues)

This release of the UX Tools supports only HoloLens 2. Support for other MR platforms remains a goal for us but is not the current focus.

Unreal 4.25 required.

## What's new

### UI elements

The new [`UxtUIElementComponent`](UIElements.md) allows an actor's visibility to be controlled by its parent actor, allowing for easy construction of hierarchical UIs. The hierarchies can be constructed using actor attachment or child actor components.

`UxtPressableButtonComponent`, `UxtPinchSliderComponent` and `UxtTouchableVolumeComponent` now inherit from `UxtUIElementComponent`.

![HandMenuExample](Images/UIElement/UIElement.gif)

### Hand menus

[Hand menus](HandMenu.md) are a convenient hand-attached UI that are great for frequently used functions. An example level has been added to demonstrate various types of hand menus and to act as a guide for building custom menus.

![HandMenuExample](Images/HandMenu/HandMenu.gif)

### Surface Magnetism (experimental)

This new component (far interaction only) allows an actor to stick to a surface.

It works both with other actors in the scene

![SurfaceMagnetism](Images/ReleaseNotes/surface_magnetism.gif)

as well as a spatial mesh

![SurfaceMagnetismSpatialMesh](Images/ReleaseNotes/surface_magnetism_spatial_mesh.gif)

## Breaking changes

### Make poke target return normal for cursor transform

Changes to `IUxtPokeTarget` that may need to be fixed for any class that implements this interface:
- `IsPokeFocusable` is now `const`.
- New method `GetClosestPoint` which returns the closest point on the given primitive and the surface normal for the pokable.

This PR also contains changes to `UUxtNearPointerComponent::GetFocusedGrabTarget` and `UUxtNearPointerComponent::GetFocusedPokeTarget`. These methods now takes another `FVector` reference in order to return the surface normal.

### UxtGrabTargetComponent - Grab point function changes

* `UUxtGrabTargetComponent::GetGrabPointCentroid(...)` has been updated to return an `FTransform` instead of just a location vector.
* `UUxtGrabTargetComponent::GetGrabPointCentroidTransform()` has been removed. Instead `GetGrabPointCentroid(...)` can be used by passing the target component's world transform.

### UxtHandInteractionActor - Proximity detection changes

* `UxtHandInteractionActor::NearActivationDistance` has been removed. The proximity detection is defined by the `ProximityCone` parameters now.

### BP_ButtonHoloLens2Toggle state and event dispatchers have moved to UUxtToggleStateComponent

In `BP_ButtonHoloLens2Toggle` the blueprint toggle state variables and events dispatchers are now accessible via the attached `UUxtToggleStateComponent`. Any blueprint references to the original blueprint variables or events must now be rewired to reference the variables and events within `UUxtToggleStateComponent`. The event graph within `BP_ButtonHoloLens2Toggle` can be used as an example of the required updates.

### UxtPinchSliderComponent - New interface

The `UxtPinchSliderComponent` has been simplified and streamlined. It now operates on the same principles as the `UxtPressableButtonComponent`.

Interface changes:
* `GetCurrentState` has been renamed to `GetState`.
* `GetThumbVisuals` has been renamed to `GetVisuals`.
* `GetSliderValue`/`SetSliderValue` have been renamed to `GetValue`/`SetValue`.
* `GetSliderLowerBound`/`SetSliderLowerBound` have been renamed to `GetValueLowerBound`/`SetValueLowerBound`.
* `GetSliderUpperBound`/`SetSliderUpperBound` have been renamed to `GetValueUpperBound`/`SetValueUpperBound`.
* `IsGrabbed`, `IsFocused` and `IsEnabled` have been removed in favor of using `GetState`.
* Any references to the track and tick mark visuals have been removed as they are now managed by `AUxtPinchSliderActor`.
* `SliderStartDistance` and `SliderEndDistance` have been replaced by a single `TrackLength` property that dictates the length of the track.

Event changes:
* `OnBeginInteraction` has been renamed to `OnBeginGrab`.
* `OnEndInteraction` has been renamed to `OnEndGrab`.
* `OnSliderEnabled` has been renamed to `OnEnable`.
* `OnSliderDisabled` has been renamed to `OnDisable`.

### BP_SimpleSlider - Moved to a native implementation and renamed to UxtPinchSliderActor

`BP_SimpleSlider` has been replaced by `AUxtPinchSliderActor`. It has the same feature set as `BP_SimpleSlider` and can be extended from either Blueprints or C++.

## Known issues

### Surface Magnetism's actor jumps to zero

This happens if the hand ray isn't hitting anything to stick to when grabbing starts for the first time. It will work properly afterwards.
