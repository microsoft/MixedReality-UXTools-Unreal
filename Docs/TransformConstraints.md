# Transform Constraints

UXTools provides a mechanism to implement and apply transform constraints to actors on manipulation by using a `UxtConstraintManager`. They are integrated with the `UxtManipulatorComponent` and `UxtBoundsControlComponent` out of the box.

## Usage

In order to use constraints:

1. Add any desired `UxtTransformConstraint`-derived components to the actor.
2. Add any components that contain and use a `UxtConstraintManager` instance, such as `UxtManipulatorComponent` and `UxtBoundsControlComponent`.

By default, every `UxtConstraintManager` instance will take all the constraint components attached to the actor into consideration.

## Built-in constraint components

There are a few built-in constraint components, which will hopefully save you from having to write your own.

### UxtFaceUserConstraint

Makes the actor face the camera while interacting with it.

Set `bFaceAway` to `true` to make the object face away from the camera.

![Face user constraint demo](Images/Constraints/Face_user_constraint.gif)

### UxtFixedDistanceConstraint

Makes the actor stay at a fixed distance from the camera.

| With | Without |
| --- | --- |
| ![Fixed distance constraint demo](Images/Constraints/Fixed_distance_with_constraint.gif) | ![Fixed distance without constraint demo](Images/Constraints/Fixed_distance_without_constraint.gif) |

### UxtFixedRotationToUserConstraint

Makes the actor maintain the same rotation (relative to the user) that it had when the interaction started.

Set `bExcludeRoll` to `false` to allow rolling of the actor.

![Fixed rotation to user constraint demo](Images/Constraints/Fixed_rotation_to_user_constraint.gif)

### UxtFixedRotationToWorldConstraint

Makes the actor maintain the same rotation (relative to the world) that it had when the interaction started.

![Fixed rotation to world constraint demo](Images/Constraints/Fixed_rotation_to_world_constraint.gif)

### UxtMaintainApparentSizeConstraint

Makes the actor maintain the apparent size (relative to the user) that it had when the interaction started.

![Maintain apparent size constraint demo](Images/Constraints/Maintain_apparent_size_constraint.gif)

### UxtMinMaxScaleConstraint

Limits the minimum and maximum scale of the actor during manipulation, via `MinScale` and `MaxScale` properties respectively.

Use `bRelativeToInitialScale` to configure whether the limits are relative to the scale at interaction start (`true`) or absolute (`false`).

![Min/max scale constraint demo](Images/Constraints/Min_max_scale_constraint.gif)

(Example limiting the minimum scale to `0.3`)

### UxtMoveAxisConstraint

Limits movement on specific axes. Use the `ConstraintOnMovement` bit mask of `EUxtAxisFlags`.

![Move axis constraint demo](Images/Constraints/Move_axis_constraint.gif)

(Example limiting the movement along the `X` and `Z` axes)

### UxtRotationAxisConstraint

Limits rotation on specific axes. Use the `ConstraintOnRotation` bit mask of `EUxtAxisFlags`.

![Rotation axis constraint demo](Images/Constraints/Rotation_axis_constraint.gif)

(Example limiting the rotation around the `X` and `Y` axes)

## Adding more constraint components

If none of the [Built-in constraint components](#built-in-constraint-components) suits your needs, you can add more by simply creating a new `UCLASS` that inherits from `UxtTransformConstraint`. Then, provide implementations for `GetConstraintType`, `ApplyConstraint` and (optionally) `Initialize`.

In this case, please feel free to take a look at our [contributing docs](CONTRIBUTING.md) and make a suggestion!
