# Introduction 

Mixed Reality Utils for Unreal Engine is a collection of C++ classes, Blueprints and example assets created to help in the implementation of features commonly needed when developing mixed reality applications, like pressable buttons or direct manipulation.

This currently takes the form of a game plugin found in [MRU_Game/Plugins/MixedRealityUtils](https://dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-UE?path=%2FMRU_Game%2FPlugins%2FMixedRealityUtils&version=GBmaster).

# Getting Started

Whether using the prebuilt plugin or building it yourself, you'll first need to obtain our internal version of UE. Vanilla UE 4.24 has HoloLens 2 support in beta but the plugin depends on internal changes that have not been released yet. You can find the latest binaries of our internal UE in  `\\cognitionfs\PUBLIC\Unreal\builds` (Redmond) or `\\havokfs\SelfHost\Sydney\UE4` (Dublin). Unzip it to a local folder and use it instead of Epic's version.

## Prebuilt Plugin

Prebuilt versions of the plugin are left regularly in ` \\cognitionfs\PUBLIC\Unreal\MixedRealityUtils` . 
To use one just unzip the file inside your game's Plugin folder and enable the plugin via the Plugin menu in the editor. 

## Working From Sources

### Clone this repository

```
git clone https://MRDevPlat@dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-UE <repo_dir>
```

### Setup submodules

We setup submodules manually instead of cloning with --recurse-submodules because we don't need to bring our submodules' dependencies which can take a bit to download.

```
cd <repo_dir>
git submodule update --init
```

### Enable symlinks and recreate them

Symlinks are required in order to include code from submodules in UE modules. If you don't have rights to create symlinks in your system, you can manually copy their contents instead.

```
git config core.symlinks true
cd MRU-UE\MRU_Game\Plugins\MixedRealityUtils\Source\MixedRealityUtils\Private
del Native
mklink /D Native ..\..\..\..\..\..\External\MixedRealityUtils-Native\libs\UX\src
```

### Switch engine version for the MRU_Game project

Right click on `MRU_Game/MRU_Game.uproject`, select _Switch Unreal Engine version..._ and choose the internal engine version you obtained before. This will generate the Visual Studio solution for the project.
		
		
## Software dependencies

We depend on the [Native Mixed Reality Utils](https://dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-Native) project and our [internal version of Unreal Engine](https://microsoft.visualstudio.com/Analog/_git/analog.internal.unrealengine?path=%2F&version=GB423_release).

## Latest releases

Not released yet.

## API references

No available yet.

# Build and Test

1. After getting latest, switch engine version in the game project to make sure the solution is up to date.
2. Open solution, select Development Editor configuration, Win64 platform and set MRU_Game as startup project.
3. Press F5 to build and run the project. After no more than 5 minutes the editor should open with one of the example maps loaded.

## Troubleshooting

### Build error caused by a failure to find a file under the Native folder

For example: 

1>C:/Code/MRU-UE/MRU_Game/Plugins/MixedRealityUtils/Source/MixedRealityUtils/Private/PressableButtonComponent.cpp(4): fatal error C1083: Cannot open include file: 'Native/PressableButton.h': No such file or directory

This is caused by an invalid symbolic link in <repo_dir>/MRU_Game/Plugins/MixedRealityUtils/Source/MixedRealityUtils/Private/Native. Try recreating it with mklink /D or copying its contents directly.

# Contribute

TODO: Explain how other users and developers can contribute to make your code better. 