<#
.SYNOPSIS
    Build UXTGame project.

.PARAMETER UnrealEngine
    Path to Unreal Engine (root folder).

.PARAMETER Platform
    Target platform: Win64, HoloLens, Android

.PARAMETER Configuration
    Build Configuration: Debug, Development, Shipping.

.PARAMETER ArchivePath
    Optional: path to where the game package should be published.
#>
param(
    [Parameter(Mandatory=$true)]
    [string]$UnrealEngine,
    [Parameter(Mandatory=$true)]
    [string]$Platform,
    [Parameter(Mandatory=$true)]
    [string]$Configuration,
    [string]$CookFlavor = $null,
    [string]$ProjectPath = $null,
    [boolean]$UnityBuild = $true,
    [boolean]$Clean = $false,
    [string]$ArchivePath = $null,
    [boolean]$UseStaticAnalyzer = $false
)

Import-Module -Name "$PSScriptRoot\BuildTools.psm1" -Force

if ([String]::IsNullOrEmpty($ProjectPath))
{
    $ProjectPath = Resolve-Path -Path "$PSScriptRoot\..\..\UXToolsGame\UXToolsGame.uproject"
}

$BuildTarget = "UXToolsGame"

$CommandArgs = "BuildCookRun", `
               "-project=`"$ProjectPath`"", `
               "-cook", `
               "-allmaps", `
               "-build", `
               "-stage", `
               "-package", `
               "-platform=$Platform", `
               "-clientconfig=$Configuration", `
               "-serverconfig=$Configuration", `
               "-target=$BuildTarget"

if ($Clean -eq $true)
{
    $CommandArgs += "-clean"
}

if (-not [String]::IsNullOrEmpty($ArchivePath))
{
    $CommandArgs += "-pak"
    $CommandArgs += "-archive"
    $CommandArgs += "-archivedirectory=`"$ArchivePath`""
}

if (-not [String]::IsNullOrEmpty($CookFlavor))
{
    $CommandArgs += "-cookflavor=$CookFlavor"
}

if (-not $UnityBuild)
{
    $CommandArgs += "-DisableUnity"
}

$Result = Start-UAT -UnrealEngine $UnrealEngine -CommandArgs $CommandArgs -UseStaticAnalyzer $UseStaticAnalyzer -UseUnityBuild $UnityBuild -ErrorAction Stop

$RC = 0
if ($Result)
{
    Write-Host "Build successful."
}
else
{
    $RC = 1
    Write-Host "Build failed."
}
exit $RC
