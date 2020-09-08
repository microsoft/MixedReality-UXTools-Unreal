![Mixed Reality Toolkit](Docs/Images/MRTK_Unreal_UXT_Banner_Rounded.png)

# What is the Mixed Reality Toolkit?

UX Tools for Unreal Engine is a UE game plugin with code, blueprints and example assets created to help you add in features commonly needed when you're developing UX for mixed reality applications. The project is still in early development (it provides a small set of features and breaking changes are to be expected) but the current features are complete and robust enough to use in your own projects.  

> [!NOTE]
> Only HoloLens 2 development is supported at the moment. 

Features:
- [Input simulation](Docs/InputSimulation.md) - Simulates articulated hands and head pose that you can use in-editor. This is great for improving development iteration times.
- [Hand interaction actor](Docs/HandInteraction.md) - Used to interact with our UX components with articulated hands.
- [Pressable button component](Docs/PressableButton.md) - Low level component used to drive the state of button blueprints. Example button blueprints are provided.
- [Pinch Slider](Docs/PinchSlider.md) - Allows the user to continuously change a value by moving the slider thumb along the track.
- [Bounds Control](Docs/BoundsControl.md) - Allows the user to change the position, rotation, and size of an actor, using _affordances_.
- [Manipulator component](Docs/Manipulator.md) - Allows moving, rotating and scaling an actor with either one or two hand manipulation.
- [Follow behaviour component](Docs/FollowComponent.md) - Keeps an actor or component within sight of another component, usually the camera.
- [Hand constraint component](Docs/HandConstraintComponent.md) - Keeps the actor position and rotation aligned with a hand while avoiding overlaping with it.
- [Palm up constraint component](Docs/PalmUpConstraintComponent.md) - A [Hand constraint](Docs/HandConstraintComponent.md) specialization that activates only when the palm is facing the player.
- [Far beam component](Docs/FarBeam.md) - Allows the user to visualise elements in the scene they can interact with from afar.
- [Graphics documentation](Docs/Graphics.md) - Breakdown of shaders, materials, and graphics techniques used to render UX components.
- The plugin also contains a handful of [utilities](Docs/Utilities.md) that augment the Unreal Engine editor.

![Features](Docs/Images/Features.png)

# Getting started with UX Tools

| [![Getting Started and Documentation](Docs/Images/MRTK_Icon_GettingStarted.png)](https://microsoft.github.io/MixedRealityToolkit-Unity/Documentation/GettingStartedWithTheMRTK.html)<br/>[Getting Started](https://microsoft.github.io/MixedRealityToolkit-Unity/Documentation/GettingStartedWithTheMRTK.html)| [![Feature Guides](Docs/Images/MRTK_Icon_FeatureGuides.png)](https://microsoft.github.io/MixedRealityToolkit-Unity/Documentation/README_Button.html)<br/>[Feature Guides](https://microsoft.github.io/MixedRealityToolkit-Unity/Documentation/README_Button.html)| [![API Reference](Docs/Images/MRTK_Icon_APIReference.png)](https://microsoft.github.io/MixedRealityToolkit-Unity/api/Microsoft.MixedReality.Toolkit.html)<br/>[API Reference](https://microsoft.github.io/MixedRealityToolkit-Unity/api/Microsoft.MixedReality.Toolkit.html)|
|:---|:---|:---|:---|

# Build status

| Branch | CI Status | Docs Status |
|---|---|---|
| `public/0.9.x` |[![CI Status](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_apis/build/status/public/mrtk_CI?branchName=mrtk_development)](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_build/latest?definitionId=15)|[![Docs Status](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_apis/build/status/public/mrtk_docs?branchName=mrtk_development)](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_build/latest?definitionId=7)

# Required software

 | [![Windows SDK 18362+](Documentation/Images/MRTK170802_Short_17.png)](https://developer.microsoft.com/windows/downloads/windows-10-sdk) [Windows SDK 18362+](https://developer.microsoft.com/windows/downloads/windows-10-sdk)| [![Unity](Documentation/Images/MRTK170802_Short_18.png)](https://unity3d.com/get-unity/download/archive) [Unity 2018.4.x](https://unity3d.com/get-unity/download/archive)| [![Visual Studio 2019](Documentation/Images/MRTK170802_Short_19.png)](http://dev.windows.com/downloads) [Visual Studio 2019](http://dev.windows.com/downloads)| [![Emulators (optional)](Documentation/Images/MRTK170802_Short_20.png)](https://docs.microsoft.com/windows/mixed-reality/using-the-hololens-emulator) [Emulators (optional)](https://docs.microsoft.com/windows/mixed-reality/using-the-hololens-emulator)|
| :--- | :--- | :--- | :--- |
| To build apps with MRTK v2, you need the Windows 10 May 2019 Update SDK. <br> To run apps for immersive headsets, you need the Windows 10 Fall Creators Update. | The Unity 3D engine provides support for building mixed reality projects in Windows 10 | Visual Studio is used for code editing, deploying and building UWP app packages | The Emulators allow you to test your app without the device in a simulated environment |

## Example maps

If you want to explore the different UXT features or want a reference for how to use them we recommend having a look at the example maps contained in the _UX Tools Game_ (/UXToolsGame) in this repository. For that you should:

1. [Clone](https://help.github.com/en/desktop/contributing-to-projects/cloning-a-repository-from-github-to-github-desktop) this repository.
1. [Checkout](https://help.github.com/en/desktop/contributing-to-projects/switching-between-branches) public/0.9.x. 
    * Bear in mind that this branch is alive. It's not a release, and will be **updated regularly with potentially breaking changes**. There will be a release tag (e.g. release/0.9.0) marked as such in GitHub.

You can now open the _UX Tools Game_ (/UXToolsGame) and explore individual example maps or open the _Loader_ level to access some of the examples from a centralized hub. 

## Packaged UX Tools game

We also provide the UX Tools game pre-packaged for HoloLens 2 so you can try out the main UXT features directly on device easily. To use it:

1. Obtain the packaged game from the latest release page (e.g. _UXTGame-HoloLens.0.9.0.zip_) and unzip it to a local directory.
1. Install it in the device via the [Device Portal](https://docs.microsoft.com/en-us/windows/uwp/debug-test-perf/device-portal-hololens).


# Documentation

The latest version of the documentation can be found [here](https://microsoft.github.io/MixedReality-UXTools-Unreal).


# Feedback and contributions

Due to the early stage of the project and the likelihood of internal refactors, we are not in a position to accept external contributions through pull requests at this time. However, contributions and feedback in the shape of bug reports, suggestions and feature requests are always welcome!
