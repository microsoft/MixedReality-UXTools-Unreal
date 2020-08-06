### UxtGrabTargetComponent - Grab point function changes

* `UUxtGrabTargetComponent::GetGrabPointCentroid(...)` has been updated to return an `FTransform` instead of just a location vector.
* `UUxtGrabTargetComponent::GetGrabPointCentroidTransform()` has been removed. Instead `GetGrabPointCentroid(...)` can be used by passing the target component's world transform.

### UxtHandInteractionActor - Proximity detection changes
* `UxtHandInteractionActor::NearActivationDistance` has been removed. The proximity detection is defined by the `ProximityCone` parameters now.

### BP_ButtonHoloLens2Toggle state and event dispatchers have moved to UUxtToggleStateComponent (TBD)

In `BP_ButtonHoloLens2Toggle` the blueprint toggle state variables and events dispatchers are now accessible via the attached `UUxtToggleStateComponent`. Any blueprint references to the original blueprint variables or events must now be rewired to reference the variables and events within `UUxtToggleStateComponent`. The event graph within `BP_ButtonHoloLens2Toggle` can be used as an example of the required updates.
