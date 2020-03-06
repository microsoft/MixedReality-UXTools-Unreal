# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.

# Script to generate the docs into doc folder

param(
    # Serve the generated docs on a temporary web server @ localhost
    # The docs are not completely static, so will not work if not served.
    [switch]$serve = $false
)

# Clear output dir
Write-Host "Deleting previously generated doc folder"
Remove-Item -Force -Recurse -ErrorAction Ignore .\doc

# Generate YAML from C++ files
code2yaml.exe

# Generate website via docfx
docfx
Write-Host "Documentation generated in doc folder"

Write-Host "Deleting code2yaml temp folder"
Remove-Item -Force -Recurse -ErrorAction Ignore ..\tempcode2yaml

Write-Host "Deleting docfx obj folder"
Remove-Item -Force -Recurse -ErrorAction Ignore .\obj

if ($serve)
{
	start 'http://localhost:8080'
	docfx serve doc
}


