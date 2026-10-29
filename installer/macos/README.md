# macOS packaging (deferred)

SPEC §12 describes `codesign` → `pkgbuild` / `productbuild` → notarization (`notarytool`) → stapler for VST3 / AU / CLAP / Standalone.

**Status:** skipped for M5. Product decision is **Windows-first**; macOS packaging will be added when a Developer ID + App Store Connect API key are available and macOS builds are enabled in CI.

Placeholder files to add later:

- `build_pkg.sh`
- `distribution.xml`
- `entitlements.plist`
