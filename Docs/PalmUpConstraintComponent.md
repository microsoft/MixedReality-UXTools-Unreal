---
title: Palm-Up Constraint Component
description: Guide to Palm-Up Constraint, a component for attaching an actor to a hand that hides it when the hand faces away from the user.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, Palm-Up Constraint Component, hand menu
---

# Palm-Up Constraint Component

[Hand constraint](HandConstraintComponent.md) specialization that activates only when the palm is facing the player.

![Palm facing camera](Images/HandConstraint/PalmUpFacingCamera.png)
![Palm facing camera](Images/HandConstraint/PalmUpFacingAway.png)

## Usage

Create a PalmUpConstraintComponent on an Actor. See Hand constraint documentation for common settings.

The Palm-Up constraint will activate when the palm normal is within a cone of size _Max Palm Angle_ of the camera direction.

Optionally the constraint can also use a hand flatness condition by enabling the _Require Flat Hand_ option. Flatness is approximated by checking the triangle between palm, index finger tip and ring finger tip. If the triangle aligns to the palm within the _Max Flat Hand Angle_ the hand is considered flat.