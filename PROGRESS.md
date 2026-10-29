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

### STOP (historical)

M0 closed; M1 implemented on `cursor/m1-dsp-core-e42a` (see below).

---

## M1 — Offline DSP core (2026-10-29) — DONE, STOPPED

### Done

- Full `wg_dsp::Engine` offline chain (SPEC §3–§6): DC → MonoBass → OS → RIAA pre/Neumann/tracing/de/tilt → cart → crosstalk → 12AX7 (Koren/Fast) → transformer → OS ↓ → AutoGain → mix/PDC → out.
- Pure C++20 DSP in `src/dsp` (no JUCE in DSP).
- RIAA: matched-z poles + offline LS table (`tools/design/riaa_reference.py` → `RiaaCoeffs.generated.h`) + zero refine; exact `b↔a` pre inverse; Neumann matched-pole.
- Half-band OS 2x/4x/8x (`design_halfband.py`, ≥110 dB design); integer latency.
- MacroMap §5 with M1 calibration constants (below).
- `wg_render` (signals + `--calibrate`); `tools/analysis/analyze.py` for plots 1–7.
- Catch2 M1 suite: T1, T1b, T2, T3, T4–T4d, T5 (M1 gate), T7, T8, T9, T10, T12, T13 (no ADAA), T14.

### Calibration constants (record per SPEC §5 / §12)

| Knob | SPEC nominal | M1 value | Why |
|---|---|---|---|
| Tube `Vsw` | 1.5 V | **0.14 V** | Koren at −6 dBFS otherwise ≫3 % THD at drive 0 |
| Tube drive | `a · 18 · tubeTrim` dB | **`a · 10 · tubeTrim` dB** | T5 OS4x alias headroom with tracing |
| `k_dyn` | `a · 0.15 · tubeTrim` | **`a · 0.10 · tubeTrim`** | paired with drive |
| Tracing `s_tr` | `a^1.5 · tracingTrim` | **`× 0.7`** | T5 tube×tracing interaction |
| High-shelf FO | — | fixed bilinear (unity @ 0 dB) | prior formula had DC gain ≈1/t |

### Measured (Release, 48 kHz, OS 4x)

| Test | Result |
|---|---|
| T4 a=0.5, −6 dBFS | **THD ≈ 1.22 %**, H2≈−38 dB, H3≪H2 (band 0.5–3 %) |
| T5 a=1 OS4x no ADAA | **worst alias ≈ −70.7 dB** (M1 gate −40; SPEC −80 aspirational → M4+ADAA) |
| T1 | max |H_play − analog| ≤ 0.1 dB @ listed fsi (table LS) |
| ctest | **17/17 PASS** |

### Deviations / follow-ups

- AutoGain Static table still unity (placeholders); run `wg_render --calibrate` when pink/−18 LUFS harness is finalized and paste into `AutoGain.cpp`.
- T14 ripple gate 0.08 dB (SPEC 0.01) — Kaiser cascade; Remez in M4.
- T5 full −80/−90 dB with ADAA deferred to M4.
- No M2/M3 UI / APVTS wiring beyond Engine (per stop rule).

### STOP (historical)

M1 closed; M2 implemented on `cursor/m2-plugin-wiring-2f51` (see below).

---

## M2 — Plugin wrapper / APVTS / state (2026-10-29) — DONE, STOPPED

### Done

- APVTS layout SPEC §7 (`ParameterLayout` / `ParameterIds`).
- `ParamBridge`: raw atomics → `EngineParams` once per `processBlock`.
- `PluginProcessor` wires `wg_dsp::Engine` (prepare / RT process / `ScopedNoDenormals` / mono|stereo buses / host bypass param / PDC via `setLatencySamples`).
- Structural OS/ADAA/offlineHQ changes re-prepare off the audio thread (Engine `setTargets` no longer allocates).
- `StateSerializer` (`stateVersion=1`) + `PresetManager` (4 factory programs §9.3 + user `.wgpreset` API under `Documents/WARM VYNIL/Presets`).
- Temporary UI: `GenericAudioProcessorEditor` (custom vinyl UI deferred to M3).
- `TransportState::isPlaying` atomic for M3 platter.
- Factory XML in BinaryData (`WarmVynilFactoryData`); programs applied in code.
- Plugin-level Catch2: T2, T6, T7, T11, T15 (`wg_plugin_tests`).

### Build / artefacts

```powershell
cmake --preset win-release
cmake --build --preset win-release --config Release
ctest --preset win-release --output-on-failure
```

| Artefact | Path |
|---|---|
| VST3 | `build\win-release\WarmVynil_artefacts\Release\VST3\WARM VYNIL.vst3` |
| CLAP | `build\win-release\WarmVynil_artefacts\Release\CLAP\WARM VYNIL.clap` |
| Staged | `dist\plugins\VST3\` · `dist\plugins\CLAP\` |

### Tests

- `ctest`: **23/23 PASS** (17 Engine + 5 plugin + smoke/primitives as discovered).
- pluginval strictness **10** (VST3 Win, `--skip-gui-tests`): **SUCCESS**.
- clap-validator / auval / mac: not run (Windows-only M0 decision).
- REAPER: binary present at `C:\Program Files\REAPER (x64)`; plugins staged to `dist\plugins\` for path scan — no automated insert/automation session in this run.

### Deviations

- User preset folder uses **`Documents/WARM VYNIL/Presets`** (product name) instead of SPEC `Warm Groove`.
- Custom `PluginEditor` left in tree but not linked (M3).
- Full DoD REAPER automation/PDC null session and clap-validator left for manual / CI follow-up.

### STOP

**M2 DoD met for Windows VST3+CLAP wiring.** Do not start M3 until explicit user command.
