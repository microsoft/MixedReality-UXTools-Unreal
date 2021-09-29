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
  - [Bounds Control component](#bounds-control-component)

This release of the UX Tools has been tested on HoloLens 2 and Windows Mixed Reality VR but should work on all [XR devices supported by Unreal Engine via OpenXR](https://docs.unrealengine.com/en-US/SharingAndReleasing/XRDevelopment/OpenXR/#platformsupport):
- HoloLens 2
- Windows Mixed Reality VR
- Oculus
- Steam VR

Unreal 4.27 required.

## What's new

### Bounds Control component

#### Fix non-uniform scaling when the actor was rotated

Non-uniformly scaling an actor via Bounds Control after having rotated it will no longer result in an undesired/unexpected transformation.
