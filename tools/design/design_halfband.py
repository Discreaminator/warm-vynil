#!/usr/bin/env python3
"""
Design linear-phase half-band FIR stages for WARM VYNIL 2x oversampling.

Half-band (cutoff F/4): centre h(M)=0.5; even offsets from centre are 0;
odd offsets carry the taps. Filter runs at F=2*fs_in after zero-stuff.

SPEC §6.1:
  Stage 1: pass ≤ 0.4535·fs_in (≈20 kHz @ 44.1), stop ≥ 0.5465·fs_in, ≥110 dB.
  Stages 2–3: same *absolute* passband (~20 kHz); wider relative transition → shorter N.
  Prefer Remez (equiripple); Kaiser fallback.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from scipy import signal

FS_REF = 44100.0
PASS_HZ = 0.4535 * FS_REF  # ≈ 20.0 kHz absolute
TARGET_ATTEN = 110.0
TARGET_RIPPLE = 0.005


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def valid_n(n: int) -> bool:
    return n % 2 == 1 and (n - 1) % 4 == 0


def kaiser_beta(atten_db: float) -> float:
    a = float(atten_db)
    if a > 50.0:
        return 0.1102 * (a - 8.7)
    if a >= 21.0:
        return 0.5842 * (a - 21.0) ** 0.4 + 0.07886 * (a - 21.0)
    return 0.0


def force_halfband(h: np.ndarray) -> np.ndarray:
    n = len(h)
    mid = (n - 1) // 2
    out = np.array(h, dtype=np.float64, copy=True)
    for i in range(n):
        if i != mid and (i - mid) % 2 == 0:
            out[i] = 0.0
    out[mid] = 0.5
    others = float(np.sum(out) - 0.5)
    if abs(others) > 1e-18:
        s = 0.5 / others
        for i in range(n):
            if (i - mid) % 2 != 0:
                out[i] *= s
    out[mid] = 0.5
    return out


def design_kaiser(n: int, atten_db: float = 120.0) -> np.ndarray:
    assert valid_n(n)
    beta = kaiser_beta(atten_db)
    h = signal.firwin(n, cutoff=0.5, window=("kaiser", beta), pass_zero="lowpass", fs=2.0)
    return force_halfband(h)


def design_remez(n: int, fs_in: float) -> np.ndarray | None:
    """Equiripple half-band with absolute audio pass / image stop edges."""
    assert valid_n(n)
    F = 2.0 * fs_in
    # Fractions of F (freqz/remez fs=1 → Nyquist=0.5).
    pass_frac = PASS_HZ / F
    stop_frac = (fs_in - PASS_HZ) / F
    # Keep a valid transition around the half-band centre (0.25).
    pass_frac = min(pass_frac, 0.24)
    stop_frac = max(stop_frac, 0.26)
    if stop_frac <= pass_frac + 1e-4:
        return None
    try:
        # numtaps for remez is filter length; bands in [0, 0.5] with fs=1.
        h = signal.remez(
            n,
            [0.0, pass_frac, stop_frac, 0.5],
            [1.0, 0.0],
            fs=1.0,
            maxiter=100,
        )
    except ValueError:
        return None
    return force_halfband(np.asarray(h, dtype=np.float64))


def measure(h: np.ndarray, stage: int, fs_in: float) -> dict:
    F = 2.0 * fs_in
    w, hr = signal.freqz(h, worN=262144, fs=F)
    mag_db = 20.0 * np.log10(np.maximum(np.abs(hr), 1e-300))
    pb0 = float(mag_db[0])
    pb = mag_db[w <= PASS_HZ]
    stop_hz = fs_in - PASS_HZ
    sb = mag_db[(w >= stop_hz) & (w <= F * 0.5)]
    ripple = float(np.max(np.abs(pb - pb0))) if len(pb) else 999.0
    stop = float(pb0 - np.max(sb)) if len(sb) else 0.0
    return {
        "stage": stage,
        "n": len(h),
        "ripple_db": ripple,
        "stop_db": stop,
        "fs_in": fs_in,
        "dc_db": pb0,
    }


def length_candidates(stage: int) -> list[int]:
    # Absolute audio band fixed (~20 kHz); higher stages → wider relative transition.
    # Stage 1 starts longer: short Remez equiripple stacks poorly in 4x/8x cascade (T14).
    if stage == 1:
        return list(range(193, 401, 4))
    if stage == 2:
        return list(range(49, 201, 4))
    return list(range(29, 129, 4))


def find_length(stage: int) -> tuple[np.ndarray, dict, str]:
    fs_in = FS_REF * (2 ** (stage - 1))
    lengths = length_candidates(stage)

    best_h = None
    best_m = None
    best_method = "kaiser"
    # Stage 1: Kaiser first (smoother cascade passband). Stages 2–3: Remez OK.
    prefer_remez = stage > 1

    for n in lengths:
        if not valid_n(n):
            continue

        if prefer_remez:
            h_r = design_remez(n, fs_in)
            if h_r is not None:
                m_r = measure(h_r, stage, fs_in)
                if best_m is None or (m_r["stop_db"], -m_r["ripple_db"]) > (
                    best_m["stop_db"],
                    -best_m["ripple_db"],
                ):
                    best_h, best_m, best_method = h_r, m_r, "remez"
                if m_r["stop_db"] >= TARGET_ATTEN and m_r["ripple_db"] <= TARGET_RIPPLE:
                    return h_r, m_r, "remez"

        for atten in (115.0, 120.0, 130.0, 140.0):
            h = design_kaiser(n, atten)
            m = measure(h, stage, fs_in)
            better = best_m is None or (m["stop_db"], -m["ripple_db"]) > (
                best_m["stop_db"],
                -best_m["ripple_db"],
            )
            if better:
                best_h, best_m, best_method = h, m, "kaiser"
            if m["stop_db"] >= TARGET_ATTEN and m["ripple_db"] <= TARGET_RIPPLE:
                return h, m, "kaiser"

        if not prefer_remez:
            h_r = design_remez(n, fs_in)
            if h_r is not None:
                m_r = measure(h_r, stage, fs_in)
                if best_m is None or (m_r["stop_db"], -m_r["ripple_db"]) > (
                    best_m["stop_db"],
                    -best_m["ripple_db"],
                ):
                    best_h, best_m, best_method = h_r, m_r, "remez"
                if m_r["stop_db"] >= TARGET_ATTEN and m_r["ripple_db"] <= TARGET_RIPPLE:
                    return h_r, m_r, "remez"

    assert best_h is not None and best_m is not None
    return best_h, best_m, best_method


def polyphase_half(h: np.ndarray) -> np.ndarray:
    """{ h(M), h(M+1), h(M+3), ..., h(N-1) }."""
    n = len(h)
    mid = (n - 1) // 2
    vals = [h[mid]]
    i = mid + 1
    while i < n:
        vals.append(h[i])
        i += 2
    return np.asarray(vals, dtype=np.float64)


def fmt_array(name: str, arr: np.ndarray, full_n: int, m: dict, method: str) -> str:
    lines = [
        f"// Stage {m['stage']}: fs_in={m['fs_in']:.0f} Hz, N={full_n}, "
        f"ripple={m['ripple_db']:.4f} dB, stopband={m['stop_db']:.2f} dB ({method})",
        f"inline constexpr std::size_t kHb{m['stage']}FullLength = {full_n};",
        f"inline constexpr std::size_t kHb{m['stage']}Length = {len(arr)};",
        f"inline constexpr std::array<double, kHb{m['stage']}Length> {name} = {{",
    ]
    row: list[str] = []
    for i, v in enumerate(arr):
        row.append(f"{v:.17g}")
        if len(row) == 4 or i == len(arr) - 1:
            lines.append("    " + ", ".join(row) + ("," if i < len(arr) - 1 else ""))
            row = []
    lines.append("};")
    return "\n".join(lines)


def write_header(stages: list[tuple[np.ndarray, dict, str]], path: Path) -> None:
    parts = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstddef>",
        "",
        "namespace wg::dsp::os {",
        "",
        "// Auto-generated by tools/design/design_halfband.py — do not edit.",
        "// Polyphase half-band: kHb*Coeffs = { h(M), h(M+1), h(M+3), ..., h(N-1) }.",
        "// Full impulse: even offsets from centre are 0; centre = 0.5; odd taps symmetric.",
        "// Stages 2–3 sized for absolute ~20 kHz passband (SPEC §6.1 / M4).",
        "",
    ]
    for h, m, method in stages:
        ph = polyphase_half(h)
        parts.append(fmt_array(f"kHb{m['stage']}Coeffs", ph, len(h), m, method))
        parts.append("")
    parts.append("} // namespace wg::dsp::os")
    parts.append("")
    path.write_text("\n".join(parts), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=None)
    args = parser.parse_args()
    out = args.out or (repo_root() / "src" / "dsp" / "os" / "HalfbandCoeffs.generated.h")

    stages = []
    for s in (1, 2, 3):
        h, m, method = find_length(s)
        ok = m["stop_db"] >= TARGET_ATTEN and m["ripple_db"] <= TARGET_RIPPLE
        print(
            f"Stage {s}: N={m['n']}  ripple={m['ripple_db']:.4f} dB  "
            f"stop={m['stop_db']:.2f} dB  method={method}  "
            f"[{'OK' if ok else 'BEST-EFFORT'}]"
        )
        stages.append((h, m, method))

    write_header(stages, out)
    print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
