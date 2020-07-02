# UI Elements

UI elements give the user a way to show and hide groups of UI functionality. UI elements are scene components that can be attached to any actor to allow it's presence to be controlled by it's parent element.

The parent-child relationship is controlled using actor attachment. When the parent actor is inactive it will hide all of its children. When the parent actor is active it will restore its children's state. There is an example of this using buttons in the ButtonExample scene.

**Notes**
1. Manually changing actor visibility will not affect child UI elements and may lead to unwanted behavior.
2. If the element is re-attached to a new parent actor, `RefreshElement()` will need to be called to update the element to match its new parent's state.

## Built-in UI Elements

A number of the built in components are UI elements by default. This includes:
* `UxtPressableButtonComponent`
* `UxtPinchSliderComponent`
* `UxtTouchableVolumeComponent`

There is a BP_UIPanel blueprint for HL2-style panels but any actor can be given UI element functionality by attaching a `UxtUIElement` component.
