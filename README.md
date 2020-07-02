![Mixed Reality Toolkit](Docs/Images/MRTK_Unreal_UXT_Banner_Rounded.png)

# Introduction

UX Tools for Unreal Engine is a UE game plugin with code, blueprints and example assets created to help you add in features commonly needed when you're developing UX for mixed reality applications. The project is still in early development (it provides a small set of features and breaking changes are to be expected) but the current features are complete and robust enough to use in your own projects.

> [!NOTE]
> Only HoloLens 2 development is supported at the moment.

Features:
- [Input simulation](Docs/InputSimulation.md) - Simulates articulated hands and head pose that you can use in-editor. This is great for improving development iteration times.
- [Hand interaction actor](Docs/HandInteraction.md) - Used to interact with our UX components with articulated hands.
- [Pressable button component](Docs/PressableButton.md) - Low level component used to drive the state of button blueprints. An example button blueprint is provided.
- [Manipulator component](Docs/Manipulator.md) - Allows moving and rotating an actor with either one or two hand manipulation.
- [Follow behaviour component](Docs/FollowComponent.md) - Keeps an actor or component within sight of another component, usually the camera.
- [UIElement component](Docs/UIElements.md) - Allows for easily hiding/showing groups of UI elements (including nested panels) while remembering the individual visibility setting for each element.
- [Graphics documentation](Docs/Graphics.md) - Breakdown of shaders, materials, and graphics techniques used to render UX components.


Included but still in development is a bounding box component that allows manipulation of an actor's transform via affordances.

![Features](Docs/Images/Features.png)


# Getting Started

1. Download [UE 4.25.1](https://www.unrealengine.com/get-now) or later. Earlier versions are not supported.
1. [Clone](https://help.github.com/en/desktop/contributing-to-projects/cloning-a-repository-from-github-to-github-desktop) this repository.
1. [Checkout](https://help.github.com/en/desktop/contributing-to-projects/switching-between-branches) public/0.8.x.
    * Bear in mind that this branch is alive. It's not a release, and will be **updated regularly with potentially breaking changes**. There will be a release tag (e.g. release/0.8.0) marked as such in GitHub.

It's a good idea to have a look at the example maps provided in the **UX Tools Game** (/UXToolsGame) to get familiar with how the plugin works.

To use the plugin in a game:
1. Copy the **UXTools** folder in **/UXToolsGame/Plugins/** to your game's **Plugins** directory.
    * Make sure the game project is a code one (not a BluePrint-only game) in order to be able to build the plugin sources.

2. Open the game project and enable the **UX Tools** plugin in the plugins menu.

You can find a complete setup guide in the [MRTK Getting Started](https://docs.microsoft.com/windows/mixed-reality/unreal-uxt-ch1) tutorial series.

To make sure everything is working as expected, add a hand interaction actor and an instance of the simple button blueprint to a level and try pressing the button with the simulated hands when playing in the editor.

# Documentation

The latest version of the documentation can be found [here](https://microsoft.github.io/MixedReality-UXTools-Unreal).

# Feedback and Contributions

Due to the early stage of the project and the likelihood of internal refactors, we are not in a position to accept external contributions through pull requests at this time. However, contributions and feedback in the shape of bug reports, suggestions and feature requests are always welcome!
