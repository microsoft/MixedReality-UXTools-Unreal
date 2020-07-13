# Surface Magnetism

The surface magnetism component allows the user to interact with the component and stick the containing actor to a surface (either in game or to real world walls). Interaction is done via far interaction only. 

To enable real world collision generation, the following values should be enabled in `ARSessionConfig > ARSettings > World Mapping`:

- [x] `Generate Mesh Data from Tracked Geometry`
- [x] `Generate Collision for Mesh Data`

Here are the events that you can use to hook up to you application logic:

- **OnMagnetismStarted**: This event is called when the user interacts with the component.
- **OnMagnetismEnded**: This event is called when the user stops interacting with the component or if movement is smoothed, this is called when the component comes to a stop after interaction ends.

## Instance Editable Properties

### Magnetism Type
The line trace for surfaces to stick to can either come from the head using look rotation or from the hand, using hand rotation. 

### Trace Distance
How far from the origin do we trace to hit surfaces

### Smooth position
If this is off, the target will snap instantly to the hit location. If this is on, the target will interpolate between current position and hit location.

### Position Interp Value
If smooth position is on, this is the value for the speed of interpolation (using standard UE4 Interp values)

### Smooth Rotation
If this is off, the target will snap instantly to the hit normal. If this is on, the target will interpolate between current rotation and hit normal.

### Rotation Interp Value
If smooth rotation is on, this is the value for the speed of interpolation (using standard UE4 Interp values)

### Impact Normal Offset
This value will move the target position away from the hit point in the direction of the hit normal.

### Trace Ray Offset
This value will move the target position away from the hit point in the direction of the traced ray (i.e. back towards the user).

### Trace Channel
The channel that is traced for surfaces to stick to.

### Auto Bounds
If this is on, the component will automatically attempt to adjust collision box size to the bounds of the owning actor.

### Box Bounds
If Auto Bounds is off, here you can manually set the collision box bounds.

### Only Enable Yaw
If this is on, pitch and roll will be omitted.

### Collision Profile
The collision profile used for the interactive element (the box collision).
