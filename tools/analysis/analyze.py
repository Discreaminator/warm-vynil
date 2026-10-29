#!/usr/bin/env python3
"""Generate M1 analysis plots 1–7 into build/reports/ (SPEC §10.3)."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import soundfile as sf

ROOT = Path(__file__).resolve().parents[2]


def run_render(bin_path: Path, out: Path, **kwargs) -> None:
    cmd = [str(bin_path), "--out", str(out)]
    for k, v in kwargs.items():
        cmd += [f"--{k}", str(v)]
    subprocess.check_call(cmd)


def plot_riaa(reports: Path) -> None:
    # Analog vs bilinear-ish digital reference curve comparison (offline estimate)
    T1, T2, T3 = 3180e-6, 318e-6, 75e-6
    f = np.logspace(np.log10(20), np.log10(20000), 500)

    def analog_db(ff):
        w = 2 * np.pi * ff
        num = np.sqrt(1 + (w * T2) ** 2)
        den = np.sqrt(1 + (w * T1) ** 2) * np.sqrt(1 + (w * T3) ** 2)
        return 20 * np.log10(num / den)

    a = analog_db(f)
    a -= analog_db(np.array([1000.0]))[0]
    fig, ax = plt.subplots(2, 1, figsize=(8, 6), sharex=True)
    ax[0].semilogx(f, a, label="analog H_play")
    ax[0].set_ylabel("dB")
    ax[0].legend()
    ax[0].set_title("Plot 1 — RIAA play curve (analog reference)")
    ax[1].semilogx(f, np.zeros_like(f), label="fit error placeholder (see T1)")
    ax[1].set_xlabel("Hz")
    ax[1].set_ylabel("err dB")
    fig.tight_layout()
    fig.savefig(reports / "01_riaa_response.png", dpi=120)
    plt.close(fig)


def plot_chain_fr(render: Path, reports: Path) -> None:
    fig, ax = plt.subplots(figsize=(8, 4))
    for amount in (0, 25, 50, 75, 100):
        wav = reports / f"_tmp_fr_{amount}.wav"
        run_render(render, wav, signal="impulse", seconds=1, sr=48000, amount=amount, os=4, level=-20, mono=1)
        x, fs = sf.read(str(wav), always_2d=True)
        y = x[:, 0]
        n = len(y)
        spec = np.fft.rfft(y * np.hanning(n))
        f = np.fft.rfftfreq(n, 1 / fs)
        mag = 20 * np.log10(np.maximum(np.abs(spec), 1e-12))
        mag -= mag[np.argmin(np.abs(f - 1000))]
        ax.semilogx(f, mag, label=f"a={amount}%")
    ax.set_xlim(20, 20000)
    ax.set_ylim(-24, 12)
    ax.set_title("Plot 2 — Full chain FR vs Amount")
    ax.legend()
    fig.tight_layout()
    fig.savefig(reports / "02_chain_fr.png", dpi=120)
    plt.close(fig)


def plot_harmonics(render: Path, reports: Path) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    for ax, freq in zip(axes, (1000, 10000)):
        wav = reports / f"_tmp_h_{freq}.wav"
        run_render(render, wav, signal="sine", freq=freq, seconds=2, sr=48000, amount=50, os=4, level=-6, mono=1)
        x, fs = sf.read(str(wav), always_2d=True)
        y = x[int(0.5 * fs) :, 0]
        n = 65536
        y = y[:n]
        win = np.blackman(n)
        spec = np.abs(np.fft.rfft(y * win))
        f = np.fft.rfftfreq(n, 1 / fs)
        hs = []
        for k in range(1, 11):
            fk = freq * k
            if fk >= fs / 2:
                hs.append(0.0)
                continue
            b = int(round(fk * n / fs))
            hs.append(20 * np.log10(max(np.max(spec[max(0, b - 3) : b + 4]), 1e-12)))
        ax.bar(range(1, 11), np.array(hs) - hs[0])
        ax.set_title(f"Plot 3 — Harmonics @ {freq} Hz")
        ax.set_xlabel("k")
        ax.set_ylabel("dBc")
    fig.tight_layout()
    fig.savefig(reports / "03_harmonics.png", dpi=120)
    plt.close(fig)


def plot_thd_vs_level(render: Path, reports: Path) -> None:
    fig, ax = plt.subplots(figsize=(8, 4))
    levels = np.arange(-40, 1, 5)
    for amount in (25, 50, 100):
        thds = []
        for lv in levels:
            wav = reports / f"_tmp_thd_{amount}_{lv}.wav"
            run_render(render, wav, signal="sine", freq=1000, seconds=1.5, sr=48000, amount=amount, os=4, level=lv, mono=1)
            x, fs = sf.read(str(wav), always_2d=True)
            y = x[int(0.4 * fs) : int(0.4 * fs) + 32768, 0]
            n = len(y)
            spec = np.abs(np.fft.rfft(y * np.blackman(n)))
            f0 = 1000.0
            b1 = int(round(f0 * n / fs))
            h1 = np.max(spec[b1 - 3 : b1 + 4])
            acc = 0.0
            for k in range(2, 11):
                bk = int(round(k * f0 * n / fs))
                if bk >= len(spec):
                    break
                acc += np.max(spec[bk - 3 : bk + 4]) ** 2
            thds.append(100 * np.sqrt(acc) / max(h1, 1e-12))
        ax.semilogy(levels, thds, marker="o", label=f"a={amount}%")
    ax.set_title("Plot 4 — THD vs level")
    ax.set_xlabel("dBFS")
    ax.set_ylabel("THD %")
    ax.legend()
    fig.tight_layout()
    fig.savefig(reports / "04_thd_vs_level.png", dpi=120)
    plt.close(fig)


def plot_alias_spectrogram(render: Path, reports: Path) -> None:
    wav = reports / "_tmp_alias.wav"
    run_render(render, wav, signal="sweep", seconds=2, sr=48000, amount=100, os=4, level=-6, mono=1)
    x, fs = sf.read(str(wav), always_2d=True)
    y = x[:, 0]
    fig, ax = plt.subplots(figsize=(8, 4))
    ax.specgram(y, NFFT=2048, Fs=fs, noverlap=1024, cmap="magma")
    ax.set_ylim(0, 20000)
    ax.set_title("Plot 5 — Sweep spectrogram (aliasing)")
    fig.tight_layout()
    fig.savefig(reports / "05_alias_spectrogram.png", dpi=120)
    plt.close(fig)


def plot_transfer(reports: Path) -> None:
    u = np.linspace(-1.5, 1.5, 501)
    s = 1.0
    c2, c3 = 0.15 * s, -0.03 * s
    y = np.where(np.abs(u) <= 1, u + c2 * u**2 + c3 * u**3, np.sign(u) * (1 + c2 + c3) + (1 + 2 * c2 * np.sign(u) + 3 * c3) * (u - np.sign(u)))
    fig, ax = plt.subplots(figsize=(6, 4))
    ax.plot(u, y)
    ax.set_title("Plot 6 — Tracing transfer")
    ax.set_xlabel("u")
    ax.set_ylabel("f(u)")
    fig.tight_layout()
    fig.savefig(reports / "06_transfer_curves.png", dpi=120)
    plt.close(fig)


def plot_automation(render: Path, reports: Path) -> None:
    # Render fixed amount then show step response of output envelope via two renders
    wav0 = reports / "_tmp_auto0.wav"
    wav1 = reports / "_tmp_auto1.wav"
    run_render(render, wav0, signal="sine", freq=1000, seconds=1, sr=48000, amount=0, os=4, level=-12, mono=1)
    run_render(render, wav1, signal="sine", freq=1000, seconds=1, sr=48000, amount=100, os=4, level=-12, mono=1)
    x0, fs = sf.read(str(wav0), always_2d=True)
    x1, _ = sf.read(str(wav1), always_2d=True)
    fig, ax = plt.subplots(figsize=(8, 4))
    t = np.arange(len(x0)) / fs
    ax.plot(t, x0[:, 0], label="amount 0")
    ax.plot(t, x1[:, 0], label="amount 100", alpha=0.7)
    ax.set_title("Plot 7 — Amount 0 vs 100 waveforms")
    ax.legend()
    fig.tight_layout()
    fig.savefig(reports / "07_automation.png", dpi=120)
    plt.close(fig)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--render-bin", type=Path, required=True)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--reports", type=Path, default=ROOT / "build" / "reports")
    args = ap.parse_args()
    reports = args.reports
    reports.mkdir(parents=True, exist_ok=True)
    render = args.render_bin
    plot_riaa(reports)
    plot_chain_fr(render, reports)
    plot_harmonics(render, reports)
    plot_thd_vs_level(render, reports)
    plot_alias_spectrogram(render, reports)
    plot_transfer(reports)
    plot_automation(render, reports)
    print(f"Wrote plots to {reports}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
