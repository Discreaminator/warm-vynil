# Discreaminator

Vinyl cutting → cartridge → tube RIAA character insert effect. **No** surface noise, crackle, wow/flutter, or rumble.

Implemented against `SPEC.md` (Warm Groove ТЗ). Product name: **Discreaminator** · Company: **Vynil Audio**.

| | |
|---|---|
| Formats (M0) | **VST3** + **CLAP** (Windows) |
| License | **AGPL-3.0** (JUCE AGPL path) |
| Codes | manufacturer `Vnyl`, plugin `Dscr` |
| Bundle / CLAP ID | `com.vynilaudio.discreaminator` |

## Requirements (Windows)

- CMake ≥ 3.25
- Visual Studio 2022 (MSVC) with C++ desktop workload
- Git (for submodules)

## Clone

```powershell
git clone --recurse-submodules <repo-url> warm-groove
cd warm-groove
# If already cloned without submodules:
git submodule update --init --recursive
```

Pinned: **JUCE 8.0.15**, **clap-juce-extensions 0.26.0**.

## Build (Release)

```powershell
cmake --preset win-release
cmake --build --preset win-release --config Release
ctest --preset win-release --output-on-failure
```

Outputs (typical paths under `build/win-release`):

- VST3: `Discreaminator_artefacts/Release/VST3/Discreaminator.vst3`
- CLAP: `Discreaminator_artefacts/Release/CLAP/Discreaminator.clap`
- Tests: `tests/Release/wg_tests.exe`
- Offline sine: `tools/.../wg_render.exe`

### Copy into REAPER search paths (optional)

```powershell
# VST3
Copy-Item -Recurse "build\win-release\Discreaminator_artefacts\Release\VST3\Discreaminator.vst3" `
  "$env:CommonProgramFiles\VST3\"
# CLAP
Copy-Item "build\win-release\Discreaminator_artefacts\Release\CLAP\Discreaminator.clap" `
  "$env:CommonProgramFiles\CLAP\"
```

Then in REAPER: **Options → Preferences → Plug-ins → VST → Re-scan** (and enable CLAP path if needed).

### Smoke sine WAV

```powershell
.\build\win-release\Release\wg_render.exe --signal sine --freq 1000 --level -6 --sr 48000 --seconds 1 --out sine.wav
```

## Milestone status

See [PROGRESS.md](PROGRESS.md). **M0** = skeleton / passthrough. DSP starts at M1.

## Spec source

Authoritative ТЗ lives alongside this tree: `../warm_groove_spec/SPEC.md` (and is intended to be copied/linked into the repo as work continues).
