<#
.SYNOPSIS
    Build UXTPlugin.

.PARAMETER UnrealEngine
    Path to Unreal Engine (root folder).

.PARAMETER PackageOutDir
    Path to where packaged plugin should be published.
#>
param(
    [Parameter(Mandatory=$true)]
    [string]$UnrealEngine,
    [Parameter(Mandatory=$true)]
    [string]$PackageOutDir,
    [string]$PluginPath = $null,
    [string]$TargetPlatforms = "Win64+HoloLens+Android",
    [boolean]$Clean = $false,
    [boolean]$StrictMode = $false
)

Import-Module -Name "$PSScriptRoot\BuildTools.psm1" -Force

if ([String]::IsNullOrEmpty($PluginPath))
{
    $PluginPath = Resolve-Path -Path "$PSScriptRoot\..\..\UXToolsGame\Plugins\UXTools\UXTools.uplugin"
}

$CommandArgs = "BuildPlugin", `
               "-Plugin=`"$PluginPath`"", `
               "-Package=`"$PackageOutDir`"", `
               "-TargetPlatforms=`"$TargetPlatforms`""

if ($Clean)
{
    $CommandArgs += "-Clean"
}

if ($StrictMode)
{
    $CommandArgs += "-StrictIncludes"
}

# Static Analyzer turned off for Plugin Build as it reports errors in Engine when building Shipping config 
$Result = Start-UAT -UnrealEngine $UnrealEngine -CommandArgs $CommandArgs -UseStaticAnalyzer $False -UseUnityBuild (-not $StrictMode) -ErrorAction Stop

$RC = 0
if ($Result)
{
    Write-Host "Build successful."
}
else
{
    $RC = 1
    Write-Host "Build failed. (See log for details)"
}
exit $RC
