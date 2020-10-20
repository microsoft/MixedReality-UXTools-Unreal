# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

$includeList = @("*.h", "*.cpp", "*.cs")
# https://docs.opensource.microsoft.com/content/releasing/copyright-headers.html?q=header
$CurrentYear = (Get-Date).Year
$copyright = (@(
    "// Copyright (c) $($CurrentYear) Microsoft Corporation.",
    "// Licensed under the MIT License."
) -Join "`r`n")

function CheckCopyright($path) {
    $OriginalContent = (Get-Content -Path $path)
    # remove comments at the top of the file
    $EndOfComments = $False
    $ContentWithoutHeader = (($OriginalContent | Where-Object {
        if ($EndOfComments)
        {
            return $True
        }
        if (-not [string]::IsNullOrEmpty($_) -and $_ -notmatch "^\s*//")
        {
            $EndOfComments = $True
            return $True
        }
        return $False
    }) -Join "`r`n")
    $NewContent = $copyright + "`r`n`r`n" + $ContentWithoutHeader
    if (($OriginalContent -Join "`r`n") -ne $NewContent)
    {
        Write-Host "Updating: $path"
        $NewContent | Set-Content "$path"
    }
    else
    {
        Write-Host "ok: $path"
    }
}

# MAIN
$reporoot = &git rev-parse --show-toplevel
Get-ChildItem -Path $reporoot -Recurse -File -Include $includeList | ForEach-Object { CheckCopyright $_ }
