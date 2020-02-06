# Coding guidelines

This document outlines coding principles and conventions to follow when contributing to MRTK-Unreal, focusing on the aspects that are particular to the Unreal + MRTK scenario. 

For a reference on general good practices we recommend the [Cpp Core Guidelines](https://github.com/isocpp/CppCoreGuidelines).

---

## C++ coding conventions

We follow Unreal Engine's [coding standard](https://docs.unrealengine.com/en-US/Programming/Development/CodingStandard/index.html) unless overriden by any of the conventions below.

### License information headers

All Microsoft employees contributing new files should add the following standard License header at the top of any new source files, exactly as shown below:

```c++
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
```

### Function / method summary headers

All public classes, structs, enums, functions, data members posted to MRTK-Unreal should be described as to its purpose and use, following UE's [conventions](https://docs.unrealengine.com/en-US/Programming/Development/CodingStandard/#exampleformatting) for comments.

### Adding new actors or components

When adding a new actor or component, ensure its `UCLASS` macro contains `ClassGroup = UXTools` so it appears correctly categorized in the editor.

```c++
UCLASS(ClassGroup = UXTools, meta=(BlueprintSpawnableComponent))
class UXTOOLS_API UHandJointAttachmentComponent : public UActorComponent
```

### Class layout

Declare first public members, followed by protected and then private. Within each access level organize declarations in this order:

- types: classes, enums and aliases (using)
- constructors, assignments, destructor and functions
- data

For example:

```c++
class UFoo : public UObject
{
public:

    using FPublicTypeAlias = int;

    UFoo();
    ~UFoo();

    void DoSomething();

    bool bData;

private:

    using FPrivateTypeAlias = int;

    ...
};
```

### Order enums for appropriate extension

If an enum is likely to be extended in the future, order defaults at the top so indexes are not affected with new additions.

#### Don't

```c++
enum class ESDKType
{
    WindowsMR,
    OpenVR,
    OpenXR,
    None, // <- default value not at start
    Other // <- anonymous value left to end of enum
}
```

#### Do

```c++
enum class ESDKType
{
    None,
    Other,    
    WindowsMR,
    OpenVR,
    OpenXR
}
```

## Other guidelines

### Support configuring actors and components both in editor and at run-time

MRTK-Unreal supports a diverse set of users – people who prefer to configure their level in the editor and people who need to instantiate and configure their level at run-time.

All your code should work when BOTH adding an actor/component in a saved level, and when instantiating that actor/component in code. Tests should include a test case for both.

### Play-in-editor is your first and primary target platform

Play-In-Editor is the fastest way to iterate in Unreal. Providing ways for our customers to iterate quickly allows them to both develop solutions more quickly and try out more ideas. In other words, maximizing the speed of iteration empowers our customers to achieve more.

Make everything work in editor, then make it work on any other platform. Keep it working in the editor. It is easy to add a new platform to Play-In-Editor. It is very difficult to get Play-In-Editor working if your app only works on a device.

### Add new non-private function and data members and UProperties with care

Every time you add a non-private member, including protected ones, it becomes part of MRTK-Unreal’s public API surface. UProperties, even private ones, can be exposed in the editor and become part of the public API surface too. Other people might use that public method, access you property from a blueprint, and take a dependency on it.

New non-private members should be carefully examined. Any such member will need to be maintained in the future. Remember that if the type of a non-private data member or UProperty changes or gets removed, that could break other people. The member will need to first be deprecated for a release, and code to migrate changes for people that have taken dependencies would need to be provided.

### Prioritize writing tests

MRTK-Unreal is a community project, modified by a diverse range of contributors. These contributors may not know the details of your bug fix / feature, and accidentally break your feature. UXT-Unreal runs [continuous integration tests](https://dev.azure.com/MRDevPlat/DevPlat/_build) before completing every pull request. Changes that break tests should not be checked in. Therefore, tests are the best way to ensure that other people do not break your feature.

When you fix a bug, write a test to ensure it does not regress in the future. If adding a feature, write tests that verify your feature works.
