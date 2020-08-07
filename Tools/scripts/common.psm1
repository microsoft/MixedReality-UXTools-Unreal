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

<#
.SYNOPSIS
    Returns true if the given file is a markdown document and
    false otherwise. Uses the extension of the file, not the actual
    content to determine this.
#>
function IsMarkdownFile {
    [CmdletBinding()]
    param(
        [string]$Filename
    )
    process {
        [IO.Path]::GetExtension($Filename).ToLower() -eq ".md"
    }
}
