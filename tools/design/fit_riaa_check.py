import numpy as np
from scipy.optimize import least_squares

T1, T2, T3 = 3180e-6, 318e-6, 75e-6


def analog_db(f):
    w = 2 * np.pi * f
    num = np.sqrt(1 + (w * T2) ** 2)
    den = np.sqrt(1 + (w * T1) ** 2) * np.sqrt(1 + (w * T3) ** 2)
    return 20 * np.log10(num / den)


def biquad_db(b, a, f, fs):
    w = 2 * np.pi * f / fs
    z = np.exp(1j * w)
    z2 = z * z
    num = b[0] + b[1] / z + b[2] / z2
    den = 1 + a[0] / z + a[1] / z2
    return 20 * np.log10(np.maximum(np.abs(num / den), 1e-300))


def design(fs):
    p1 = np.exp(-1 / (T1 * fs))
    p3 = np.exp(-1 / (T3 * fs))
    zz = np.exp(-1 / (T2 * fs))
    a = np.array([-(p1 + p3), p1 * p3])
    f = np.logspace(np.log10(10), np.log10(min(0.49 * fs, 40000)), 600)
    tgt = analog_db(f) - analog_db(1000)
    w = np.where((f >= 20) & (f <= 20000), 1.0, 0.2)

    def pack(x):
        b = x.copy()
        g = 10 ** (-(biquad_db(b, a, np.array([1000.0]), fs)[0]) / 20)
        return b * g

    def resid(x):
        b = pack(x)
        return w * (biquad_db(b, a, f, fs) - tgt)

    # Free all 5 params
    x0 = np.array([1.0, -zz, 0.0, a[0], a[1]])

    def resid5(x):
        b = x[:3].copy()
        aa = x[3:]
        g = 10 ** (-(biquad_db(b, aa, np.array([1000.0]), fs)[0]) / 20)
        b *= g
        # soft pole penalty
        disc = aa[0] ** 2 - 4 * aa[1]
        if disc < 0:
            r = np.sqrt((-0.5 * aa[0]) ** 2 + (0.5 * np.sqrt(-disc)) ** 2)
        else:
            s = np.sqrt(disc)
            r = max(abs((-aa[0] + s) / 2), abs((-aa[0] - s) / 2))
        pen = max(0.0, r - (1 - 1e-6)) * 1e3
        return np.concatenate([w * (biquad_db(b, aa, f, fs) - tgt), [pen, pen]])

    sol = least_squares(resid5, x0, method="trf", max_nfev=5000)
    b = sol.x[:3]
    aa = sol.x[3:]
    g = 10 ** (-(biquad_db(b, aa, np.array([1000.0]), fs)[0]) / 20)
    b = b * g
    mask = (f >= 20) & (f <= 20000)
    err = np.max(np.abs((biquad_db(b, aa, f, fs) - biquad_db(b, aa, np.array([1000.0]), fs)) - tgt)[mask])
    return b, aa, err


for fs in [88200, 96000, 176400, 192000, 352800, 384000]:
    b, a, err = design(fs)
    print(f"fs={fs} err={err:.4f}")
    print(f"  b0={b[0]:.17g}, b1={b[1]:.17g}, b2={b[2]:.17g}, a1={a[0]:.17g}, a2={a[1]:.17g}")
