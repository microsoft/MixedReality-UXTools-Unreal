# Palm-Up Constraint Component

[Hand constraint](HandConstraintComponent.md) specialization that activates only when the palm is facing the player.

![Palm facing camera](Images/HandConstraint/PalmUpFacingCamera.png)
![Palm facing camera](Images/HandConstraint/PalmUpFacingAway.png)

## Usage

Create a PalmUpConstraintComponent on an Actor. See [Hand constraint usage](HandConstraintComponent.md#Usage) for common settings.

The Palm up constraint will activate when the palm normal is within a cone of size _Max Palm Angle_ of the camera direction.

Optionally the constraint can also use a hand flatness condition by enabling the _Require Flat Hand_ option. Flatness is approximated by checking the triangle between palm, index finger tip and ring finger tip. If the triangle aligns to the palm within the _Max Flat Hand Angle_ the hand is considered flat.