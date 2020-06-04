# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.Synopsis
Mirrors selected refs (tags/branches) to a remote repo.
E.g. MirrorToRemote https://github.com/microsoft/MixedReality-UXTools-Unreal public/0.8.x
#>
param(
    [Parameter(Mandatory=$true, Position=0)]
    [string]$RemoteUrl,

    [Parameter(Mandatory=$true, ValueFromRemainingArguments=$true)]
    [string[]]$RefsToPush
)

foreach ($ref in $RefsToPush) {
    Write-Host "Pushing ${ref} to $RemoteUrl"
    git push $RemoteUrl "refs/remotes/origin/${ref}:refs/heads/${ref}"
}
