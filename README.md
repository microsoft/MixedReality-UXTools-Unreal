![Mixed Reality Toolkit](Docs/Images/MRTK_Unreal_UXT_Banner_Rounded.png)

# What are the UX Tools?

UX Tools for Unreal Engine is a UE game plugin with code, blueprints and example assets created to help you add in features commonly needed when you're developing UX for mixed reality applications. The project is still in early development (it provides a small set of features and breaking changes are to be expected) but the current features are complete and robust enough to use in your own projects.  

> [!NOTE]
> Only HoloLens 2 development is supported at the moment. 

![Features](Docs/Images/Features.png)

# Getting started with UX Tools

| [![Getting Started and Documentation](Docs/Images/MRTK_Icon_GettingStarted.png)](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.8.x/Docs/WelcomeToUXTools.html)<br/>[Getting Started](hhttps://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.8.x/Docs/WelcomeToUXTools.html)| [![Feature Guides](Docs/Images/MRTK_Icon_FeatureGuides.png)](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.8.x/Docs/InputSimulation.html)<br/>[Feature Guides](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.8.x/Docs/InputSimulation.html)| [![API Reference](Docs/Images/MRTK_Icon_APIReference.png)](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.8.x/api/_a_uxt_hand_interaction_actor.html)<br/>[API Reference](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.8.x/api/_a_uxt_hand_interaction_actor.html)|
|:---|:---|:---|

# Build status

<!-- LUIS: Does UXT have CI and Docs status links? If so, can you update below? -->
| Branch | CI Status | Docs Status |
|---|---|---|
| `public/0.8.x` |[![CI Status](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_apis/build/status/public/mrtk_CI?branchName=mrtk_development)](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_build/latest?definitionId=15)|[![Docs Status](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_apis/build/status/public/mrtk_docs?branchName=mrtk_development)](https://dev.azure.com/aipmr/MixedRealityToolkit-Unity-CI/_build/latest?definitionId=7)

# Required software

<!-- LUIS: Do you have a small UE image like the Unity one used as a placeholder below? -->
<!-- LUIS: Are the descriptions and required software content correct? -->
 | [![Windows SDK 18362+](Docs/Images/Windows-Logo.png)](https://developer.microsoft.com/windows/downloads/windows-10-sdk) [Windows SDK 18362+](https://developer.microsoft.com/windows/downloads/windows-10-sdk)| [![Unreal](Docs/Images/Unity-Logo.png)](https://www.unrealengine.com/get-now) [Unreal 4.25.1 or later](https://www.unrealengine.com/get-now)| [![Visual Studio 2019](Docs/Images/VS-Logo.png)](http://dev.windows.com/downloads) [Visual Studio 2019](http://dev.windows.com/downloads)|
| :--- | :--- | :--- | 
| To build apps with MRTK-Unreal and UX Tools, you need the Windows 10 May 2019 Update SDK. <br> To run apps for immersive headsets, you need the Windows 10 Fall Creators Update. | The Unreal Engine provides support for building mixed reality projects in Windows 10 | Visual Studio is used for code editing and deploying app packages |

# Feature areas

| ![Input Simulation](Docs/Images/Input-System.png) [Input Simulation](Docs/InputSimulation.md)<br/>&nbsp;  | ![Hand Interaction](Docs/Images/Hand-Tracking.png) [Hand Interaction](Docs/HandInteraction.md) | ![Pressable Button](Docs/Images/Pressable-Button.png) [Pressable Button](Docs/PressableButton.md) | ![Manipulators](Docs/Images/Hand-Tracking.png) [Manipulators](Docs/Manipulator.md)<br/>&nbsp; | ![Follow Behavior](Docs/Images/Follow-Behavior.png) [Follow Behavior](Docs/FollowComponent.md)<br/>&nbsp; | 
| :--- | :--- | :--- | :--- | :--- |

# Example maps

If you want to explore the different UXT features or want a reference for how to use them we recommend having a look at the example maps contained in the _UX Tools Game_ (/UXToolsGame) in this repository. For that you should:

1. [Clone](https://help.github.com/en/desktop/contributing-to-projects/cloning-a-repository-from-github-to-github-desktop) this repository.
1. [Checkout](https://help.github.com/en/desktop/contributing-to-projects/switching-between-branches) public/0.8.x. 
    * Bear in mind that this branch is alive. It's not a release, and will be **updated regularly with potentially breaking changes**. There will be a release tag (e.g. release/0.8.0) marked as such in GitHub.

You can now open the _UX Tools Game_ (/UXToolsGame) and explore individual example maps or open the _Loader_ level to access some of the examples from a centralized hub. 

# Packaged UX Tools game

We also provide the UX Tools game pre-packaged for HoloLens 2 so you can try out the main UXT features directly on device easily. To use it:

1. Obtain the packaged game from the latest release page (e.g. _UXTGame-HoloLens.0.9.0.zip_) and unzip it to a local directory.
1. Install it in the device via the [Device Portal](https://docs.microsoft.com/en-us/windows/uwp/debug-test-perf/device-portal-hololens).


# Documentation

The latest version of the documentation can be found [here](https://microsoft.github.io/MixedReality-UXTools-Unreal/version/public/0.9.x/Docs/WelcomeToUXTools.html).


# Feedback and contributions

Due to the early stage of the project and the likelihood of internal refactors, we are not in a position to accept external contributions through pull requests at this time. However, contributions and feedback in the shape of bug reports, suggestions and feature requests are always welcome!

