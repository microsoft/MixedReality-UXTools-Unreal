# Hand Constraint Component

Component that calculates a goal based on hand tracking and moves the owning actor.

Several zones around the hand supported: radial and ulnar for the thumb side and its opposite, as well as above and below the hand. The goal position is computed by casting a ray in the direction of the one at a bounding box around the hand joints.

The constraint can be oriented on either the hand rotation alone or facing the player.

## Usage

Create a HandConstraintComponent on an Actor. Set the Hand property to select which hand should be tracked. If 'Any Hand' is selected, the first tracked hand will be used and switch to the opposite when tracking is lost.

The Zone defines the general area around the hand that the actor is placed in. GoalMargin can be used to increase the distance from the hand for larger actors.

![Zones of the hand constraint](Images/HandConstraint/Zones.png)

At runtime the component will move the actor towards the goal position and rotation. Movement can be disabled with the MoveOwningActor option. In that case the goal position and rotation will still be computed and can be used in blueprints.

The component will by default use smoothing to avoid jittering artifacts resulting from hand tracking. Smoothing can be disabled by setting LocationLerpTime and/or RotationLerpTime properties to zero. The higher these values, the more smoothing will be applied and the longer it will take for the actor to reach the goal.

## Rotation Modes

Two main rotation modes are supported:

1. _Look-at-Camera_: The actor X axis is oriented towards the player head, with Z in the global "up" direction.
    ![Look-at-Camera orientation](Images/HandConstraint/LookAtCamera.png)

1. _Hand Rotation_: The actor aligns with the palm. X axis is facing the inside of the palm, Z aligns with the direction of fingers.
    ![Hand palm orientation](Images/HandConstraint/HandRotation.png)

The zone direction and the rotation can be configured independently. For example the zone offset can be aligned with the palm, while the rotation faces the camera:

![Mixed rotation modes](Images/HandConstraint/MixedRotation.png)
