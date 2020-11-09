---
title: Release Notes
description: Release notes for the latest UXT release.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, release notes
---

# UX Tools 0.11.0 release notes

- [What's new](#whats-new)
- [Breaking changes](#breaking-changes)

This release of the UX Tools supports only HoloLens 2. Support for other MR platforms remains a goal for us but is not the current focus.

Unreal 4.25 required.

## What's new

### Examples Plugin

UX Tools common content and example scenes were moved to a separate plugin to make it easier to incorporate them
as a starting point for new projects.

## UxtPalmUpConstraintComponent

The palm-up constraint has gained the _Require Gaze_ option to ensure the user is deliberately trying to use the constraint and help prevent false activations. This is particularly useful for world-locking hand menus as it prevents the menu from unintentionally re-attaching to the hand.

### UxtPinchSliderActor

The pinch slider actor has gained a number of quality of life improvements. These include:

- Customizable minimum / maximum values.
- GetValue, SetValue and OnSliderUpdateValue exposed directly on the actor to allow easy access to the slider's value relative to the custom min / max value. (note: if using the underlying `UxtPinchSliderComponent`, it's value will still be in the range 0-1)
- Stepped movement along the slider's tick marks.

### UxtPinchSliderComponent

The pinch slider component now has the option to use stepped movement. This can be configured in the advanced settings for the component.

## Breaking changes

## UxtHandConstraintComponent

`UUxtHandConstraintComponent::IsHandUsableForConstraint()` is now a non-const member function.

### UxtPinchSliderActor

As part of adding a customizable minimum / maximum slider value, the _InitialValue_ property has been replaced with a _Value_ property. Previous settings can be easily carried forward by:

1. Add `+PropertyRedirects=(OldName="UxtPinchSliderActor.InitialValue", NewName="UxtPinchSliderActor.Value")` to _DefaultUXTools.ini_. (found in the plugin's configuration folder)
2. Re-save any levels with sliders to update their properties.
3. Remove the property redirect from _DefaultUXTools.ini_.
