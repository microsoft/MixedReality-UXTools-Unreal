# Contribution guidelines

Mixed Reality UX Tools for Unreal Engine (UXT-Unreal) welcomes contributions. All changes, be they small or large, need to adhere to the [UXT-Unreal coding standards](CodingGuidelines.md), so please ensure you are familiar with these while developing to avoid delays when the change is being reviewed.

## Reporting bugs or proposing a new feature

You can report a bug or propose a new feature by creating a task in the [MRDevPlat Azure Devops Project](https://dev.azure.com/MRDevPlat/DevPlat/_workitems/). From here a new work item can be created:

- **Bug report** - When reporting a bug select _Bug_ when creating a work item. Please provide a detailed description or repro steps.
- **Feature request** - When requesting a new feature select _Task_ when creating a work item. Please provide a detailed description, but also document the customer benefit or problem to be solved.

Please ensure that all new work items are created in the _DevPlat\UXT-Unreal_ area path.

## Feature contribution process

The following process ensures that new work complies with UXT-Unreal's specification and standards. In order to contribute to UXT-Unreal, please ensure that the following steps have been completed:

### Open a new task

See [Reporting bugs or proposing a new feature](#reporting-bugs-or-proposing-a-new-feature) above for how to create a new bug or feature request. All contributions require a work item, so please ensure that a work item has been created before creating a pull request.

> [!NOTE]
> If you wish to work on something that already exists on our backlog, you can use that work item as your proposal. Be sure to also comment on the task, or assign it to yourself, notifying maintainers that you're working towards completing it.

### Implement the bug fix or feature

Instructions for acquiring UXT-Unreal sources can be found [here](../../README.md#working-from-sources) and instructions for building and troubleshooting can be found [here](../../README.md#build-and-test). When fixing a bug or adding a new feature, please adhere to our coding guidelines. Ensure that you are familiar with these guidelines while developing as this will avoid delays while changes are being reviewed. Please ensure that sufficient tests are added to verify the new changes. Also, please document any changes where it is necessary. Please refer to the [documentation guidelines](DocumentationGuidelines.md) for writing documentation.

### Create a pull request
Push these new changes to a branch in the form `user/alias/branch-name`. A pull request can then be created and you can request review from members of the UXT-Unreal team. In order to complete a pull request it will need approval from at least one person and it will need to be linked to the work item for the new changes. As well as this a PR validation pipeline will run for all new pull request. Please ensure that this pipeline succeeds for before completing a pull request and fix the test or build failures that it runs into.

In general, following these suggestions to make the PR is suggested in order to make the process quicker and easier for everyone involved:
- **Keep pull requests small** - Smaller PRs are reviewed more quickly and thoroughly, are less likely to introduce bugs, easier to roll back, and easier to merge. Pull requests should be small enough that an engineer could review it in under 30 minutes. Try to make a minimal change that addresses just one thing. If a large PR is necessary, split it into several PRs and potentially target a feature branch rather than `master`.
- **Pull request descriptions should clearly and completely describe changes** - Clear and complete descriptions of pull requests ensure reviewers understand what they are reviewing. If possible, add an image/gif of the feature that's being changed. Another suggestion is to have a gif of Before and After. A tool we recommend for generating gifs from screen captures is [ScreenToGif](https://www.screentogif.com/).
- **Tests/Documentation should be added in the same PR as you fix/feature** - Tests are the best way to ensure changes do not regress existing code. Every feature and bug fix should have tests associated with it. If you complete a PR without adding tests, please keep the work item for the feature open and comment that the changes still need tests.

  Most developers look first at documentation, not code, when understanding how to use a feature. Ensuring documentation is up to date makes it much easier for people to consume and rely upon UXT-Unreal. Documentation should always be bundled with the related pull to ensure items remain up-to-date and consistent. It also helps the reviewer understand what new changes do and how they can be used.