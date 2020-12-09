---
title: Installation Guide
description: Guide for installating UX Tools in a new Unreal project.
author: hferrone
ms.author: v-hferrone
ms.date: 09/8/2020
ms.localizationpriority: high
keywords: Unreal, Unreal Engine, UE4, HoloLens, HoloLens 2, Mixed Reality, development, MRTK, UXT, UX Tools, Graphics, rendering, materials
---

# Installation Guide

## Prerequisites

Before getting started with UX Tools, make sure that you have [installed the required tools](https://docs.microsoft.com/windows/mixed-reality/install-the-tools?tabs=unreal).

## Getting the prebuilt plugin

If you just want to add UXT to your game project, the quickest way is through the packaged plugin provided in the release page:
1. Download the packaged plugin zip from the [latest release](https://github.com/microsoft/MixedReality-UXTools-Unreal/releases) page (e.g. _UXTools.0.11.0.zip_).
1. Unzip the file directly into your project's _Plugins_ folder. The _Plugins_ folder should be located at the root of your project, where the _.uproject_ file is. Create it if it doesn't exist.
1. Make sure your game project is a code one, as opposed to blueprint-only, if you are planning to package it for HoloLens. Otherwise UE will fail to package it because it can't build the plugin sources.
1. Open your project and enable the _UX Tools_ plugin in the plugins menu. 

You now have access to all of the plugin features. The first thing you want to do is probably add a hand interaction actor per hand to your map or pawn so you can use your hands to drive the controls and behaviors provided in UXT.

## Next steps

* [HoloLens 2 tutorial series](https://docs.microsoft.com/windows/mixed-reality/unreal-uxt-ch1)
* [Unreal development journey](https://docs.microsoft.com/windows/mixed-reality/unreal-development-overview)

