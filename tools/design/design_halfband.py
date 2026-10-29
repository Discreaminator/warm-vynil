#!/usr/bin/env python3
"""
Design linear-phase half-band FIR stages for WARM VYNIL 2× oversampling cascades.

Half-band is implemented via polyphase subfilter G: full length N = 2·len(G)−1,
h[2k] = G[k], odd taps zero, h[M] = 0.5 (M = (N−1)/2). Remez designs G on
[0, FP, FSB, 1] (SPEC edges relative to base fs on the G axis).

Generates src/dsp/os/HalfbandCoeffs.generated.h — polyphase half only:
  [ h(M), h(M+2), …, h(N−1) ].

SPEC §6.1 (fs_ref = 44.1 kHz):
  Stage 1 ~150–170 taps, ripple ≤ ±0.005 dB, stopband ≥ 110 dB from 0.5465·fs_in.
  Stages 2–3: same absolute pass/stop Hz, shorter filters.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from scipy import signal

FP_BASE = 0.4535
FSB_BASE = 0.5465
FS_REF_HZ = 44100.0

PASSBAND_RIPPLE_DB = 0.005
STOPBAND_DB = 110.0

STAGE1_LEN_MIN = 150
STAGE1_LEN_MAX = 170


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def valid_full_length(n: int) -> bool:
    return n % 2 == 1 and (n - 1) % 4 == 0


def expand_halfband(g: np.ndarray) -> np.ndarray:
    n = 2 * len(g) - 1
    h = np.zeros(n, dtype=np.float64)
    h[::2] = g
    h[(n - 1) // 2] = 0.5
    return h


def remez_g(num_g: int, stopband_weight: float) -> np.ndarray:
    return signal.remez(
        num_g,
        [0.0, FP_BASE, FSB_BASE, 1.0],
        [1.0, 0.0],
        weight=[1.0, stopband_weight],
        fs=2.0,
        maxiter=5000,
    )


def stopband_hi(fp_norm: float, fsb_norm: float) -> float:
    mirror = 1.0 - fp_norm
    if mirror > fsb_norm + 0.02:
        return mirror - 0.02
    return min(0.72, fsb_norm + 0.15)


def measure(h: np.ndarray, stage: int) -> dict:
    fp = FP_BASE
    fsb = FSB_BASE
    fs_in = FS_REF_HZ * (2 ** (stage - 1))

    w, hr = signal.freqz(h, worN=65536, fs=2.0)
    mag = np.abs(hr)
    mag_db = 20.0 * np.log10(np.maximum(mag, 1e-300))

    # Primary passband lobe: half-band magnitude flat at low w (audio band maps here per stage).
    lobe = w <= min(fp * 0.5, 0.12) + 1e-9
    pb_peak_db = float(np.max(mag_db[lobe]))
    ripple_db = float(np.max(mag_db[lobe]) - np.min(mag_db[lobe]))

    sb_hi = stopband_hi(fp, fsb)
    sb = (w >= fsb - 1e-9) & (w <= sb_hi + 1e-9)
    if np.any(sb):
        stopband_atten_db = float(pb_peak_db - np.max(mag_db[sb]))
    else:
        stopband_atten_db = float(pb_peak_db - mag_db[int(np.argmin(np.abs(w - fsb)))])

    meets = ripple_db <= PASSBAND_RIPPLE_DB + 1e-6 and stopband_atten_db >= STOPBAND_DB - 1e-6

    return {
        "stage": stage,
        "full_length": int(len(h)),
        "fs_in_hz": fs_in,
        "fp_hz": FP_BASE * FS_REF_HZ,
        "fsb_hz": FSB_BASE * FS_REF_HZ,
        "passband_ripple_db": ripple_db,
        "stopband_atten_db": stopband_atten_db,
        "dc_gain_db": float(mag_db[0]),
        "meets_spec": meets,
        "stopband_weight": None,
    }


def design_stage(
    stage: int,
    g_min: int,
    g_max: int,
    len_min: int,
    len_max: int,
) -> tuple[np.ndarray, dict]:
    best: tuple[np.ndarray, dict] | None = None

    for num_g in range(g_min, g_max + 1, 2):
        full = 2 * num_g - 1
        if not valid_full_length(full):
            continue
        if full < len_min or full > len_max:
            continue
        for ws in (1.0, 10.0, 50.0, 100.0, 200.0, 400.0, 800.0):
            try:
                g = remez_g(num_g, ws)
            except Exception:
                continue
            h = expand_halfband(g)
            meta = measure(h, stage)
            meta["stopband_weight"] = ws
            score = (
                0 if meta["meets_spec"] else 1,
                1 if meta["passband_ripple_db"] > PASSBAND_RIPPLE_DB else 0,
                meta["passband_ripple_db"],
                -meta["stopband_atten_db"],
                abs(full - (len_min + len_max) // 2),
            )
            if best is None or score < (
                0 if best[1]["meets_spec"] else 1,
                1 if best[1]["passband_ripple_db"] > PASSBAND_RIPPLE_DB else 0,
                best[1]["passband_ripple_db"],
                -best[1]["stopband_atten_db"],
                abs(best[1]["full_length"] - (len_min + len_max) // 2),
            ):
                best = (h, meta)

    if best is None:
        raise RuntimeError(f"no design for stage {stage}")

    return best


def polyphase_half(h: np.ndarray) -> np.ndarray:
    m = (len(h) - 1) // 2
    return h[m::2].astype(np.float64).copy()


def format_coeffs(values: np.ndarray) -> str:
    lines: list[str] = []
    row: list[str] = []
    for v in values:
        row.append(f"{v:.17g}")
        if len(row) == 4:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row))
    text = "\n".join(lines)
    if text.endswith(","):
        text = text[:-1]
    return text


def write_header(stages: list[tuple[np.ndarray, dict]], out_path: Path) -> None:
    parts = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstddef>",
        "",
        "namespace wg::dsp::os {",
        "",
        "// Auto-generated by tools/design/design_halfband.py — do not edit.",
        "// Polyphase half-band branch: kHb*Coeffs = { h(M), h(M+2), …, h(N−1) }, M=(N−1)/2.",
        "// Full impulse response: odd taps 0, even taps from coeffs (symmetric), h(M)=0.5.",
        "",
    ]
    for idx, (h, meta) in enumerate(stages, start=1):
        poly = polyphase_half(h)
        parts.append(
            f"// Stage {idx}: fs_in={meta['fs_in_hz']:.0f} Hz, N={meta['full_length']}, "
            f"ripple={meta['passband_ripple_db']:.4f} dB, stopband={meta['stopband_atten_db']:.2f} dB"
        )
        parts.append(f"inline constexpr std::size_t kHb{idx}FullLength = {meta['full_length']};")
        parts.append(f"inline constexpr std::size_t kHb{idx}Length = {len(poly)};")
        parts.append(f"inline constexpr std::array<double, kHb{idx}Length> kHb{idx}Coeffs = {{")
        parts.append(format_coeffs(poly))
        parts.append("};")
        parts.append("")
    parts.append("} // namespace wg::dsp::os")
    parts.append("")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text("\n".join(parts), encoding="utf-8")


def print_report(stages: list[tuple[np.ndarray, dict]]) -> None:
    print(
        f"Reference fs = {FS_REF_HZ} Hz, pass = {FP_BASE * FS_REF_HZ:.2f} Hz, "
        f"stop from {FSB_BASE * FS_REF_HZ:.2f} Hz"
    )
    print(f"Targets: ripple <= {PASSBAND_RIPPLE_DB} dB, stopband >= {STOPBAND_DB} dB")
    print()
    all_ok = True
    for h, meta in stages:
        poly = polyphase_half(h)
        ok = meta["meets_spec"]
        all_ok &= ok
        print(
            f"Stage {meta['stage']}: full={meta['full_length']} poly={len(poly)} "
            f"ripple={meta['passband_ripple_db']:.4f} dB "
            f"stopband={meta['stopband_atten_db']:.2f} dB "
            f"dc={meta['dc_gain_db']:.4f} dB "
            f"[{'OK' if ok else 'FAIL'}]"
        )
    print()
    if not all_ok:
        print("WARNING: one or more stages miss SPEC targets.", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=repo_root() / "src" / "dsp" / "os" / "HalfbandCoeffs.generated.h",
    )
    args = parser.parse_args()

    g1_min = (STAGE1_LEN_MIN + 1) // 2
    g1_max = (STAGE1_LEN_MAX + 1) // 2
    stage1 = design_stage(1, g1_min, g1_max, STAGE1_LEN_MIN, STAGE1_LEN_MAX)

    stage2 = design_stage(2, 23, 55, 45, 129)
    stage3 = design_stage(3, 23, 35, 45, 73)

    stages = [stage1, stage2, stage3]
    write_header(stages, args.output)
    print(f"Wrote {args.output}")
    print_report(stages)
    return 0 if all(m["meets_spec"] for _, m in stages) else 1


if __name__ == "__main__":
    raise SystemExit(main())
