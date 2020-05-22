![Mixed Reality Toolkit](Docs/Images/MRTK_Unreal_UXT_Banner_Rounded.png)

# Introduction

UX Tools for Unreal Engine is a UE game plugin with code, blueprints and example assets created to help in the implementation of features commonly needed when developing UX for mixed reality applications. The project is still in early development (it provides a small set of features and breaking changes are to be expected) but the included features are complete and robust enough to use in your own projects. Only HoloLens 2 is supported at the moment.

Features:
- [Input simulation](Docs/InputSimulation.md): simulated articulated hands and head pose for use in-editor. Great for improving development iteration times.
- [Hand interaction actor](Docs/HandInteraction.md): used to interact with our UX components with articulated hands.
- [Pressable button component](Docs/PressableButton.md): low level component used to drive the state of button blueprints. An example button blueprint is provided.
- [Manipulator component](Docs/Manipulator.md): allows moving and rotating an actor via one or two hand manipulation.
- [Follow behaviour component](Docs/FollowComponent.md): keeps an actor or component within sight of another component, usually the camera.
- [Graphics documentation](Docs/Graphics.md): breakdown of shaders, materials, and graphics techniques used to render UX components.

![Features](Docs/Images/Features.png)

Included but still in development is a bounding box component that allows manipulation of an actor's transform via affordances.

# Getting Started

1. Get UE 4.25 or later. Earlier previews or UE versions are not supported.
1. **Note**, if you are running UE 4.25.0 and your computer's GPU supports ray tracing you must disable UE 4.25 ray tracing support for UXT's custom shaders to compile. To disable ray tracing navigate to _C:\Program Files\Epic Games\UE_4.25\Engine\Config\Windows_ (or where your installation is located), open _DataDrivenPlatformInfo.ini_ and change _bSupportsRayTracing=true_ to _bSupportsRayTracing=false_ (this setting change is not required in UE 4.25.1 or later).
1. Clone this repository.
1. Checkout public/0.8.x. Bear in mind this branch is alive, it is not a release, and will be **updated regularly with potentially breaking changes**. There will be a release tag (e.g. release/0.8.0) marked as such in GitHub.

At this point it can be useful to have a look at the example maps provided in the UX Tools Game (_/UXToolsGame_) to get familiar with the plugin's functionality.

To use the plugin in a game:
1. Copy _/UXToolsGame/Plugins/UXTools_ to the game's _Plugins_ directory. Make sure the game project is a code one in order to be able to build the plugin sources.
2. Open the game project and enable the UX Tools plugin in the plugins menu.

To make sure everything is working as expected, add a hand interaction actor and an instance of the simple button blueprint to a level and try pressing the button with the simulated hands when playing in the editor.

# Documentation

The latest version of the documentation can be found [here](https://microsoft.github.io/MixedReality-UXTools-Unreal).

# Feedback and Contributions

Due to the early stage of the project and the likelihood of internal refactors, we are not in a position to accept external contributions via pull requests at this time. However, contributions and feedback in the shape of bug reports, suggestions and feature requests are welcome and encouraged.
