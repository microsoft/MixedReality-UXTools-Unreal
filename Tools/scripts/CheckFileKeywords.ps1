# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Check text file contents disallowed patterns.
#>

[CmdletBinding()]
Param(
    [Parameter(Mandatory=$true)]
    [string]$Path
)

Set-StrictMode -Version 3

Push-Location $PSScriptRoot

$disallowed = (git show origin/private:disallowed.txt)
if (!$disallowed)
{
	Write-Host "Unable to read disallowed text"
	exit 1
}

Pop-Location

$bad = @()
(Get-Content -Path $Path -Encoding UTF8) `
    | Select-String "\b$disallowed\b" -AllMatches `
    | ForEach-Object { $_.matches } `
    | ForEach-Object { $bad += $_.Value }

if ($bad)
{
    Write-Host -ForegroundColor Red "ERROR: found disallowed keywords:"
    $bad | Sort-Object | Get-Unique | ForEach-Object { Write-Host "    $_" }
	exit 1
}

Write-Host "Keyword check completed successfully on $Path."
exit 0
