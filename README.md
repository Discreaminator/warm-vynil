# WARM VYNIL

Vinyl cutting → cartridge → tube RIAA character insert effect. **No** surface noise, crackle, wow/flutter, or rumble.

Implemented against `SPEC.md` (Warm Groove ТЗ). Product: **WARM VYNIL** · Company: **Discreaminator**.

| | |
|---|---|
| Formats | **VST3** + **CLAP** (+ **Standalone** in pack builds) on Windows |
| License | **AGPL-3.0** — see `LICENSE` + `NOTICE` |
| Copyright | Copyright (C) 2026 Discreaminator |
| Codes | manufacturer `Dscr`, plugin `WVnl` |
| Bundle / CLAP ID | `com.discreaminator.warmvynil` |
| Installer | Inno Setup — see [docs/INSTALL.md](docs/INSTALL.md) |

## Requirements (Windows)

- CMake ≥ 3.25
- Visual Studio 2022 (MSVC) with C++ desktop workload
- Git (for submodules)

## Clone

```powershell
git clone --recurse-submodules <repo-url> warm-groove
cd warm-groove
git submodule update --init --recursive
```

Pinned: **JUCE 8.0.15**, **clap-juce-extensions** `7adee3a`.

## Build (Release)

```powershell
cmake --preset win-release
cmake --build --preset win-release --config Release
ctest --preset win-release --output-on-failure
```

Outputs (typical under `build/win-release`):

- VST3 / CLAP under `WarmVynil_artefacts/Release/`
- Staged copies: `dist/plugins/VST3/` and `dist/plugins/CLAP/`

### REAPER

Add folder `dist\plugins\VST3` (and `dist\plugins\CLAP`), then **Re-scan**. Insert **WARM VYNIL** (manufacturer Discreaminator).

### Smoke sine WAV

```powershell
.\build\win-release\Release\wg_render.exe --signal sine --freq 1000 --level -6 --sr 48000 --seconds 1 --out sine.wav
```

## Windows installer (M5)

```powershell
cmake --preset pack-win
cmake --build --preset pack-win --config Release
powershell -ExecutionPolicy Bypass -File .\installer\windows\package.ps1
```

Produces `dist\installer\WarmVynil-Setup-0.1.0.exe`. Signing runs when cert env vars are set; otherwise the build is **unsigned** ([docs/SIGNING.md](docs/SIGNING.md)). User install notes: [docs/INSTALL.md](docs/INSTALL.md).

Tagged releases (`v*`) use `.github/workflows/release.yml`.

## Milestone status

See [PROGRESS.md](PROGRESS.md). **M5** = Windows packaging / Inno installer (macOS deferred).
