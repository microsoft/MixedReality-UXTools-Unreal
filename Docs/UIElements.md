# UI Elements

UI elements give the user a way to show and hide groups of UI functionality. UI elements are scene components that can be attached to any actor to allow its visibility to be controlled by its parent element.

Parent-child relationships are managed via actor attachments. If the parent is hidden, all of its children will be hidden. It is recommended to have the `UxtUIElementComponent` as the root component as the actor as this allows it to automatically update if the actor is attached to a new parent actor. If it is not the root component, RefreshUIElement() will need to be called manually after attaching a new parent actor.

**Note**: Manually changing actor visibility will not affect child UI elements and may lead to unwanted behavior.

## Built-in UI Elements

A number of the built in components are UI elements by default, but any actor can be given UI element functionality by attaching a `UxtUIElementComponent`.
* `UxtPressableButtonComponent`
* `UxtPinchSliderComponent`
* `UxtTouchableVolumeComponent`
