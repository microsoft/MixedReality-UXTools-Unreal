# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

$includeList = @("*.h", "*.cpp", "*.cs")
$placeholder = "// Fill out your copyright notice in the Description page of Project Settings."
# https://docs.opensource.microsoft.com/content/releasing/copyright-headers.html?q=header
$copyright = @'
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
'@

function CheckCopyright($path) {
    $first2lines = (Get-Content -Path $path -TotalCount 2) -Join "`r`n"
    $existing = $first2lines.ToLower().IndexOf('copyright')
    if ( $existing -ge 0) {
        # There is /some/ copyright
        if ($first2lines.StartsWith($copyright)) {
            # MS MIT is good
            Write-Host "ok $path"
        }
        elseif ($first2lines.StartsWith($placeholder) ) {
            (Get-Content -raw -Path $path).Replace($placeholder,$copyright) | Set-Content "$path"
            Write-Host "REPLACE $path $first2lines"
        }
        else {
            # Anything else needs eyes
            Write-Host "Other $path $first2lines"
        }
    } else {
        # Add copyright if none exists.
        ($copyright + "`r`n" + (Get-Content -raw -Path $path)) | Set-Content "$path"
        Write-Host "ADD $path"
    }
}

# MAIN
$reporoot = &git rev-parse --show-toplevel
Get-ChildItem -Path $reporoot -Recurse -File -Include $includeList | ForEach-Object { CheckCopyright $_ }
