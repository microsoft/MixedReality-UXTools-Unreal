# Tap To Place

The UXT Tap To Place component is a far interaction component used to place objects against surfaces. It is especially useful for placing objects against the spacial mesh. You can initiate the tap to place behaviour by selecting the target object with far interaction. The object will then track either the head or the hand, moving along hit surfaces until it the user ends placement by making any other far tap.

![TapToPlace](Images/TapToPlace.gif)

To enable real world collision generation, the following values should be enabled in `ARSessionConfig > ARSettings > World Mapping`:

- [x] `Generate Mesh Data from Tracked Geometry`
- [x] `Generate Collision for Mesh Data`

You can also add a World Override blueprint in `Blueprint > World Override` (see TapToPlaceExample for reference).

## Usage

Add a primitive component to your actor/actor blueprint. Also add a UxtTapToPlaceComponent. Set the _Target Component_ property to reference the primitive component you have already added. You should now be able to select the primitive with far interaction to initiate placement.

## Events

- **OnBeginFocus**: Event raised when the far pointer starts focusing the tap to place target primitive.
- **OnUpdateFocus**: Event raised when the focusing far pointer updates.
- **OnEndFocus**: Event raised when the far pointer stops focusing the tap to place target primitive.
- **OnBeginPlacing**: Event raised when a far pointer selects the tap to place target primitive and placement begins.
- **OnEndPlacing**: Event raised when any far pointer is pressed during placement. 

## Properties

**Orientation Type**: This property provides options for how the target component will be placed against hit surfaces.
- If _Align to Camera_ is selected, the target component will always face the camera.
- If _Align To Surface_ is selected, the target component will align with the normal of the surface the component is being placed against.

**Placement Type**: This property provides options for what the target component will track during placement.
- If _Head_ is selected, the component will be placed at the centre of the users vision and will move as the user moves their hand.
- If _Hand_ is selected, the component will be placed at the end of the hand [Far Beam](FarBeam.md) and will move as the user moves their hand.

**Keep Orientation Vertical**: If true, the object will remain upright, even if being placed against surfaces that are not upright.

**Default Placement Distance**: During placement, if there is no surface to place against, the target component will be placed at this distance from the head/hand.

**Max Raycast Distance**: The maximum distance to ray cast to check if there are surfaces to place against.

**Trace Channel**: The trace channel used for the ray cast.

**Interpolate Pose**: If true, the object will interpolate smoothly between poses.

**Lerp Time**: The rate at which the object will interpolate between target poses if _Interpolate Pose_ is true.

**Target Component**: A reference to the primitive component that is transformed by the tap to place component. Selecting this primitive with far interaction will initiate placement.

