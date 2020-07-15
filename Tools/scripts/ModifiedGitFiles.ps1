<#
.SYNOPSIS
    This script generates a list of changed files between two git branches (in particular files
    modified in a given pull request) and outputs that list to a file.
.DESCRIPTION
    Generates a file containing a list of all modified files (added/modified) in
    the given pull request. The output file contains a list of paths relative to repo root,
    each file on separate line. E.g.:

    UXToolsGame/UXToolsGame.uproject
    UXToolsGame/Config/HoloLens/HoloLensEngine.ini

    The file will not contain files that have been completely deleted (for example, if
    a change deleted UXToolsGame/Content/DeletedFile.uasset), that line will not exist
    in the output file.

    The script will fetch the target branch from origin if it doesn't exist in the local git repo.
    This is what happens by default on Azure DevOps pipeline integration with pull requests (shallow fetch).
    In particular, this will checkout via this command:
    git fetch --force --tags --prune --progress --no-recurse-submodules origin $(System.PullRequest.TargetBranch)
.EXAMPLE
    .\githubchanges.ps1 -OutputFile c:\path\to\changes\file.txt -PullRequestId 1234 -RepoRoot c:\path\to\mrtk -TargetBranch mrtk_development
#>
param(
    # Optional: branch containing changes to compare, e.g. "pull/[PullRequestId]/merge" on GitHub
    # Note that the format of the pull request branch (i.e. "pull/$PullRequestId/merge") is based on the format 
    # that Azure DevOps does for its local checkout of the pull request code.
    # If not provided, currently checked out commit will be used (HEAD).
    [string]$ModifiedBranch,

    # The target branch that the pull request will merge into (e.g. mrtk_development)
    [string]$TargetBranch,

    # The output filename (e.g. c:\path\to\output.txt)
    [Parameter(Mandatory=$true)]
    [string]$OutputFile,

    # The root folder of the repo (e.g. c:\repo)
    # This primarily used to filter out files that were deleted.
    [Parameter(Mandatory=$true)]
    [string]$RepoRoot
)

# The pull request ID might not be present (i.e. this is an adhoc build being spun up)
# and the target branch might not be set in which case there's nothing to validate.
if ([string]::IsNullOrEmpty($TargetBranch))
{
    Write-Warning "-TargetBranch needs to be specified."
    exit 1;
}

if ([string]::IsNullOrEmpty($ModifiedBranch))
{
    $ModifiedBranch = "HEAD"
}

# If the output file already exists, blow it away. Each run should get a new set of changed files.
if (Test-Path $OutputFile -PathType leaf) {
    Remove-Item $OutputFile
}
$Null = (New-Item -ItemType File -Force -Path $OutputFile)

# The path to the .git file is necessary when invoking the git command below, as the working
# directory may not actually be pointed toward the git path.
$gitDir = Join-Path -Path $RepoRoot -ChildPath ".git"

$TargetBranch = $TargetBranch -replace "refs/heads/",""

# Check if TargetBranch exists in local repo
$Null = $(& git --git-dir=$gitDir rev-parse origin/$TargetBranch)
if ($LASTEXITCODE -ne 0)
{
    # Set up credentials if provided
    $RepoUrl = $(& git --git-dir=$gitDir remote get-url origin)
    if (Test-Path env:SYSTEM_ACCESSTOKEN)
    {
        $protocol, $url = $RepoUrl.Split("//", 2)
        $protocol = ($protocol.TrimEnd("/")) + "//"
        $url = ($url.TrimStart("/"))
        if ($url.contains('@'))
        {
            $_, $url = $url.Split("@", 2)    
        }
        $RepoUrl = "$($protocol)user:$($env:SYSTEM_ACCESSTOKEN)@$($url)"
    }

    # Fetches the target branch so that the git diffing down below will actually be possible. git diff will list
    # the set of changed files between two different commit stamps (or branches, in this case), and needs
    # both branches to exist in order to make this happen.
    # Uses a shallow fetch (i.e. depth=1) because only the latest commit from the target branch is
    # needed to do the diff.
    Write-Host "Fetching $TargetBranch from origin."
    git --git-dir=$gitDir --work-tree=$RepoRoot  fetch --depth=1 --force --tags --prune --progress --no-recurse-submodules --quiet $RepoUrl $TargetBranch
    if ($LASTEXITCODE -ne 0)
    {
        Write-Host "Git finished with RC=$LASTEXITCODE"
        exit $LASTEXITCODE
    }
}

# The set of changed files is the diff between the target branch and the ModifiedBranch (pull request branch)
# that was checked out locally. 
$changedFiles=$(git --git-dir=$gitDir --work-tree=$RepoRoot diff --name-only $ModifiedBranch origin/$TargetBranch 2>&1)

foreach ($changedFile in $changedFiles) {
    $joinedPath = Join-Path -Path $RepoRoot -ChildPath $changedFile
    # Only save the path if the file still exists - also, do not store the absolute path
    # of the file, in case this set of information is used later in the pipeline on a different
    # machine/context.
    if (Test-Path $joinedPath -PathType leaf) {
        Add-Content -Path $OutputFile -Value $changedFile
    }
}
