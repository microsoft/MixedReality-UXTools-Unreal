---
title: Bounds Control
description: Guide to Bounds Control, a component that allows you to rotate, translate and scale an actor using affordances.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, bounds control, bounding box
---

# BoundsControl

Bounds Control is a component that allows the user to change the position, rotation, and size of an actor, using _affordances_. Affordances are grabbable areas on corners, edges, and faces of the actor's bounding box.

To enable bounds control on an actor, add a UxtBoundsControl component. The component has a default configuration that can be tweaked to change the behavior and appearance as needed.

![FollowComponent](Images/BoundsControl.png)

## Bounds Control Config

Bounds control uses `UxtBoundsControlConfig` data assets that describe the layout of affordances.

A number of presets exist for standard behavior in _Bounds Control/Presets_:
* _BoundsControlDefault_: Uniform resizing with corners and rotation with edges.
* _BoundsControlSlate2D_: Only front corners and edges are shown, all resize.
* _BoundsControlAllResize_: Full set of affordances, all resizing.
* _BoundsControlAllTranslate_: Full set of affordances, all translating.
* _BoundsControlAllScale_: Full set of affordances, all scaling.
* _BoundsControlAllRotate_: Full set of affordances, all rotating.
Users can create custom bounds control setups by copying or adding new `UxtBoundsControlConfig` data assets.

The config asset contains a list of affordances. Each affordance is described by its _Placement_ on the bounding box, a combination of the affordance kind and directions, for example:
* `CornerFrontTopLeft`
* `EdgeBottomRight`
* `FaceBack`
* `Center`

The affordance _Action_ describes the effect on the bounding box when moving the affordance:
* _Resize_: Move only one side of the bounding box.
* _Translate_: Move both sides of the bounding box in parallel.
* _Scale_: Scale the bounding box, moving both sides in opposite directions.
* _Rotate_: Rotate the bounding box about its center point.

The _Uniform Action_ flag can be turned off to allow non-uniform scaling and translation.

_Locked Axes_ flags restrict actions in the local space of the bounds control. For example, if the `X` axis is locked then moving an affordance forward will not have an effect. Two axes can be locked to allow movement only along the remaining axis.

## Affordance meshes

At runtime a separate actor is created for displaying affordances. Each affordance is a StaticMesh component on the BoundsControlActor. The mesh used for each kind of affordance (Corner, Edge, Face, Center) can be changed on the bounds control component (`Corner Affordance Mesh` etc.).

When creating custom affordance meshes they should by default be oriented in the _forward_, _right_, _up_ direction. Each instance is then automatically rotated to match its placement on the bounding box. It is recommended to use simple box collision primitives to make affordances grabbable.