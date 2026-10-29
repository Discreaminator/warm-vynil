# Single source of product identity (SPEC §0, §11.1)
# User-approved M0: plugin name Discreaminator; AGPL; VST3+CLAP; Windows-first.

set(WG_PLUGIN_NAME "Discreaminator")
set(WG_COMPANY_NAME "Vynil Audio")
set(WG_PLUGIN_DESCRIPTION "Vinyl cutting / cartridge / tube RIAA character (no surface noise)")
set(WG_PLUGIN_VERSION "0.1.0")
set(WG_BUNDLE_ID "com.vynilaudio.discreaminator")
set(WG_CLAP_ID "com.vynilaudio.discreaminator")

# 4-char codes (at least one uppercase). Documented for host IDs — do not change after release.
# Manufacturer: Vnyl  — VYNIL / Vynil Audio
# Plugin:       Dscr  — Discreaminator
set(WG_PLUGIN_MANUFACTURER_CODE "Vnyl")
set(WG_PLUGIN_CODE "Dscr")

set(WG_VST3_CATEGORIES "Fx Distortion")
set(WG_AU_MAIN_TYPE "kAudioUnitType_Effect")
