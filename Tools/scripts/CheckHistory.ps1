# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

<#
.SYNOPSIS
Check the complete history of a git reference for any disallowed patterns.

.NOTES
Defaults to the current checkout (aka HEAD) if a ref is not specified.
#>

[CmdletBinding()]
Param(
    # Ref to check - otherwise the current checked out branch
    [string]$Ref = "HEAD"
)

Set-StrictMode -Version 3

$disallowed=(git show origin/private:disallowed.txt)
if(!$disallowed) {
	Write-Host "Unable to read disallowed text"
	exit 1
}

$bad=(git log -p $Ref) | Select-String "\b$disallowed\b" | Out-String
if($LASTEXITCODE) {
	Write-Host "Failed to run git log"
	exit 1
}
if($bad) {
	Write-Host "Error: found disallowed words in the history"
	Write-Host $bad
	exit 1
}

Write-Host "History check completed successfully on $Ref."
exit 0
