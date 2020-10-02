<#
.SYNOPSIS
    Validates the docs to check for common patterns and usage that shouldn't be
    checked in.
.DESCRIPTION
    This currently checks:

    - That documentation doesn't contain fully resolved links (i.e. https://microsoft.github.io/MixedReality-UXTools-Unreal)
      and are instead relative links. Relative links can be resolved and validated by docfx, and fully resolved links cannot.

    Returns 0 if there are no issues, non-zero if there are.
.EXAMPLE
    .\validatedocs.ps1 -Directory c:\path\to\UXTools\Docs
#>
param(
    # The directory containing the docs to validate. This is the fallback if
    # ChangesFile doesn't exist or isn't valid, and as a result this is
    # always required.
    [Parameter(Mandatory=$true)]
    [string]$Directory,

    # The filename containing the list of files to scope the code validation
    # to. This is useful in pull request validation when there isn't a need
    # to check every single file in the repo for changes (i.e. only the list
    # of changed files)
    #
    # If ChangesFile doesn't exist (i.e. not specified, null, is specified but
    # the actual file doesn't exist), then this defaults to checking for everything
    # in the repo.
    [string]$ChangesFile,

    # The directory containing the repo root. Used in conjunction with ChangesFile
    [string]$RepoRoot
)

if (([string]::IsNullOrEmpty($ChangesFile) -ne $true) -and ([string]::IsNullOrEmpty($RepoRoot)))
{
    Write-Warning "-RepoRoot needs to be specified when using -ChangesFile."
    exit 1;
}

<#
.SYNOPSIS
    Checks if the given file at the given line contains a fully resolved documentation
    link. Returns true if found.
#>
function CheckDocLinks {
    [CmdletBinding()]
    param(
        [string]$FileName,
        [string[]]$FileContent,
        [int]$LineNumber
    )
    process {
        if ($FileContent[$LineNumber] -match "https://microsoft.github.io/MixedReality-UXTools-Unreal") {
            Write-Host "An non-relative doc link was found in $FileName at line $LineNumber "
            Write-Host "Avoid doc links containing https://microsoft.github.io/MixedReality-UXTools-Unreal "
            Write-Host "and use relative links instead."
            $true;
        }
        $false
    }
}

<#
.SYNOPSIS
    Checks if the given file at the given line contains an incorrect relative path
    (for example /Docs/...). These "absolute" paths will only resolve correct on
    github.com and not github.io
#>
function CheckIncorrectRelativePath {
    [CmdletBinding()]
    param(
        [string]$FileName,
        [string[]]$FileContent,
        [int]$LineNumber
    )
    process {
        if ($FileContent[$LineNumber] -match "]\(/") {
            Write-Host "An incorrect absolute path was found in $FileName at line $LineNumber "
            Write-Host "Avoid links of the form '/Docs/Folder' and use relative paths "
            Write-Host "'../Folder' instead."
            $true;
        }
        $false
    }
}

# A case-sensitve cache of all checked image paths, so that we can
# avoid hitting disk when checking for repeated images. Uses System.Collections.Hashtable
# directly because the default Dictionary in powershell is case-insensitive
$ResolvedImagePaths = New-Object System.Collections.Hashtable

<#
.SYNOPSIS
    Given a full path to a markdown file and a relative image path, checks to see if the
    image actually exists. Note that this does a case-sensitive checking, because
    some of our documentation endpoints are hosted on case-sensitive environments.
#>
function CheckImage {
    [CmdletBinding()]
    param(
        [string]$DocsRootDir,

        # An absolute path to the markdown file
        [string]$MarkdownFilePath,

        # The image path string (often relative) within the markdown file
        # e.g. "../Docs/Images/image.png"
        [string]$ImagePath
    )
    process {
        if (!$MarkdownFilePath -or !$ImagePath) {
            return $false
        }

        Push-Location
        Set-Location -Path $DocsRootDir

        # Resolves the given image based on the location of the markdown file location
        # For example, given "/path/to/Docs/subfolder/file.md"
        # and an image "../Images/image.png", resolves this to:
        # "/path/to/Docs/Images/image.png"
        $markdownFolder = [System.IO.Path]::GetDirectoryName($MarkdownFilePath)
        $unresolvedPath = Join-Path -Path $markdownFolder -ChildPath $ImagePath
        $resolvedPath = [System.IO.Path]::GetFullPath($unresolvedPath)

        # Take the resolved path relative to the project's Docs\ folder.
        $relativePath = Resolve-Path -Relative $resolvedPath

        # It's possible that we could have tested the path already (rare, but possible) -
        # in that case cache the value so that we don't have to hit the disk again.
        if (!$ResolvedImagePaths.ContainsKey($relativePath)) {
            # First check to see if the path exists at all - if it doesn't exist,
            # this is an invalid image.
            if (-Not (Test-Path $relativePath -PathType leaf)) {
                Write-Host "Image file was not found: "
                Write-Host $relativePath
                $ResolvedImagePaths[$relativePath] = $false
            } else {
                # Otherwise we still have to ensure that the image itself matches the
                # correct case (i.e. image.png vs image.PNG).
                #
                # Checking case-sensitive matches is awkward on some OSes (i.e. Windows)
                # because they are case-insensitive by default. Below is a workaround
                # that uses regex to avoid the issue.
                #
                # Mutate the resolved path by making the last character bracketed by
                # [], which turns it into a regular expression. This makes it so that
                # the returned FullName actually reflects the real fullname on disk
                # rather than the casing that is passed in the regex.
                $lastCharacter = $resolvedPath.Substring($resolvedPath.Length - 1)
                $prefix = $resolvedPath.Substring(0, $resolvedPath.Length - 1)
                $regexPath = "{0}[{1}]" -f $prefix, $lastCharacter
                $file = Get-Item $regexPath

                # The image path is only valid if the actual case on disk matches
                # the case written in the markdown file (i.e. the resolved path)
                $caseAccurateName = Resolve-Path -Relative $file.FullName
                $ResolvedImagePaths[$relativePath] = ($caseAccurateName -ceq $relativePath)

                if (-Not $ResolvedImagePaths[$relativePath]) {
                    Write-Host "Found a case-sensitive mismatch: "
                    Write-Host "Actual:" + $caseAccurateName
                    Write-Host "Markdown:" + $relativePath
                }
            }
        }

        Pop-Location

        $ResolvedImagePaths[$relativePath]
    }
}

