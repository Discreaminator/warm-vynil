# Single source of product identity (SPEC §0, §11.1)
# User correction M0: display name WARM VYNIL; company Discreaminator; AGPL; VST3+CLAP; Windows-first.

set(WG_PLUGIN_NAME "WARM VYNIL")
set(WG_COMPANY_NAME "Discreaminator")
set(WG_PLUGIN_DESCRIPTION "Vinyl cutting / cartridge / tube RIAA character (no surface noise)")
set(WG_PLUGIN_VERSION "0.1.0")
set(WG_BUNDLE_ID "com.discreaminator.warmvynil")
set(WG_CLAP_ID "com.discreaminator.warmvynil")
set(WG_COPYRIGHT "Copyright (C) 2026 Discreaminator")

# 4-char codes (at least one uppercase). Host IDs — do not change after release.
# Replaces prior mistaken mapping Vnyl/Dscr (company/plugin were swapped).
# Manufacturer: Dscr  — Discreaminator (company)
# Plugin:       WVnl  — WARM VYNIL
set(WG_PLUGIN_MANUFACTURER_CODE "Dscr")
set(WG_PLUGIN_CODE "WVnl")

set(WG_VST3_CATEGORIES "Fx Distortion")
set(WG_AU_MAIN_TYPE "kAudioUnitType_Effect")
