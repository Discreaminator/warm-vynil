#Requires -Version 5.1
<#
.SYNOPSIS
  Authenticode-sign WARM VYNIL binaries and/or the Inno Setup installer (SPEC S12).

.DESCRIPTION
  Signing runs only when credentials are available. Without a paid OV/EV or Azure
  Trusted Signing identity this script exits 0 and records SKIPPED - SmartScreen
  may warn on first run of an unsigned installer (documented in docs/INSTALL.md
  and docs/SIGNING.md).

  Supported auth modes (first match wins):
    1. WG_SIGN_PFX_PATH + WG_SIGN_PFX_PASSWORD   (local .pfx / GitHub secret file)
    2. WG_SIGN_CERT_THUMBPRINT                   (cert already in CurrentUser/LocalMachine My)
    3. Azure Trusted Signing via WG_AZURE_TRUSTED_SIGNING=1 and azuresigntool on PATH
       (WG_AZURE_TENANT_ID, WG_AZURE_CLIENT_ID, WG_AZURE_CLIENT_SECRET, WG_AZURE_ENDPOINT,
        WG_AZURE_ACCOUNT, WG_AZURE_CERTIFICATE_PROFILE)

.PARAMETER Paths
  Files to sign (.dll inside .vst3, .clap, .exe, Setup.exe).

.PARAMETER SkipIfMissing
  If set (default), missing credentials -> SKIPPED instead of error.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, ValueFromRemainingArguments = $false)]
    [string[]] $Paths,

    [switch] $SkipIfMissing = $true,

    [string] $TimestampUrl = $(if ($env:WG_SIGN_TIMESTAMP_URL) { $env:WG_SIGN_TIMESTAMP_URL } else { 'http://timestamp.digicert.com' }),

    [string] $Description = 'WARM VYNIL'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Find-SignTool {
    $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $kits = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
    if (Test-Path $kits) {
        $found = Get-ChildItem -Path $kits -Filter signtool.exe -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x64\\' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    return $null
}

function Resolve-ExistingFiles([string[]] $InputPaths) {
    $out = @()
    foreach ($p in $InputPaths) {
        if (-not $p) { continue }
        if (Test-Path -LiteralPath $p -PathType Leaf) {
            $out += (Resolve-Path -LiteralPath $p).Path
            continue
        }
        if (Test-Path -LiteralPath $p -PathType Container) {
            # Sign payload DLLs inside a VST3 bundle
            $dlls = Get-ChildItem -LiteralPath $p -Recurse -File -Include *.dll, *.vst3, *.clap, *.exe -ErrorAction SilentlyContinue |
                Where-Object {
                    $_.Extension -in '.dll', '.clap', '.exe' -or
                    ($_.Extension -eq '.vst3' -and -not $_.PSIsContainer)
                }
            foreach ($f in $dlls) { $out += $f.FullName }
        }
    }
    return $out | Select-Object -Unique
}

$files = @(Resolve-ExistingFiles $Paths)
if ($files.Count -eq 0) {
    Write-Host '[sign] No files to sign.'
    exit 0
}

$mode = $null
if ($env:WG_AZURE_TRUSTED_SIGNING -eq '1') {
    $mode = 'azure'
}
elseif ($env:WG_SIGN_PFX_PATH -and (Test-Path -LiteralPath $env:WG_SIGN_PFX_PATH)) {
    $mode = 'pfx'
}
elseif ($env:WG_SIGN_CERT_THUMBPRINT) {
    $mode = 'thumbprint'
}

if (-not $mode) {
    $msg = '[sign] SKIPPED - no Authenticode credentials (WG_SIGN_PFX_PATH / WG_SIGN_CERT_THUMBPRINT / WG_AZURE_TRUSTED_SIGNING). Unsigned artefacts; see docs/SIGNING.md.'
    Write-Warning $msg
    if (-not $SkipIfMissing) { throw $msg }
    exit 0
}

Write-Host "[sign] Mode=$mode  files=$($files.Count)  ts=$TimestampUrl"

if ($mode -eq 'azure') {
    $az = Get-Command azuresigntool.exe -ErrorAction SilentlyContinue
    if (-not $az) {
        $msg = '[sign] Azure Trusted Signing requested but azuresigntool.exe not on PATH.'
        if ($SkipIfMissing) { Write-Warning $msg; exit 0 }
        throw $msg
    }
    foreach ($f in $files) {
        & $az.Source sign `
            -kvu $env:WG_AZURE_ENDPOINT `
            -kvi $env:WG_AZURE_CLIENT_ID `
            -kvs $env:WG_AZURE_CLIENT_SECRET `
            -kvt $env:WG_AZURE_TENANT_ID `
            -kvc $env:WG_AZURE_CERTIFICATE_PROFILE `
            -d $Description `
            -tr $TimestampUrl `
            -td sha256 `
            $f
        if ($LASTEXITCODE -ne 0) { throw "azuresigntool failed for $f (exit $LASTEXITCODE)" }
        Write-Host "[sign] OK $f"
    }
    exit 0
}

$signtool = Find-SignTool
if (-not $signtool) {
    $msg = '[sign] signtool.exe not found (install Windows SDK Signing Tools).'
    if ($SkipIfMissing) { Write-Warning $msg; exit 0 }
    throw $msg
}

foreach ($f in $files) {
    $args = @(
        'sign', '/fd', 'SHA256', '/td', 'SHA256', '/tr', $TimestampUrl,
        '/d', $Description
    )
    if ($mode -eq 'pfx') {
        $args += @('/f', $env:WG_SIGN_PFX_PATH)
        if ($env:WG_SIGN_PFX_PASSWORD) {
            $args += @('/p', $env:WG_SIGN_PFX_PASSWORD)
        }
    }
    else {
        $args += @('/sha1', $env:WG_SIGN_CERT_THUMBPRINT)
    }
    $args += $f

    & $signtool @args
    if ($LASTEXITCODE -ne 0) { throw "signtool failed for $f (exit $LASTEXITCODE)" }
    Write-Host "[sign] OK $f"
}

Write-Host '[sign] Done.'
