# PROGRESS — WARM VYNIL (Warm Groove SPEC)

## Identity (M0, user-corrected rebrand)

| Field | Value |
|---|---|
| Plugin display name | **WARM VYNIL** |
| Company / manufacturer | **Discreaminator** |
| Copyright | **Copyright (C) 2026 Discreaminator** (`NOTICE`, `LICENSE` header) |
| Manufacturer code (4-char) | **`Dscr`** |
| Plugin code (4-char) | **`WVnl`** |
| Bundle / CLAP ID | `com.discreaminator.warmvynil` |
| License | **AGPL-3.0** |
| Formats (M0) | **VST3 + CLAP** only |
| Platform (M0) | **Windows** only |
| CMake target | `WarmVynil` |

Source of branding: `cmake/PluginConfig.cmake` + `src/plugin/ProductInfo.h`.

**Supersedes** earlier M0 draft that used display name Discreaminator / company Vynil Audio / codes `Vnyl`/`Dscr` (wrong mapping). Old codes retired to avoid host-ID confusion.

---

## M0 — Skeleton (2026-10-29) — DONE + REBRAND, STOPPED

### Done

- Repo at `C:\Vynil vst\warm-groove` per SPEC §2 layout (M0 subset).
- Submodules: JUCE **8.0.15**; clap-juce-extensions **`7adee3a`**.
- Passthrough engine, stub editor (shows **WARM VYNIL**), smoke test, `wg_render`.
- Windows-only CI; README; AGPL + NOTICE.

### Build

```powershell
cmake --preset win-release
cmake --build --preset win-release --config Release
ctest --preset win-release --output-on-failure
```

### Artefacts (after rebrand rebuild)

| Artefact | Path |
|---|---|
| VST3 | `build\win-release\WarmVynil_artefacts\Release\VST3\WARM VYNIL.vst3` |
| CLAP | `build\win-release\WarmVynil_artefacts\Release\CLAP\WARM VYNIL.clap` |
| Staged for REAPER | `dist\plugins\VST3\WARM VYNIL.vst3` · `dist\plugins\CLAP\WARM VYNIL.clap` |

### Tests

- `ctest`: smoke passthrough PASS
- pluginval strictness 5: run after rebrand if binary available

### Deviations (unchanged)

- No Standalone; no macOS/AU CI (user M0 decisions).
- No admin install to Common Files — use `dist\plugins\`.

### STOP

**M1 not started.** Await explicit command.
