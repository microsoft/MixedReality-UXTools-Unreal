# Utilities

UX Tools contains a handful of utilities that augment the Unreal Engine editor.

## Editor Utility Blueprints

Editor utilities can be authored using [scripted actions](https://docs.unrealengine.com/en-US/Engine/Editor/ScriptingAndAutomation/Blueprints/ScriptedActions/index.html). Scripted actions are accessed by right-clicking actors or assets.

### Align Actors

The `Align Actors` action aides in the layout of UX controls, or any actor type. To access the action select multiple actors you wish to align. Then right-click in a viewport or outliner window. Finally, select _Scripted Actions > Align Actors_. A properties window will pop up prompting for alignment settings.

Note, the first actor selected is used as the alignment origin. The actor's bounds are used to ensure actors don't interpenetrate when aligned.

![AlignActors](Images/Utilities/UtilitiesAlignActors.png)

