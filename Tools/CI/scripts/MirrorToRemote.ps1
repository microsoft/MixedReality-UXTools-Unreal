# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Mirrors a single branch to a remote repo. Typically used on a successful build of a public branch.

.NOTES

#>

[CmdletBinding()]
Param(
    # RemoteUrl will often include a PAT for auth.
    [Parameter(Mandatory=$true, Position=0)]
    [string]$RemoteUrl,

    # CI usually has a detached HEAD so it's handy to pass the branch name in.
    [Parameter(Mandatory=$true)]
    [string]$RefToPush,

    # To allow this script to exist in several physical repositories with different
    # mirroring rules, we allow the pipeline to allow only certain branch mirrors.
    [Parameter(Mandatory=$true)]
    [string[]]$RefsToAllowRegex
)

Set-StrictMode -Version 3

# Check vs the allow list
$AllowedRef = $RefToPush | Select-String -Pattern $RefsToAllowRegex
if( -not $AllowedRef ) {
    Write-Host("Skipping $RefToPush based on allow list $RefsToAllowRegex")
    exit 0
}
Write-Host("Branch $RefToPush is allowed by allow list '$RefsToAllowRegex'");

& $env:UXTSourceDir\Tools\scripts\CheckHistory.ps1 ${RefToPush}
if ($LastExitCode) { throw $LastExitCode }

git push $RemoteUrl $RefToPush
