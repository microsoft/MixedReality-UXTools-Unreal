# Developer portal generation guide

UXT uses [docfx](https://dotnet.github.io/docfx/index.html) to generate html documentation out of triple slash comments in code and .md files in the UXT repository. Docfx documentation generation is automatically triggered by CI on completed PRs in the master branch.
The current state of the developer documentation can be found on the [UXT github.io page](https://microsoft.github.io/MixedRealityToolkit-Unity/)

Docfx supports DFM Docfx Flavored Markdown which includes GFM Github Flavored Markdown. The full documentation and feature list can be found [here](https://dotnet.github.io/docfx/tutorial/docfx.exe_user_manual.html)

Docfx is not only converting but also checking all used local links in the documentation. If a path can't be resolved it won't be converted into its html equivalent. Therefor it's important to only use relative paths when referring to other local files.

## Building docfx locally

The docfx build files in the UXT repo can be used to create a local version of the developer documentation in a doc/ subfolder in the root of the project.

### Setup

* get the latest version of [docfx](https://dotnet.github.io/docfx/index.html)
* extract the files in a folder on your computer
* add the folder to your PATH in your environment variables

### Generation

* open a powershell or cmd prompt in the root of the UXT project
* execute `docfx docfx.json` (optionally with the -f option to force a rebuild of doc files)
* execute `docfx serve doc` (optionally with -p *portnumber* if you don't want to use the 8888 default port)
* open a web browser with localhost:*portnumber*

Note that on executing the docfx command on the json build file docfx will show any broken links in the documentation as warning.
Please make sure whenever you perform changes on any of the documentation files or API to update all links pointing to these articles or code.

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
| Images | Documentation/Images/ |

## Releasing a new version

Multiple versions of developer docs are supported and can be switched by the version drop down in the top menu bar. If you're releasing a new version perform the following steps to have your version on the developer docs page.

1. Optional: Adjusting your docfx.json  
Depending on whether you want to have the "Improve this doc" to point to a specific version of the github repo you will have to add the following entry to the globalMetaData section in the docfx.json file before calling the docfx command:

    ```json
    "_gitContribute": {
        "repo": "UXT_GITHUB_URL_TODO",
        "branch": "master"
    }
    ```

    If you don't set this up docfx will default to the branch and repo of the current folder you're calling docfx from.

1. Create your docfx docs by calling docfx docfx.json in the root of the repo
1. Create a folder with the name of your version in the version folder of the gh-pages branch and copy the contents of the generated doc folder into that folder
1. Add your version number into the versionArray in web/version.js
1. Push the modified version.js to master branch and the changes in gh-pages branch

CI will pick up the changes done to the version.js file and update the version dropdown automatically.

### Supporting development branches on CI

The versioning system can also be used for showing doc versions from other dev branches that are built by CI. When setting up CI for one of those branches make sure your powershell script on CI copies the contents of the generated docfx output into a version folder named after your branch and add the corresponding version entry into the web/version.js file.


## See also

* [UXT documentation guide](DocumentationGuidelines.md)
* [DocFX](https://dotnet.github.io/docfx/index.html)
