# Code signing (Windows) — M5

SPEC §12 requires Authenticode signing of the VST3 payload DLL, `.clap`, Standalone `.exe`, and the Inno Setup installer, with a trusted timestamp.

## Status for this tree

**Signing is SKIPPED by default** — no paid OV/EV certificate and no Azure Trusted Signing identity are configured in this repository or on the local build machine.

| Artefact | Signed? |
|---|---|
| `WARM VYNIL.vst3` (payload) | No |
| `WARM VYNIL.clap` | No |
| `WARM VYNIL.exe` (Standalone) | No |
| `WarmVynil-Setup-*.exe` | No |

Consequences:

- Windows **SmartScreen** may warn on first run of the installer (“Windows protected your PC” / unknown publisher). Users can proceed via **More info → Run anyway** (see [INSTALL.md](INSTALL.md)).
- Hosts do not require Authenticode for VST3/CLAP loading; unsigned plugins still load.

When a certificate is available, flip signing on without changing the Inno script — use `installer/windows/sign.ps1` (invoked from `package.ps1` and the release workflow).

## Enabling signing

### Option A — local / CI PFX (OV/EV)

GitHub Actions secrets (or local env):

| Secret / env | Purpose |
|---|---|
| `WG_SIGN_PFX_PATH` | Path to `.pfx` (CI: write secret bytes to a temp file first) |
| `WG_SIGN_PFX_PASSWORD` | PFX password |
| `WG_SIGN_TIMESTAMP_URL` | Optional; default `http://timestamp.digicert.com` |

Or install the cert into the machine store and set `WG_SIGN_CERT_THUMBPRINT`.

Requires `signtool.exe` from the Windows SDK.

### Option B — Azure Trusted Signing

Set `WG_AZURE_TRUSTED_SIGNING=1` and provide:

- `WG_AZURE_ENDPOINT`, `WG_AZURE_TENANT_ID`, `WG_AZURE_CLIENT_ID`, `WG_AZURE_CLIENT_SECRET`
- `WG_AZURE_CERTIFICATE_PROFILE` (and account name as required by your `azuresigntool` invocation)

`azuresigntool` must be on `PATH`.

### Release workflow

`.github/workflows/release.yml` builds on tag `v*`, packages with Inno Setup, and calls `sign.ps1`. If secrets are absent, the job **continues unsigned** and uploads the installer with a clear “unsigned” note in the release body.

**Never** commit `.pfx` / `.p12` / private keys (see `.gitignore`).
