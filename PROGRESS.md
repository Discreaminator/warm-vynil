# PROGRESS — Discreaminator (Warm Groove SPEC)

## Identity (M0, user-approved)

| Field | Value |
|---|---|
| Plugin name | **Discreaminator** |
| Company | **Vynil Audio** |
| Manufacturer code (4-char) | **`Vnyl`** |
| Plugin code (4-char) | **`Dscr`** |
| Bundle / CLAP ID | `com.vynilaudio.discreaminator` |
| License | **AGPL-3.0** (`LICENSE`) |
| Formats (M0) | **VST3 + CLAP** only |
| Platform (M0) | **Windows** only |

Source of branding: `cmake/PluginConfig.cmake` + `src/plugin/ProductInfo.h`.

---

## M0 — Skeleton (2026-10-29) — DONE, STOPPED

### Done

- Repo at `C:\Vynil vst\warm-groove` per SPEC §2 layout (subset sufficient for M0).
- Submodules:
  - **JUCE** tag **`8.0.15`**
  - **clap-juce-extensions** commit **`7adee3a`** (main tip; tag `0.26.0` does **not** build against JUCE ≥ 8.0.11 — needs `#169` compat)
- CMake + `CMakePresets.json` (`win-release`, `dev-debug`, `ci`)
- Passthrough `wg::dsp::Engine`, stub editor, `wg_tests` smoke, `wg_render` sine → WAV
- CI workflow: Windows-only (`.github/workflows/ci.yml`)
- README build instructions; AGPL `LICENSE`

### Build commands used

```powershell
cmake --preset win-release
cmake --build --preset win-release --config Release
ctest --preset win-release --output-on-failure
.\build\win-release\Release\wg_render.exe --signal sine --freq 1000 --level -6 --sr 48000 --seconds 0.5 --out build\win-release\m0_sine.wav
```

### Artefacts

| Artefact | Path |
|---|---|
| VST3 | `build\win-release\Discreaminator_artefacts\Release\VST3\Discreaminator.vst3` |
| CLAP | `build\win-release\Discreaminator_artefacts\Release\CLAP\Discreaminator.clap` |
| Local copy (for REAPER path) | `dist\plugins\VST3\` and `dist\plugins\CLAP\` |
| `wg_tests` | `build\win-release\tests\Release\wg_tests.exe` |
| `wg_render` | `build\win-release\Release\wg_render.exe` |
| Sine WAV | `build\win-release\m0_sine.wav` |

### Tests

- `ctest`: **1/1 PASS** — `Engine passthrough leaves buffer unchanged`
- **pluginval** strictness **5**, `--validate-in-process --skip-gui-tests`: **SUCCESS** on VST3

### Deviations from SPEC §12 M0 DoD (user-binding)

1. **No Standalone** — user preferred VST3+CLAP only (`WG_BUILD_STANDALONE=OFF`).
2. **No macOS / AU / mac CI** — Windows-only for M0.
3. **clap-juce-extensions** pinned to commit `7adee3a`, not release tag `0.26.0` (JUCE 8.0.15 compat).
4. System install to `C:\Program Files\Common Files\{VST3,CLAP}` needs admin — not done; plugins staged under `dist\plugins\`.
5. REAPER load: not GUI-verified in this session (REAPER installed at `C:\Program Files\REAPER (x64)`). User should add `dist\plugins\VST3` (and CLAP path) and re-scan.

### Open questions (carry to later milestones)

- Confirm manufacturer/plugin 4-char codes stay `Vnyl` / `Dscr` permanently.
- When to re-enable Standalone / macOS+AU.
- Git remote for this repo (local init only so far).

### STOP

**M1 not started.** Await explicit command.
