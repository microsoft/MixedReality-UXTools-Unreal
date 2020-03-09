# Developer portal generation guide

The UXT documentation build script uses [code2yaml]("https://github.com/docascode/code2yaml") and [docfx](https://dotnet.github.io/docfx/index.html) to generate html documentation out of C++ code and comments and .md files in the UXT repository. 

Docfx supports DFM Docfx Flavored Markdown which includes GFM Github Flavored Markdown. The full documentation and feature list can be found [here](https://dotnet.github.io/docfx/tutorial/docfx.exe_user_manual.html)

The build script is not only converting but also checking all used local links in the documentation. If a path can't be resolved it won't be converted into its html equivalent. Therefore it's important to only use relative paths when referring to other local files.

## Building UXT docs locally

Before executing the UXT documentation build script in Tools/DocGen/generateDocs.ps1 to create a local version of the developer documentation, code2yaml and docfx need to be installed.

### Setup

* get the latest version of [docfx](https://dotnet.github.io/docfx/index.html) and [code2yaml]("https://github.com/docascode/code2yaml")
* extract the files in a folder on your computer
* add the folder to your PATH in your environment variables

### Generation

Execute powershell script Tools/DocGen/generateDocs.ps1 to generate a local version of the UXT docs in DocGen/doc and output any documentation build or link errors. 
This script can be executed with an optional -serve to host the website on localhost port 8080 and display the result in the machines default web browser. 

Please make sure whenever there's a change on any of the documentation files or API to run this script and make sure there's no errors or warnings that will break any existing links.

## Linking in .md documentation files

Docfx is translating and validating all relative local links on generation, there's no special syntax required. Referring to another documentation article should always be done by referring to the corresponding .md file, never the auto generated .html file. Please note that all links to local files need to be relative to the file you're modifying.

Linking to the API documentation can be done by using [cross references](https://dotnet.github.io/docfx/tutorial/links_and_cross_references.html) based on UIDs. Code2Yaml automatically generates UIDs for all API docs by mangling the signature.

Example:

This links to the [Pressable Button Component](xref:_u_uxt_pressable_button_component) API Documentation
as well as this short version: <xref:_u_uxt_pressable_button_component>

```md
This links to the [Pressable Button Component](xref:_u_uxt_pressable_button_component) API Documentation
as well as this short version: <xref:_u_uxt_pressable_button_component>
```

## Enumerating available xrefs

Xref syntax can be difficult to remember - it's possible to enumerate all of the available xref IDs by first running the uxt doc build script Tools/DocGen/generateDocs.ps1 locally.

This will generate an xrefmap.yml file, which will be located in the root of the generated doc folder (doc/xrefmap.yml).

For example, in order to link to the method GetPointerPose in IUxtHandTracker, the syntax is fairly arcane:

```yml
- uid: _i_uxt_hand_tracker.GetPointerPose(EControllerHand,FQuat &,FVector &)
  name: GetPointerPose(EControllerHand Hand, FQuat &OutOrientation, FVector &OutPosition)
  href: api/_i_uxt_hand_tracker.html#_i_uxt_hand_tracker_GetPointerPose_EControllerHand_FQuat___FVector___
  fullName: virtual bool IUxtHandTracker::GetPointerPose(EControllerHand Hand, FQuat &OutOrientation, FVector &OutPosition)
  nameWithType: IUxtHandTracker::GetPointerPose(EControllerHand Hand, FQuat &OutOrientation, FVector &OutPosition)
```

It's easy, however, to search for the name and then use the entire **uid field** as the xref.
In this example, the xref would look like: <xref:_i_uxt_hand_tracker.GetPointerPose(EControllerHand,FQuat &,FVector &)>

## Adding new .md files to developer docs

Docfx will pick up any .md files in folders that are added as content files in the build section of the docfx.json and generate html files out of them. For new folders a corresponding entry in the build file needs to be added.

### Navigation entries

To determine the entries of the navigation in the developer docs docfx uses toc.yml/toc.md - table of content files.
The toc file in the root of the project defines entries in the top navigation bar whereas the toc.yml files in the subfolders of the repo define subtopics in the sidebar navigation.
toc.yml files can be used for structuring and there can be any amount of those files. For more info about defining entries for toc.yml check the [docfx documentation entry on toc](https://dotnet.github.io/docfx/tutorial/intro_toc.html).

## Resource files

There are some files like images, videos or PDFs that the documentation can refer to but are not converted by docfx. For those files there's a resource section in the docfx.json. Files in that section will only be copied over without performing any conversion on them.

Currently there's a definition for the following resource types:

| ResourceType | Path |
| --- | --- |
| Images | Docs/Images/ |

## Releasing a new version - Coming soon (github move)

Multiple versions of developer docs are supported and can be switched by the version drop down in the top menu bar. When releasing a new version perform the following steps to have that version on the developer docs page.

1. Optional: Adjusting your docfx.json  
Depending on whether you want to have the "Improve this doc" to point to a specific version of the github repo the following entry to the globalMetaData section in the docfx.json file needs to be adjusted before executing the build script:

    ```json
    "_gitContribute": {
        "repo": "https://MRDevPlat@dev.azure.com/MRDevPlat/DevPlat/_git/MixedRealityUtils-UE",
        "branch": "master"
    }
    ```

    If this is not set up docfx will default to the branch and repo of the current folder the build script is called from.

1. Create docs via build script Tools/DocGen/generateDocs.ps1
1. Create a folder with the name of your version in the version folder of the gh-pages branch and copy the contents of the generated doc folder into that folder
1. Add the new version number into the versionArray in web/version.js
1. Push the modified version.js to master branch and the changes in gh-pages branch

CI will pick up the changes done to the version.js file and update the version dropdown automatically.

### Supporting development branches on CI - Coming soon (github move)

The versioning system can also be used for showing doc versions from other dev branches that are built by CI. When setting up CI for one of those branches make sure your powershell script on CI copies the contents of the generated docfx output into a version folder named after the new branch and add the corresponding version entry into the web/version.js file.

## Good practices for developers

* Use **relative paths** whenever referring to UXT internal pages
* Use **cross references** for linking to any UXT API page by using the **mangled UID**
* Use the indicated folders in this doc for resource files
* **Run the build script locally** and check for warnings in the output whenever existing APIs are modified or documentation pages are updated
* Coming soon (github move): Watch out for docfx **warnings on CI** after completing and merging your PR into one of the official UXT branches

## Common errors when generating docs

* toc.yml errors: usually happens when an .md file gets moved/renamed or removed but the table of content file (toc.yml) pointing to that file wasn't updated accordingly. On the website this will result in a broken link on our top level or side navigation

## See also

* [UXT documentation guide](DocumentationGuidelines.md)
* [DocFX](https://dotnet.github.io/docfx/index.html)
