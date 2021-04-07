---
title: Release Notes
description: Release notes for the latest UXT release.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, release notes
---

# UX Tools 0.13.0 release notes

- [What's new](#whats-new)
  - [Custom SurfaceNormalOffset in TapToPlace component](#custom-surfacenormaloffset-in-taptoplace-component)
  - [TapToPlace component allows SceneComponent as target](#taptoplace-component-allows-scenecomponent-as-target)
- [Breaking changes](#breaking-changes)
  - [Manipulator and Pinch Slider smoothing fixes](#manipulator-and-pinch-slider-smoothing-fixes)
- [Known issues](#known-issues)
- [Full change list](#full-change-list)

Unreal 4.26 required.

## What's new

These are some of this release's highlights. For a more comprehensive list of changes see the [full change list](#full-change-list).

### Custom SurfaceNormalOffset in TapToPlace component

TapTopPlace component now allows to specify a custom offset between the object being placed and the surface it is being placed on. This can be achieved by setting `bUseDefaultSurfaceNormalOffset` to false and providing a custom offset in `SurfaceNormalOffset` property. The offset is measured from the pivot point of the object along the X axis (towards the positive direction as this is the side which is aligned with the surface).

If `bUseDefaultSurfaceNormalOffset` is set to true, the object will be aligned with the surface on which it is being placed and the value `SurfaceNormalOffset` will be calculated automatically. The offset will now be correct also for objects that use a pivot point which is not located in the centre of the bounding box.

### TapToPlace component allows SceneComponent as target

`UUxtTapToPlaceComponent` allows assigning `USceneComponent` as the `TargetComponent` (previously `UPrimitiveComponent` was required). This allows adding TapToPlace behaviour to any hierarchy of actor components and makes the experience consistent with the other _UX Tools_ components (e.g. `UxtGenericManipulator`).

## Breaking changes

### Manipulator and Pinch Slider smoothing fixes

The calculation of smoothed movement for the manipulator and pinch slider components was using an incorrect formula, leading to slower manipulation when the frame rate drops. The default smoothing factors in the `UUxtGenericManipulatorComponent` and `UUxtPinchSliderComponent` have been changed to yield the same behavior at the targeted frame rate of 60 FPS.

The smoothing factors for these components are now equivalent to the "Lerp Time" settings of `UUxtFollowComponent`, `UUxtHandConstraintComponent`, and `UUxtTapToPlaceComponent`. If your manipulator or slider uses a modified smoothing factor you must update the value. You can revert it to the default value, or you or you can calculate the exact equivalent smoothing at 60 FPS using the formula below:

`S_new = - 1 / (60 * log(1 - exp(-S_old / 60)))`

where log is the natural logarithm.

Here are a few values for comparison:
|`S_old`|`S_new`|
|---|---|
|0|0|
|0.1|0.0026|
|1|0.0041|
|10|0.0089|
|100|0.080|

## Known issues

## Full change list

