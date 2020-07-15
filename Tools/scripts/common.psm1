<#
.SYNOPSIS
    Given the path to the list of raw git changes, returns an array of
    those changes rooted in the git root directory.
.DESCRIPTION
    For example, the raw git changes will contain lines like:

    Assets/File.cs

    This function will return a list of paths that look like (assuming
    that RepoRoot is C:\repo):

    C:\repo\Assets\File.cs
#>
function GetChangedFiles { 
    [CmdletBinding()]
    param(
        [string]$Filename,
        [string]$RepoRoot
    )
    process {
        $rawContent = Get-Content -Path $Filename
        $processedContent = @()
        foreach ($line in $rawContent) {
            $joinedPath = Join-Path -Path $RepoRoot -ChildPath $line
            $processedContent += $joinedPath
        }
        $processedContent
    }
}
