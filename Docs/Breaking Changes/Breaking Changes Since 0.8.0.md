### PushBehaviour and MaxPushDistance made private in pressable button component (e343d0c)

`UUxtPressableButtonComponent::PushBehavior` and `UUxtPressableButtonComponent::MaxPushDistance` have been moved to private to enforce `MaxPushDistance` constraints with compressible buttons (MaxPushDistance is auto calculated for buttons with a compress push behavior). Please use the associated getters/setters from now on.

### Add remaining states to button (dbd56b1)

In `UUxtPressableButton`: `IsFocused`, `IsPressed` and `IsDisabled` have been have been removed in favor of `GetState`. These don't map 1:1, before it was possible for `IsFocused` and `IsPressed` to be true simultaneously. `GetState` will only give you the primary state of the button (e.g. Pressed). You can track the sub-states of the button using the begin/end events triggered by the button or combinations of overall states.

### Add option to switch between local and world space for button distances (2c63bcf)
MaxPushDistance is now in button local space. There are two ways of fixing this if you wish the button planes to remain in the same positions in space as they were before updating uxt:
  1. If you want button distances in world space, record the value of _Max Push Distance_, switch the button property _Use Absolute Push Distance_ to true and set _Max Push Distance_ to the old value.
  2. If you want button distances in local space, complete step 1. and then switch the button property _Use Absolute Push Distance_ back to false. The _Max Push Distance_ will update automatically.

### Fix buttons so that they face positive x (fb1e72e)
Buttons now depress towards negative x. This means that buttons will need to be rotated in order to function properly.
  1. Open your button blueprint, if you select the pressable button component you'll notice that the button planes are now aligned with the other side of the button.
  2. Transform your movable button visuals so that the new front face is the same distance from the blueprint origin as it was before. Here's a rough visualization of what this might mean:

     | Before Update | After Update | After Transform |
     | :-: | :-: | :-: |
     | • ¦ \| | • \| ¦ | \| ¦ • |

     | → Front Plane, ¦ → Back Plane, • → Blueprint Origin
  3. Translate or rotate any other parts of your button blueprint so that it works with the new button orientation and location (e.g. rotate text to face new front plane, move baseplate so that it aligns with the new back of the button).
  4. Wherever this blueprint is used (maps, other blueprints etc.) you will need to rotate the actor 180° around the z-axis.

### Rename the UxtBoundingBoxManipulatorComponent and related classes to BoundsControl (da40dc7)

Existing assets referencing those types will have to be re-saved as the redirects for the names changes will be eventually removed. Code references to the renamed types will have to be updated.

### Shader compilation errors when ray tracing is supported (9ec5a363)
If you are running UE 4.25.0 and your computer's GPU supports ray tracing you must disable UE 4.25 ray tracing support for UXT's custom shaders to compile. To disable ray tracing navigate to C:\Program Files\Epic Games\UE_4.25\Engine\Config\Windows (or where your installation is located), open `DataDrivenPlatformInfo.ini` and change `bSupportsRayTracing=true` to `bSupportsRayTracing=false`

This setting change is not required in UE 4.25.1 (or later) or people with Epic's 13205426 changelist. This warning is also in the Getting Started portion of the [README](../../README.md).

### Fix UxtFollowComponent to work with +X convention (PR 370)
`UxtFollowComponent` now works appropriately, following (Actor's) +X convention. That means that, if an asset was rotated by 180º in any subcomponent to be displayed as expected, that should now be reverted, rotating the Actor appropriately instead.
