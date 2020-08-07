# UX Tools 0.10.0 release notes

- [What's new](#whats-new)
- [Breaking changes](#breaking-changes)
- [Known issues](#known-issues)

This release of the UX Tools supports only HoloLens 2. Support for other MR platforms remains a goal for us but is not the current focus.

Unreal 4.25 required.

## What's new

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

## Known issues