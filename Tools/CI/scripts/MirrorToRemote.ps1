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
    # Example: refs/heads/master
    [Parameter(Mandatory=$true)]
    [string]$RefToPush,

    # To allow this script to exist in several physical repositories with different
    # mirroring rules, we allow the pipeline to allow only certain branch mirrors.
    # Expected a comma-separated list of regular expressions matching short branch names (e.g. 'master')
    # Example: master,public/0\.8\.x
    [Parameter(Mandatory=$true)]
    [string[]]$RefsToAllowRegex
)

Set-StrictMode -Version 3

# Check vs the allow list
$UpdatedRefsToAllowRegex = $RefsToAllowRegex | ForEach-Object { "^(refs/heads/){0,1}$_$" }
$AllowedRef = $RefToPush | Select-String -Pattern $UpdatedRefsToAllowRegex
if( -not $AllowedRef ) {
    Write-Host("Skipping $RefToPush based on allow list $RefsToAllowRegex")
    exit 0
}
Write-Host("Branch $RefToPush is allowed by allow list '$RefsToAllowRegex'");

$ShortRef = $RefToPush -replace "^(refs/heads/)",""
$FullRef = "refs/heads/$ShortRef"
$OriginSourceBranch = "remotes/origin/$ShortRef"

# Repo on the build agent is in "detached head" state and will not have local branches
Write-Host "Checking commit history on ${OriginSourceBranch}"
& "$env:UXTSourceDir\Tools\scripts\CheckHistory.ps1" -Ref "${OriginSourceBranch}"
if ($LastExitCode) { throw $LastExitCode }

Write-Host "Pushing ${OriginSourceBranch} to ${FullRef} on upstream"
git push $RemoteUrl "${OriginSourceBranch}:${FullRef}"
