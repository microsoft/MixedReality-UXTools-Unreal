---
title: Utilities
description: Guide to UX utilities that augment the Unreal Engine editor.
author: luis-valverde-ms
ms.author: luval
ms.date: 08/25/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, Utilities
---

# Utilities

UX Tools contains a handful of utilities that augment the Unreal Engine editor.

## Editor Utility Blueprints

Editor utilities can be authored using [scripted actions](https://docs.unrealengine.com/en-US/Engine/Editor/ScriptingAndAutomation/Blueprints/ScriptedActions/index.html). Scripted actions are accessed by right-clicking actors or assets.

### Align Actors

The `Align Actors` action aides in the layout of UX controls, or any actor type. To access the action select multiple actors you wish to align. Then right-click in a viewport or outliner window. Finally, select _Scripted Actions > Align Actors_. A properties window will pop up prompting for alignment settings.

Note, the first actor selected is used as the alignment origin. The actor's bounds are used to ensure actors don't interpenetrate when aligned.

![AlignActors](Images/Utilities/UtilitiesAlignActors.png)

## Editor Utility Widgets

[Editor utility widgets](https://docs.unrealengine.com/en-US/Engine/UMG/UserGuide/EditorUtilityWidgets/index.html) can be used to modify the User Interface (UI) of the Unreal Editor.

### Icon Brush Editor

The `Icon Brush Editor` editor utility widget aides in editing a `UxtIconBrush` by visually searching  though the characters in a [`UFont`](https://docs.unrealengine.com/en-US/API/Runtime/Engine/Engine/UFont/index.html). To open the editor click the _"Open Icon Brush Editor"_ button from any `UxtIconBrush` details panel. Once opened, the editor should display a window similar to the one below:

![UtilitiesIconBrushEditor](Images/Utilities/UtilitiesIconBrushEditor.png)

Selecting different icons, outlined in green, will apply the icon selection to the current `UxtIconBrush`.

The `Icon Brush Editor` comes in handy when selecting new icons for controls. The `BP_ButtonHoloLens2` blueprint makes the viewer accessible via the top level [details panel](https://docs.unrealengine.com/en-US/Engine/UI/LevelEditor/Details/index.html):

![UtilitiesIconBrushEditorDetails](Images/Utilities/UtilitiesIconBrushEditorDetails.png)
