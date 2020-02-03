# Documentation guidelines

This document outlines the documentation guidelines and standards for Mixed Reality UX Tools for Unreal Engine (UXT-Unreal). Its purpose is to get you started quickly by giving an introduction about the technical aspects that you need to know, to point out common pitfalls and to describe the writing style that you should try to follow.

The page itself is supposed to serve as an example, therefore it uses the intended style and the most common markup features of the documentation.

Herein you will find some general style advice as well as the standards for the following forms of UXT-Unreal documentation:

+ [When you are finished with a page](#when-you-are-finished-with-a-page)
+ [How-to documentation](#how-to-documentation)
+ [Design documentation](#design-documentation)
+ [Tools for editing MarkDown](#tools-for-editing-markdown)

---

## Functionality and markup

This section describes frequently needed features. To see how they work, look at the source code of the page.

1. Numbered lists
   1. Nested numbered lists with at least 3 leading blank spaces
   1. The actual number in code is irrelevant; parsing will take care of setting the correct item number.

* Bullet point lists
  * Nested bullet point lists
* Text in **bold** with \*\*double asterisk\*\*
* _italic_ *text* with \_underscore\_ or \*single asterisk\*
* Text `highlighted as code` within a sentence \`using backquotes\`
* Links to docs pages [UXT-Unreal documentation guidelines](DocumentationGuidelines.md)
* Links to [anchors within a page](#style); anchors are formed by replacing spaces with dashes, and converting to lowercase

For code samples we use the blocks with three backticks \`\`\` and specify *cpp* as the language for syntax highlighting:

```c++
int SampleFunction(int i)
{
   return i + 42;
}
```

When mentioning code within a sentence `use a single backtick`.

### TODOs

Avoid using TODOs in docs, as over time these TODOs (like code TODOs) tend to accumulate
and information about how they should be updated and why gets lost.

If it is absolutely necessary to add a TODO, follow these steps:

1. File a new issue in the [MRDevPlat Azure Devops Project](https://dev.azure.com/MRDevPlat/DevPlat/_workitems/) describing the context behind the TODO, and provide enough background that another contributor would be able to understand and then address the
   TODO.
2. Reference the issue URL in the todo in the docs.

\<\!-- TODO(https://dev.azure.com/MRDevPlat/DevPlat/_workitems/edit/ISSUE_NUMBER_HERE): A brief blurb on the issue --\>

### Highlighted sections

To highlight specific points to the reader, use *> [!NOTE]* , *> [!WARNING]* , and *> [!IMPORTANT]* to produce the following styles. It is recommended to use notes for general points and warning/important points only for special relevant cases.

> [!NOTE]
> Example of a note

> [!WARNING]
> Example of a warning

> [!IMPORTANT]
> Example of an important comment

## Page layout

### Introduction

The part right after the main chapter title should serve as a short introduction what the chapter is about. Do not make this too long, instead add sub headlines. These allow to link to sections and can be saved as bookmarks.

### Main body

Use two-level and three-level headlines to structure the rest.

**Mini Sections**

Use a bold line of text for blocks that should stand out. We might replace this by four-level headlines at some point.

### 'See also' section

Most pages should end with a chapter called *See also*. This chapter is simply a bullet pointed list of links to pages related to this topic. These links may also appear within the page text where appropriate, but this is not required. Similarly, the page text may contain links to pages that are not related to the main topic, these should not be included in the *See also* list. See [this page's ''See also'' chapter](#see-also) as an example for the choice of links.

## Style

### Writing style

General rule of thumb: Try to **sound professional**. That usually means to avoid a 'conversational tone'. Also try to avoid hyperbole and sensationalism.

1. Don't try to be (overly) funny.
2. Never write 'I'
3. Avoid 'we'. This can usually be rephrased easily, sometimes you can use 'UXT-Unreal' instead. Example: "we support this feature" -> "UXT-Unreal supports this feature" or "the following features are supported ...".
4. Similarly, try to avoid 'you'. Example: "With this simple change your shader becomes configurable!" -> "Shaders can be made configurable with little effort."
5. Do not use 'sloppy phrases'.
6. Avoid sounding overly excited, we do not need to sell anything.
7. Similarly, avoid being overly dramatic. Exclamation marks are rarely needed.

### Capitalization

* Use **Sentence case for headlines**. Ie. capitalize the first letter and names, but nothing else.
* Use regular English for everything else. That means **do not capitalize arbitrary words**, even if they hold a special meaning in that context. Prefer *italic text*, if you really want to highlight certain words, [see below](#emphasis-and-highlighting).
* When a link is embedded in a sentence (which is the preferred method), the standard chapter name always uses capital letters, thus breaking the rule of no arbitrary capitalization inside text. Therefore use a custom link name to fix the capitalization. As an example, here is a link to these guidelines: [documentation guidelines](DocumentationGuidelines.md).
* Do capitalize names, such as *Unreal Engine*.

### Emphasis and highlighting

There are two ways to emphasize or highlight words, making them bold or making them italic. The effect of bold text is that **bold text sticks out** and therefore can easily be noticed while skimming a piece of text or even just scrolling over a page. Bold is great to highlight phrases that people should remember. However, **use bold text rarely**, because it is generally distracting.

Often one wants to either 'group' something that belongs logically together or highlight a specific term, because it has a special meaning. Such things do not need to stand out of the overall text. Use italic text as a *lightweight method* to highlight something.

Similarly, when a filename, a path or a menu-entry is mentioned in text, prefer to make it italic to logically group it, without being distracting.

In general, try to **avoid unnecessary text highlighting**. Special terms can be highlighted once to make the reader aware, do not repeat such highlighting throughout the text, when it serves no purpose anymore and only distracts.

### Mentioning menu entries

When mentioning a menu entry that a user should click, the current convention is:
*Window > Viewports > Viewport 2*

### Links

Insert as many useful links to other pages as possible, but each link only once. Assume a reader clicks on every link in your page, and think about how annoying it would be, if the same page opens 20 times.

Prefer links embedded in a sentence:

* BAD: Guidelines are useful. See [this chapter](DocumentationGuidelines.md) for details.
* GOOD: [Guidelines](DocumentationGuidelines.md) are useful.

Avoid external links, they can become outdated or contain copyrighted content.

When you add a link, consider whether it should also be listed in the [See also](#see-also) section. Similarly, check whether a link to your new page should be added to the linked-to page.

### Images / screenshots

**Use screenshots sparingly.** Maintaining images in documentation is a lot of work, small UI changes can make a lot of screenshots outdated. The following rules will reduce maintenance effort:

1. Do not use screenshots for things that can be described in text. Especially, **never screenshot a property grid** for the sole purpose of showing property names and values.
2. Do not include things in a screenshot that are irrelevant to what is shown. For instance, when a rendering effect is shown, make a screenshot of the viewport, but exclude any UI around it. When you have to show some UI, try to move windows around such that only that important part is in the image.
3. When you do screenshot UI, only show the important parts. For example, when talking about buttons in a toolbar, you can make a small image that shows the important toolbar buttons, but exclude everything around it.
4. Only use images that are easy to reproduce. That means do not paint markers or highlights into screenshots. First, there are no consistent rules how these should look, anyway. Second, reproducing such a screenshot is additional effort. Instead, describe the important parts in text. There are exceptions to this rule, but they are rare.
5. Obviously, it is much more effort to recreate an animated GIF. If you make one, expect to be responsible to recreate it for the rest of your life, or expect people to throw it out, if they don't want to spend that time.
6. Keep the number of images in an article low. Often a good method is to make one overall screenshot of some tool, that shows everything, and then describe the rest in text. This makes it easy to replace the screenshot when necessary.

Some other aspects:

* Default image width is 500 pixels, as this displays well on most monitors. Try not to deviate too much from it. 800 pixels width should be the maximum.
* Use PNGs for screenshots of UI.
* Use PNGs or JPGs for 3D viewport screenshots. Prefer quality over compression ratio.

### List of component properties

When documenting a list of properties, use bold text to highlight the property name, then line breaks and regular text to describe them. Do not use sub-chapters or bullet point lists.

Also, don't forget to finish all sentences with a period.

## When you are finished with a page

1. Make sure you followed the guidelines in this document.
1. Browse the document structure and see if your new document could be mentioned under the [See also](#see-also) section of other pages.
1. If available, have someone with knowledge of the topic proof-read the page for technical correctness.
1. Have someone proof-read your page for style and formatting. This can be someone unfamiliar with the topic, which is also a good idea to get feedback about how understandable the documentation is.

## How-to documentation

Many users of UXT-Unreal may not need to use the API documentation. These users will take advantage of our pre-made, reusable blueprints and C++ to create their experiences.

Each feature area will contain one or more markdown (.md) files that describe at a fairly high level, what is provided. Depending on the size and/or complexity of a given feature area, there may be a need for additional files, up to one per feature provided.

When a feature is added (or the usage is changed), overview documentation must be provided.

As part of this documentation, how-to sections, including illustrations, should be provided to assist customers new to a feature or concept in getting started.

## Design documentation

Mixed Reality provides an opportunity to create entirely new worlds. Part of this is likely to involve the creation of custom assets for use with UXT-Unreal. To make this as friction free as possible for customers, components should provide design documentation describing any formatting or other requirements for art assets.

Some examples where design documentation can be helpful:

- Cursor models
- Spatial mapping visualizations
- Sound effect files

This type of documentation is **strongly** recommended, and **may** be requested as part of a pull request review.

This may or may not be different from the design recommendation on the [MS Developer site](https://docs.microsoft.com/windows/mixed-reality/design)

## Tools for editing MarkDown

[Visual Studio Code](https://code.visualstudio.com/) is a great tool for editing markdown files that are part of UXT-Unreal's documentation.

When writing documentation, installing the following two extensions is also highly recommended:

- Docs Markdown Extension for Visual Studio Code - Use Alt+M to bring up a menu of docs authoring options.

- Code Spell Checker - misspelled words will be underlined; right-click on a misspelled word to change it or save it to the dictionary.

Both of these come packaged in the Microsoft published Docs Authoring Pack.