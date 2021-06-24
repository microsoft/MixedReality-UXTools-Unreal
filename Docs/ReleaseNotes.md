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
  - [Graduation of the scrolling object collection from experimental](#graduation-of-the-scrolling-object-collection-from-experimental)
- [Breaking changes](#breaking-changes)
  - [UxtBoundsControl](#uxtboundscontrol)
- [Known issues](#known-issues)
- [Full change list](#full-change-list)

This release of the UX Tools has been tested on HoloLens 2 and Windows Mixed Reality VR but should work on all [XR devices supported by Unreal Engine via OpenXR](https://docs.unrealengine.com/en-US/SharingAndReleasing/XRDevelopment/OpenXR/#platformsupport):
- HoloLens 2
- Windows Mixed Reality VR
- Oculus
- Steam VR

Unreal 4.26 required.

## What's new

These are some of this release's highlights. For a more comprehensive list of changes see the [full change list](#full-change-list).

### Graduation of the scrolling object collection from experimental

The scrolling object collection has been updated and is no longer tagged as experimental. The component now supports per-pixel item clipping, compound item input routing, and a handful of bugs and quality of life issues have been addressed. Check out the [new documentation](ScrollingObjectCollection.md) and example level to learn more.

![ScrollingObjectCollectionVariants](Images/ScrollingObjectCollection/ScrollingObjectCollectionVariants.png)

### Re-parented UxtBoundsControl's affordances

**Uxt Bounds Control** places its associated affordances and box collider in an actor that it creates on **Begin Play**. This external actor used to be un-parented and required its transform (and those of the affordances) to be recalculated explicitly every frame. Now, the external actor is parented to the one that owns the **Uxt Bounds Control** component and it leverages the `TransformUpdated` event instead of the tick function, so it only recalculates the scale of the affordances when necessary.

## Breaking changes

### UxtBoundsControl

As described [above](#re-parented-uxtboundscontrols-affordances), affordances are re-parented, so there are a few things to keep in mind:

- The `UpdateAffordanceTransforms` protected member has been removed so, if you have any explicit calls to this function for some reason, you should remove them. Everything should still work as expected, because the affordance transforms are updated automatically when modifying the bounds' root transform.
- `InitialRelativeScale` and `ReferenceRelativeScale` inside `FUxtAffordanceInstance` need to be absolute now, so they have been renamed to `InitialScale` and `ReferenceScale`. If you were using them, please make sure to update their usage.

## Known issues

### Crash in UX Tools game when leaving surface magnetism or tap to place maps

This happens only when running on device without OpenXR. It's caused by a bug in the engine (UEVR-2118) that happens after stopping an AR session with tracked geometry enabled. We expect it will be fixed in the next UE4 release (4.26.3 or 4.27).

## Full change list
