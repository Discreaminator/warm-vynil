#Requires -Version 5.1
<#
.SYNOPSIS
  Stage Release artefacts and build the WARM VYNIL Inno Setup installer (M5).

.EXAMPLE
  .\installer\windows\package.ps1
  .\installer\windows\package.ps1 -BuildDir build\win-release -Configuration Release
#>
[CmdletBinding()]
param(
    [string] $RepoRoot = '',

    [string] $BuildDir = 'build\win-release',

    [string] $Configuration = 'Release',

    [string] $Version = '0.1.0',

    [string] $IsccPath = '',

    [switch] $SkipSign,

    [switch] $SkipBuildCheck
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

function Find-Iscc {
    param([string] $Hint)
    if ($Hint -and (Test-Path -LiteralPath $Hint)) { return (Resolve-Path $Hint).Path }
    $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 7\ISCC.exe'),
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 7\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 7\ISCC.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path -LiteralPath $c) { return $c }
    }
    return $null
}

$buildRoot = if ([IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $RepoRoot $BuildDir }
$artefacts = Join-Path $buildRoot "WarmVynil_artefacts\$Configuration"
$stageRoot = Join-Path $PSScriptRoot 'stage'
$outDir = Join-Path $RepoRoot 'dist\installer'
$iss = Join-Path $PSScriptRoot 'WarmGroove.iss'
$signScript = Join-Path $PSScriptRoot 'sign.ps1'

Write-Host "=== WARM VYNIL package (M5) ==="
Write-Host "Repo:       $RepoRoot"
Write-Host "Artefacts:  $artefacts"
Write-Host "Stage:      $stageRoot"
Write-Host "Version:    $Version"

$vst3Src = Join-Path $artefacts 'VST3\WARM VYNIL.vst3'
$clapSrc = Join-Path $artefacts 'CLAP\WARM VYNIL.clap'
$standaloneSrc = Join-Path $artefacts 'Standalone\WARM VYNIL.exe'

if (-not $SkipBuildCheck) {
    if (-not (Test-Path -LiteralPath $vst3Src)) {
        throw "Missing VST3 at '$vst3Src'. Build Release first (cmake --build --preset win-release --config Release)."
    }
    if (-not (Test-Path -LiteralPath $clapSrc)) {
        throw "Missing CLAP at '$clapSrc'."
    }
}

if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $stageRoot 'VST3') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stageRoot 'CLAP') | Out-Null
New-Item -ItemType Directory -Path (Join-Path $stageRoot 'Standalone') | Out-Null
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

Copy-Item -LiteralPath $vst3Src -Destination (Join-Path $stageRoot 'VST3\WARM VYNIL.vst3') -Recurse -Force
Copy-Item -LiteralPath $clapSrc -Destination (Join-Path $stageRoot 'CLAP\WARM VYNIL.clap') -Force

$hasStandalone = $false
if (Test-Path -LiteralPath $standaloneSrc) {
    Copy-Item -LiteralPath $standaloneSrc -Destination (Join-Path $stageRoot 'Standalone\WARM VYNIL.exe') -Force
    $hasStandalone = $true
    Write-Host 'Standalone: included'
}
else {
    Write-Warning "Standalone EXE not found at '$standaloneSrc' - installer will ship VST3+CLAP only. Reconfigure with -DWG_BUILD_STANDALONE=ON to include it."
}

Copy-Item -LiteralPath (Join-Path $RepoRoot 'LICENSE') -Destination (Join-Path $stageRoot 'LICENSE') -Force
Copy-Item -LiteralPath (Join-Path $RepoRoot 'NOTICE') -Destination (Join-Path $stageRoot 'NOTICE') -Force
$installMd = Join-Path $RepoRoot 'docs\INSTALL.md'
if (-not (Test-Path -LiteralPath $installMd)) {
    throw "Missing docs\INSTALL.md"
}
Copy-Item -LiteralPath $installMd -Destination (Join-Path $stageRoot 'INSTALL.md') -Force

# Also mirror to dist\plugins for local REAPER scanning without admin install
$distPlugins = Join-Path $RepoRoot 'dist\plugins'
New-Item -ItemType Directory -Path (Join-Path $distPlugins 'VST3') -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $distPlugins 'CLAP') -Force | Out-Null
if (Test-Path (Join-Path $distPlugins 'VST3\WARM VYNIL.vst3')) {
    Remove-Item (Join-Path $distPlugins 'VST3\WARM VYNIL.vst3') -Recurse -Force
}
Copy-Item -LiteralPath $vst3Src -Destination (Join-Path $distPlugins 'VST3\WARM VYNIL.vst3') -Recurse -Force
Copy-Item -LiteralPath $clapSrc -Destination (Join-Path $distPlugins 'CLAP\WARM VYNIL.clap') -Force
if ($hasStandalone) {
    New-Item -ItemType Directory -Path (Join-Path $distPlugins 'Standalone') -Force | Out-Null
    Copy-Item -LiteralPath $standaloneSrc -Destination (Join-Path $distPlugins 'Standalone\WARM VYNIL.exe') -Force
}

if (-not $SkipSign) {
    $toSign = @(
        (Join-Path $stageRoot 'VST3\WARM VYNIL.vst3'),
        (Join-Path $stageRoot 'CLAP\WARM VYNIL.clap')
    )
    if ($hasStandalone) {
        $toSign += (Join-Path $stageRoot 'Standalone\WARM VYNIL.exe')
    }
    & $signScript -Paths $toSign
}

$iscc = Find-Iscc -Hint $IsccPath
if (-not $iscc) {
    throw 'ISCC.exe not found. Install Inno Setup 6+ (winget install JRSoftware.InnoSetup) or pass -IsccPath.'
}
Write-Host "ISCC: $iscc"

$defines = @(
    "/DWGAppVersion=$Version",
    "/DWGSourceDir=stage",
    "/DWGOutputDir=$outDir",
    "/DWGOutputBase=WarmVynil-Setup-$Version",
    "/DWGHasStandalone=$(if ($hasStandalone) { '1' } else { '0' })"
)

Push-Location $PSScriptRoot
try {
    & $iscc @defines $iss
    if ($LASTEXITCODE -ne 0) { throw "ISCC failed (exit $LASTEXITCODE)" }
}
finally {
    Pop-Location
}

$setup = Join-Path $outDir "WarmVynil-Setup-$Version.exe"
if (-not (Test-Path -LiteralPath $setup)) {
    throw "Expected installer missing: $setup"
}

if (-not $SkipSign) {
    & $signScript -Paths @($setup)
}

$hash = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash
$shaFile = "$setup.sha256"
Set-Content -LiteralPath $shaFile -Value "$hash  $(Split-Path $setup -Leaf)" -Encoding ascii

Write-Host ''
Write-Host "Installer: $setup"
Write-Host "SHA256:    $hash"
Write-Host 'Done.'
