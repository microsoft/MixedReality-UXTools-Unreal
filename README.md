# Introduction 

UX Tools for Unreal Engine is a collection of C++ classes, Blueprints and example assets created to help in the implementation of features commonly needed when developing mixed reality applications, like pressable buttons or direct manipulation.

This currently takes the form of a game plugin found in [UXToolsGame/Plugins/UXTools](https://dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-UE?path=%2FUXToolsGame%2FPlugins%2FUXTools&version=GBmaster) and a collection of example maps in the containing game project [UXToolsGame](https://dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-UE?path=%2FUXToolsGame).

# Getting Started

Whether using the prebuilt plugin or building it yourself, you'll first need to obtain our internal version of UE. Vanilla UE 4.24 has HoloLens 2 support in beta but the plugin depends on internal changes that have not been released yet. You can find the latest binaries of our internal UE in  `\\cognitionfs\PUBLIC\Unreal\builds` (Redmond) or `\\havokfs\SelfHost\Sydney\UE4` (Dublin). Unzip it to a local folder and use it instead of Epic's version.
Note that DirectX SDK (Jun 10) needs to be installed before launching Unreal Editor in `Engine\Binaries\Win64\UE4Editor.exe`.

## Prebuilt Plugin

Prebuilt versions of the plugin are left regularly in ` \\cognitionfs\PUBLIC\Unreal\UXTools`. The file name reflects the plugin version and the internal UE version it was built for. For example `2020-01-03_UXTools_0ffb6e01_UE_e3b93ec63.7z` contains plugin version `0ffb6e01` (commit number in this repository) built for internal UE version `e3b93ec63` (commit number in internal UE repository). 

To use just unzip the file inside your game's Plugin folder and enable the plugin via the Plugin menu in the editor.

If your engine version doesn't match exactly the one the plugin was built for, the editor will try to rebuild the plugin automatically when loading the project. If the project is blueprint-only the rebuild will fail because the project is not setup for building code.

Because of this and the inability to package for HoloLens a blueprint-only project that uses external code plugins, we recommend that you always use the MRU plugin with code projects.

### Example Maps

Since `ed617442`, for each plugin version there is a corresponding zip with a game with example maps, e.g. `2020-01-16_Examples_ed617442.7z`. To use the examples, unzip them to a local folder and then unzip the corresponding plugin version to the Plugins folder inside the examples folder.

## Working From Sources

### Clone this repository

```
git clone --recurse-submodules https://MRDevPlat@dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-UE <repo_dir>
```
`--recurse-submodules` is required in order to bring in the UXT-Native repository we depend on.

### Setup symlinks

Run the `Setup.bat` script located in the repo root to create the symlinks required to use submodule sources in plugin modules.

### Switch engine version for the UXToolsGame project

Right click on `UXToolsGame/UXToolsGame.uproject`, select _Switch Unreal Engine version..._ and choose the internal engine version you obtained before. This will generate the Visual Studio solution for the project.
> [!NOTE]
> If you don't have the context menu for switching the version of Unreal Engine make sure you've got at least one official version of Unreal Engine installed on your machine.
		
		
## Software dependencies

We depend on the [Native UX Tools](https://dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-Native) project and our [internal version of Unreal Engine](https://microsoft.visualstudio.com/Analog/_git/analog.internal.unrealengine?path=%2F&version=GB423_release).

## Latest releases

Not released yet.

## API references

No available yet.

# Build and Test

1. After getting latest, switch engine version in the game project to make sure the solution is up to date.
2. Open solution, select Development Editor configuration, Win64 platform and set UXToolsGame as startup project.
3. Press F5 to build and run the project. After no more than 5 minutes the editor should open with one of the example maps loaded.

## Troubleshooting

### Build error caused by a failure to find a file under the Native folder

For example: 

1>C:/Code/MRU-UE/UXToolsGame/Plugins/UXTools/Source/UXTools/Private/PressableButtonComponent.cpp(4): fatal error C1083: Cannot open include file: 'Native/PressableButton.h': No such file or directory

This is caused by an invalid symbolic link in <repo_dir>/UXToolsGame/Plugins/UXTools/Source/UXTools/Private/Native. Try recreating it with mklink /D or copying its contents directly.

# Contribute

See UXT-Unreal's [contribution guidelines](Docs/Contributing/ContributionGuidelines.md) for information on how to contribute.