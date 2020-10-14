# Surface Magnetism

The surface magnetism component allows the user to interact with the component and stick the containing actor to a surface (either in game or to real world walls). Interaction is done via far interaction only.

To enable real world collision generation, the following values should be enabled in `ARSessionConfig > ARSettings > World Mapping`:

- [x] `Generate Mesh Data from Tracked Geometry`
- [x] `Generate Collision for Mesh Data`

Here are the events that you can use to hook up to your application logic:

- **OnMagnetismStarted**: This event is called when the user interacts with the component.
- **OnMagnetismEnded**: This event is called when the user stops interacting with the component or, if movement is smoothed, when the component comes to a stop after interaction ends.

## Instance Editable Properties

### Trace Distance

Maximum length of the trace for a surface to stick to.

### Smooth position

If `true`, the target will interpolate between current position and hit location. If `false`, the target will snap instantly to the hit location.

### Position Interp Value

If `SmoothPosition` is `true`, this is the position's interpolation speed (using standard UE4 `Interp` values).

### Smooth Rotation

If `true`, the target will interpolate between current rotation and hit normal. If `false`, the target will snap instantly to the hit normal.

### Rotation Interp Value

If `SmoothRotation` is `true`, this is the rotation's interpolation speed (using standard UE4 `Interp` values)

### Impact Normal Offset

Distance offset from the hit point to place the target at, along the hit normal.

### Trace Ray Offset

Distance offset from the hit point to place the target at, along the traced ray (i.e. back towards the user).

### Trace Channel

The channel that is traced for surfaces to stick to.

### Keep Orientation Vertical

If `true`, pitch and roll are omitted.
