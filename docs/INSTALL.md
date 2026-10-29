# Install WARM VYNIL (Windows)

**Product:** WARM VYNIL · **Company:** Discreaminator · **License:** AGPL-3.0

## Recommended: installer

1. Download `WarmVynil-Setup-<version>.exe` from the [GitHub Releases](https://github.com/discreaminator/warm-vynil/releases) page (or build it locally — see below).
2. Run the installer. Administrator rights are required so plugins land in the shared Common Files folders.
3. Default install locations:
   - **VST3:** `C:\Program Files\Common Files\VST3\WARM VYNIL.vst3`
   - **CLAP:** `C:\Program Files\Common Files\CLAP\WARM VYNIL.clap`
   - **Standalone** (if included): `C:\Program Files\Discreaminator\WARM VYNIL\WARM VYNIL.exe`
4. License / NOTICE / this file are copied under `C:\Program Files\Discreaminator\WARM VYNIL\`.

### Windows SmartScreen / “Unknown publisher”

Release builds may be **unsigned** when no paid Authenticode certificate is configured (see [SIGNING.md](SIGNING.md)). On first launch Windows may show SmartScreen:

1. Click **More info**
2. Click **Run anyway**

Once a paid OV/EV certificate or Azure Trusted Signing is wired via CI secrets, future installers will be signed and SmartScreen warnings should stop after reputation builds.

## REAPER — rescan plugins

After install (or after dropping plugins into a custom folder):

1. **Options → Preferences → Plug-ins → VST**
2. Ensure the VST3 path includes `C:\Program Files\Common Files\VST3` (REAPER usually scans this by default).
3. For CLAP: **Options → Preferences → Plug-ins → CLAP** (or equivalent) and add `C:\Program Files\Common Files\CLAP` if needed.
4. Click **Re-scan** / **Clear cache and re-scan**.
5. Insert **WARM VYNIL** (manufacturer **Discreaminator**).

### Dev / no-admin staging

Local Release builds are also mirrored to `dist\plugins\VST3` and `dist\plugins\CLAP`. Add those folders in REAPER’s VST/CLAP path lists and Re-scan — no installer required.

## Other hosts

Point the host at the Common Files VST3 / CLAP directories above (or `dist\plugins\…` for local builds) and refresh the plugin list.

## Uninstall

Use **Apps → Installed apps → WARM VYNIL → Uninstall**, or the Start Menu uninstall shortcut under Discreaminator. This removes the VST3 bundle, CLAP, Standalone, and docs placed by the installer.

## Build the installer from source

Requirements: Visual Studio 2022, CMake ≥ 3.25, [Inno Setup 6+](https://jrsoftware.org/isinfo.php) (`winget install JRSoftware.InnoSetup`).

```powershell
cmake --preset pack-win
cmake --build --preset pack-win --config Release
ctest --preset pack-win --output-on-failure   # optional
powershell -ExecutionPolicy Bypass -File .\installer\windows\package.ps1
```

Output: `dist\installer\WarmVynil-Setup-0.1.0.exe` (+ `.sha256`).

Signing is attempted automatically when credentials are present; otherwise packaging continues **unsigned** (see [SIGNING.md](SIGNING.md)).

## macOS

macOS installers / notarization are deferred (Windows-first). Placeholder notes live under `installer/macos/`.