<#
.SYNOPSIS
    Checks if the given file at the given line contains a reference to an image that doesn't exist (either
    the image itself doesn't exist or is the wrong case)
#>
function CheckBrokenImages {
    [CmdletBinding()]
    param(
        [string]$DocsRootDir,
        [string]$FileName,
        [string[]]$FileContent,
        [int]$LineNumber
    )
    process {
        # There are two different types of image links we have - some are <img> and some are [](image.png)
        # Below we use two different regexes to capture those (i.e. making sure we're properly pairing the
        # quotes and parenthesis in order to extract the image path)
        # Explanation of the regexes:
        # '\((?!http)([^\)]+\.(?:png|gif|jpg))\)'
        # Looks for cases like ![](path/to/image.png)
        # \(                - Looks for the opening ( surrounding (path/to/image.png)
        # (?!http)          - Filters out absolute HTTP links (we only want to validate images that we control)
        # (                 - Beginning of the grouping/match (used to extract the image path)
        # [^\)]+            - Matches any character that isn't the ) character (to ensure that on lines with
        #                     multiple images, we don't end up matching multiple strings at the same)
        # \.(?:png|gif|jpg) - Matches a filename that ends in png, gif, or jpg
        # )                 - Ending of the grouping/match  (used to extract the image path)
        # \)                - Looks for the closing ) surrounding (path/to/image.png)
        # The imgTagRegex is similar, except for the opening and closing characters (i.e. "" of ())
        $images = @()
        $markdownMatches = ($FileContent[$LineNumber] | Select-String -Pattern '\((?!http)([^\)]+\.(?:png|gif|jpg))\)' -AllMatches).Matches
        foreach ($match in $markdownMatches) {
            $images += $match.Groups[1]
        }
        $imgTagMatches = ($FileContent[$LineNumber] | Select-String -Pattern '\"(?!http)([^\"]+\.(?:png|gif|jpg))\"' -AllMatches).Matches
        foreach ($match in $imgTagMatches) {
            $images += $match.Groups[1]
        }

        try
        {
            $hasBrokenImage = $false;
            foreach ($image in $images) {
                if ([System.IO.Path]::IsPathRooted($image)) {
                    $hasBrokenImage = $true
                    Write-Host -ForegroundColor Red "Absolute path to an image found in $FileName at line $LineNumber"
                    Write-Host -ForegroundColor Red $FileContent[$LineNumber]
                }
                elseif (-Not (CheckImage $DocsRootDir $FileName $image)) {
                    $hasBrokenImage = $true
                    Write-Host -ForegroundColor Red "A broken image link was found in $FileName at line $LineNumber "
                    Write-Host -ForegroundColor Red $FileContent[$LineNumber]
                }
            }
        }
        catch
        {
            Write-Host "Unexpected error: image = `"$image`""
            throw
        }

        $hasBrokenImage;
    }
}

function CheckDocument {
    [CmdletBinding()]
    param(
        [string]$DocsRootDir,
        [string]$FileName
    )
    process {
        Write-Host "Checking file: $FileName"
        # Each line of each script is checked by all of the validators above - this ensures that in
        # a single pass, we'll get all of the issues highlighted all at once, rather than
        # repeatedly running this script, discovering a single issue, fixing it, and then
        # re-running the script
        $issueFound = $false
        $fileContent = Get-Content $FileName
        for ($i = 0; $i -lt $fileContent.Length; $i++) {
            if (CheckDocLinks $FileName $fileContent $i) {
                $issueFound = $true
            } elseif (CheckBrokenImages $DocsRootDir $FileName $fileContent $i) {
                $issueFound = $true
            } elseif (CheckIncorrectRelativePath $FileName $fileContent $i) {
                $issueFound = $true
            }
        }
        $issueFound
    }
}

$containsIssue = $false

# If the file containing the list of changes was provided and actually exists,
# this validation should scope to only those changed files.
if (($ChangesFile) -and (Test-Path $ChangesFile -PathType leaf)) {
    Import-Module "$PSScriptRoot\common.psm1" -Force

    Write-Host "Checking only changed files for doc issues:"
    $changedFiles = GetChangedFiles -Filename $ChangesFile -RepoRoot $RepoRoot

    foreach ($changedFile in $changedFiles) {
        if ((IsMarkdownFile -Filename $changedFile) -and (CheckDocument $Directory $changedFile)) {
            $containsIssue = $true;
        }
    }
}
else {
    Write-Host "Checking $Directory for common doc issues"

    $docFiles = Get-ChildItem $Directory *.md -Recurse | Select-Object FullName
    foreach ($docFile in $docFiles) {
        if (CheckDocument $Directory $docFile.FullName) {
            $containsIssue = $true
        }
    }
}

if ($containsIssue) {
    Write-Output "Issues found, please see above for details"
    exit 1;
}
else {
    Write-Output "No issues found"
    exit 0;
}
