; WARM VYNIL — Windows installer (SPEC §12 / M5)
; Compile with: ISCC.exe WarmGroove.iss
; Or: powershell -File package.ps1
;
; Expected staged payload (relative to this .iss / SourceDir):
;   stage\VST3\WARM VYNIL.vst3\...
;   stage\CLAP\WARM VYNIL.clap
;   stage\Standalone\WARM VYNIL.exe   (optional — omitted if absent)
;   stage\LICENSE, stage\NOTICE, stage\INSTALL.md

#ifndef WGAppVersion
  #define WGAppVersion "0.1.0"
#endif
#ifndef WGCompany
  #define WGCompany "Discreaminator"
#endif
#ifndef WGAppName
  #define WGAppName "WARM VYNIL"
#endif
#ifndef WGSourceDir
  #define WGSourceDir "stage"
#endif
#ifndef WGOutputDir
  #define WGOutputDir "..\..\dist\installer"
#endif
#ifndef WGOutputBase
  #define WGOutputBase "WarmVynil-Setup-" + WGAppVersion
#endif

#ifndef WGHasStandalone
  #define WGHasStandalone "0"
#endif

[Setup]
AppId={{A8F3C2E1-7B4D-4F9A-9C1E-2D6B8E0A4F31}
AppName={#WGAppName}
AppVersion={#WGAppVersion}
AppVerName={#WGAppName} {#WGAppVersion}
AppPublisher={#WGCompany}
AppPublisherURL=https://github.com/discreaminator/warm-vynil
AppSupportURL=https://github.com/discreaminator/warm-vynil
AppCopyright=Copyright (C) 2026 {#WGCompany}
DefaultDirName={autopf}\{#WGCompany}\{#WGAppName}
DefaultGroupName={#WGCompany}\{#WGAppName}
DisableProgramGroupPage=yes
LicenseFile={#WGSourceDir}\LICENSE
InfoBeforeFile={#WGSourceDir}\INSTALL.md
OutputDir={#WGOutputDir}
OutputBaseFilename={#WGOutputBase}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#WGAppName}
VersionInfoVersion={#WGAppVersion}.0
VersionInfoCompany={#WGCompany}
VersionInfoProductName={#WGAppName}
VersionInfoCopyright=Copyright (C) 2026 {#WGCompany}
; Code signing is applied externally (see sign.ps1). Leave SignTool empty when unsigned.
; Signed: ISCC /Ssigntool=... or post-sign the Setup.exe.

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation"
Name: "plugins"; Description: "Plugins only (VST3 + CLAP)"
Name: "custom"; Description: "Custom"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin"; Types: full plugins custom; Flags: fixed
Name: "clap"; Description: "CLAP plugin"; Types: full plugins custom
#if WGHasStandalone == "1"
Name: "standalone"; Description: "Standalone application"; Types: full custom
#endif
Name: "docs"; Description: "License and install notes"; Types: full plugins custom; Flags: fixed

[Files]
; VST3 bundle → Common Files\VST3
Source: "{#WGSourceDir}\VST3\WARM VYNIL.vst3\*"; DestDir: "{commoncf64}\VST3\WARM VYNIL.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3

; CLAP → Common Files\CLAP
Source: "{#WGSourceDir}\CLAP\WARM VYNIL.clap"; DestDir: "{commoncf64}\CLAP"; Flags: ignoreversion; Components: clap

#if WGHasStandalone == "1"
; Standalone → Program Files\<Company>\WARM VYNIL
Source: "{#WGSourceDir}\Standalone\WARM VYNIL.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
#endif

Source: "{#WGSourceDir}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion; Components: docs
Source: "{#WGSourceDir}\NOTICE"; DestDir: "{app}"; Flags: ignoreversion; Components: docs
Source: "{#WGSourceDir}\INSTALL.md"; DestDir: "{app}"; Flags: ignoreversion isreadme; Components: docs

[Icons]
#if WGHasStandalone == "1"
Name: "{group}\{#WGAppName}"; Filename: "{app}\WARM VYNIL.exe"; Components: standalone
#endif
Name: "{group}\Uninstall {#WGAppName}"; Filename: "{uninstallexe}"

[Run]
#if WGHasStandalone == "1"
Filename: "{app}\WARM VYNIL.exe"; Description: "Launch {#WGAppName}"; Flags: nowait postinstall skipifsilent unchecked; Components: standalone
#endif

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;
