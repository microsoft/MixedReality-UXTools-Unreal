# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Array of refs to push to github.
# Comma separates each element
$refs_to_push=@("public/0.8.x") #TODD add release/0.8.0 when tagged
# Github repo
$github="https://github.com/microsoft/MixedReality-UXTools-Unreal"

git fetch origin
if($LastExitCode -ne 0) {
    Write-Host "Unable to git fetch"
    exit 1
}
foreach ($ref in $refs_to_push) {
    Write-Host "Pushing ${ref}"
    git push --progress "$github" "refs/remotes/origin/${ref}:refs/heads/${ref}"
}