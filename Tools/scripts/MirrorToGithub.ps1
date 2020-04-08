# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Array of release branches to sync. We assume the branch name is "release/$name"
# Comma separates each element
$branches=@("0.1.0")
#
$github="https://github.com/microsoft/MixedReality-UXTools-Unreal"

git fetch origin
if($LastExitCode -ne 0) {
    Write-Host "Unable to git fetch"
	exit 1
}
foreach ($branch in $branches) {
    Write-Host "Pushing ${branch}"
    git push --progress "$github" "refs/remotes/origin/release/${branch}:refs/heads/release/${branch}"
}
