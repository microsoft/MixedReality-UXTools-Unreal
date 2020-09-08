---
title: Far Beam
description: Guide to Far Beam, a component used to visualize the ray used for far interactions.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, far beam
---

# Far Beam

The far beam component allows the user to visualise elements in the scene they can interact with from afar.

If you wish to customise the look of the beam, you can set the beam material by calling SetBeamMaterial with the material you wish to use. This material can have the following parameters if you wish to use them in your material.

- **IsGrabbing**: Scalar parameter. This value will be 0.0f if the user is not grabbing with the far cursor, 1.0f if they are.
- **handIndex**: Scalar parameter. This value will be 0.0f for the left hand, 1.0f for the right hand.

The default material for the far beam has the following parameters:

![FarBeamMatInterface](Images/FarBeamMatInterface.png)

## Far Beam Material Interface

### Color
Color value for the solid beam

### Emissive
Emissive intensity for the beam

### Gradation Hardness
Scalar value to control the gradation hardness

### Gradation Multiply
Scalar value for the gradation of the solid beam

### Gradation Offset
Scalar value for the gradation start position offset

### Gradation Scale
Scalar value to control the gradation scale

### Line Number
Scalar value to control the dot amount in the beam

### Middle Fade
Scalar value to control the fade-out intensity of the dotted beam

### Mid Fade Offset
Scalar value to control the fade-out position of the dotted beam

### Is Grabbing
System value for if the user is currently grabbing

### Hand Index
System value used for distinguishing left from right hand 

### Spline Length
System value used for the beam length


