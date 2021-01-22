---
title: Release Notes
description: Release notes for the latest UXT release.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, release notes
---

# UX Tools 0.12.0 release notes

- [What's new](#whats-new)
- [Breaking changes](#breaking-changes)
- [Known issues](#known-issues)
- [Full change list](#full-change-list)

This release of the UX Tools supports only HoloLens 2. Support for other MR platforms remains a goal for us and we are hoping to be able to deliver it in the near future.

Unreal 4.26 required.

## What's new

These are some of this release's highlights. For a more comprehensive list of changes see the [full change list](#full-change-list).

### TapToPlace component MaintainOrientation setting

Added `MaintainOrientation` mode to `UxtTapToPlaceComponent`. It allows to preserve the original rotation of the object while it is being repositioned.

### BoundsOverride property in UxtBoundsControl

`UUxtBoundsControlComponent` now has a `BoundsOverride` property, which (if set) will make the bounds control box and affordances display around that `UPrimitiveComponent` instead of around the whole actor. Please note that this setup will still modify the actor's root transform.

| Unset | Set |
| --- | --- |
| ![Bounds control override unset](Images/ReleaseNotes/bounds_control_override_unset.jpg) | ![Bounds control override set](Images/ReleaseNotes/bounds_control_override_set.jpg) |

### UxtBoundsControl and UxtTapToPlace integration

Thanks to the new [`BoundsOverride` property in `UUxtBoundsControlComponent`](#boundsoverride-property-in-uxtboundscontrol) and other few changes, this component and `UUxtTapToPlaceComponent` can now work together.

![BoundsControl - TapToPlace integration](Images/ReleaseNotes/boundscontrol_taptoplace_intregration.gif)

## Breaking changes

### `UxtHandTrackingFunctionLibrary` removed

This function library was just a thin wrapper around the `IUxtHandTracker` interface. It has been removed to make code less redundant. The `IUxtHandTracker` singleton getter returns a reference instead of a pointer for simpler access. If the function library was used in blueprints, the equivalent functions of the XR tracking system should be used, such as [Get Motion Controller Data](https://docs.unrealengine.com/en-US/BlueprintAPI/Input/XRTracking/GetMotionControllerData/index.html).

### UxtMathUtilsFunctionLibrary's API update

`CalculateNestedActorBoundsInGivenSpace` and `CalculateNestedActorBoundsInLocalSpace` have been removed in favor of a simpler and more flexible `CalculateNestedBoundsInGivenSpace`.

Hopefully, switching to the new function is not troublesome, but here are some guidelines:

- The first parameter is now a `USceneComponent` instead of an `AActor`. Simply add a `GetRootComponent()` to the previously used parameter.
- If you were using the `InLocalSpace` variant, now you need to pass in the local space explicitly. On the same component that you're now passing as first parameter (see previous point), simply use `GetComponentTransform().Inverse()`.
- The `Ignore` parameter is now a `TArrayView<const USceneComponent* const>`, instead of a single component. Typically, enclosing the previously used parameter in curly braces `{}` will suffice, thanks to the [TArrayView's initializer list constructor][tarrayview-initializer-list-ctor]. The [MakeArrayView overload list][makearrayview-overload-list] and [MoveTemp][movetemp] utilities might come in handy, too.

## Known issues

## Full change list

[tarrayview-initializer-list-ctor]: https://docs.unrealengine.com/en-US/API/Runtime/Core/Containers/TArrayView/__ctor/3/index.html
[makearrayview-overload-list]: https://docs.unrealengine.com/en-US/API/Runtime/Core/Containers/MakeArrayView/index.html
[movetemp]: https://docs.unrealengine.com/en-US/API/Runtime/Core/Templates/MoveTemp/index.html
