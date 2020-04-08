# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Script to generate the docs into doc folder

param(
    # Serve the generated docs on a temporary web server @ localhost
    # The docs are not completely static, so will not work if not served.
    [switch]$serve = $false
)
Push-Location (Split-Path $MyInvocation.MyCommand.Path)

# Clear output dir
Write-Host "Deleting previously generated doc folder"
Remove-Item -Force -Recurse -ErrorAction Ignore .\doc

# Generate YAML from C++ files
Invoke-Expression code2yaml.exe

# Generate website via docfx
$errors = 0
$output = ""
Invoke-Expression "docfx -f" | Tee-Object -Variable output | Write-Host
$results = $output | Out-String
if ($results -match "(?<warningCount>\d*) Warning\(s\)\s*(?<errorCount>\d*) Error\(s\)")
{
    if ($Matches.errorCount -gt 0 -or $Matches.warningCount -gt 0)
    {
        Write-Host "Broken reference found in documentation - Build validation failed." -ForegroundColor red
		$errors = 1;
    }
}

if ($errors -eq 0)
{
	Write-Host "Success! Documentation generated in doc folder" -ForegroundColor green
}

#delete temp folders
Write-Host "Deleting code2yaml temp folder"
Remove-Item -Force -Recurse -ErrorAction Ignore ..\..\tempcode2yaml

Write-Host "Deleting docfx obj folder"
Remove-Item -Force -Recurse -ErrorAction Ignore .\obj

# optional serve on localhost
if ($serve)
{
	start 'http://localhost:8080'
	docfx serve doc
}

Pop-Location

exit $errors