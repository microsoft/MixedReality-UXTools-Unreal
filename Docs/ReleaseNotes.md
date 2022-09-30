---
title: Release Notes
description: Release notes for the latest UXT release.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, release notes
---

# UX Tools 0.12.1 release notes

- [What's new](#whats-new)
  - [Fixed non-uniform scaling in Bounds Control when the actor was rotated](#fixed-non-uniform-scaling-in-bounds-control-when-the-actor-was-rotated)
  - [Hand tracking component no longer consumes input events](#hand-tracking-component-no-longer-consumes-input-events)

This release of the UX Tools has been tested on HoloLens 2 and Windows Mixed Reality VR but should work on all [XR devices supported by Unreal Engine via OpenXR](https://docs.unrealengine.com/en-US/SharingAndReleasing/XRDevelopment/OpenXR/#platformsupport):
- HoloLens 2
- Windows Mixed Reality VR
- Oculus
- Steam VR

Unreal 5.0 required.

This is an incremental hotfix release. Please review changes in [0.12.0 Release Notes](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.12.x/Docs/ReleaseNotes.html) for information regarding changes introduced in the major UX Tools version.

## What's new

### Fixed non-uniform scaling in Bounds Control when the actor was rotated

Non-uniformly scaling an actor via Bounds Control after having rotated it will no longer result in an undesired/unexpected transformation.

### Hand tracking component no longer consumes input events

UXT is using input events to set internal state for interacting with input widgets, but other actors in the scene outside of UXT may need to use the same events for other purposes. UXT actions no longer consume input, so other events with the same action will still fire.
