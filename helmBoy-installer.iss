#define MyAppVersion "1.1.2"

[Setup]
AppName=HelmBoy Synthesizer
AppVersion={#MyAppVersion}
AppPublisher=Marc Scheffer
AppPublisherURL=https://github.com/Hans45/HelmBoy
AppSupportURL=https://github.com/Hans45/HelmBoy/issues
AppUpdatesURL=https://github.com/Hans45/HelmBoy/releases
DefaultDirName={autopf}\HelmBoy
DefaultGroupName=HelmBoy Synthesizer
AllowNoIcons=yes
LicenseFile=COPYING
InfoBeforeFile=README.md
ShowLanguageDialog=yes
DisableWelcomePage=no
UninstallDisplayIcon={app}\HelmBoy.exe
OutputDir=installer
OutputBaseFilename=helmBoy-setup-{#MyAppVersion}
SetupIconFile=images\inno setup graphics\helmBoy.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardImageFile=images\inno setup graphics\helmBoy_wizard.png
WizardImageFileDynamicDark=images\inno setup graphics\helmBoy_wizard.png
WizardStyle=modern dynamic windows11
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Complete Installation"
Name: "factoryOnly"; Description: "Factory Patches Only Installation"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone Application"; Types: full custom factoryOnly
Name: "vst3"; Description: "VST3 Plugin"; Types: full custom factoryOnly
Name: "lv2"; Description: "LV2 Plugin"; Types: full custom factoryOnly
Name: "patches"; Description: "Patches Banks"; Types: full custom factoryOnly
Name: "Patches\Factory"; Description: "Factory Patches"; Types: full custom factoryOnly
Name: "Patches\Supplementary"; Description: "Supplementary Patches"; Types: full custom
Name: "DevicesProfiles"; Description: "Devices Profiles"; Types: full custom factoryOnly
[Dirs]
Name: "{userdocs}\HelmBoy"; Flags: uninsneveruninstall; Check: not IsAdminInstallMode
Name: "{userdocs}\HelmBoy\Patches"; Flags: uninsneveruninstall; Check: not IsAdminInstallMode
Name: "{userdocs}\HelmBoy\Devices Profiles"; Flags: uninsneveruninstall; Check: not IsAdminInstallMode
Name: "{commondocs}\HelmBoy\Patches"; Flags: uninsneveruninstall
Name: "{commondocs}\HelmBoy\Devices Profiles"; Flags: uninsneveruninstall

[Files]
; Application standalone (autonome - aucun DLL externe requis)
Source: "build\HelmBoyStandalone_artefacts\Release\HelmBoy.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

; Plugin VST3
Source: "build\HelmBoyPlugin_artefacts\Release\VST3\HelmBoy.vst3\*"; DestDir: "{commoncf}\VST3\Marc Scheffer\HelmBoy.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
; Plugin LV2
Source: "build\HelmBoyPlugin_artefacts\Release\LV2\HelmBoy.lv2\*"; DestDir: "{commoncf}\LV2\Marc Scheffer"; Components: lv2; Flags: ignoreversion recursesubdirs createallsubdirs

; Documentation et licence
Source: "README.md"; DestDir: "{commondocs}\HelmBoy"; Flags: ignoreversion; Check: IsAdminInstallMode
Source: "COPYING"; DestDir: "{commondocs}\HelmBoy"; Flags: ignoreversion; Check: IsAdminInstallMode
Source: "README.md"; DestDir: "{userdocs}\HelmBoy"; Flags: ignoreversion; Check: not IsAdminInstallMode
Source: "COPYING"; DestDir: "{userdocs}\HelmBoy"; Flags: ignoreversion; Check: not IsAdminInstallMode

; Factory Presets (repertoire public pour les plugins)
Source: "patches\HelmBoy Factory Presets\*"; DestDir: "{commondocs}\HelmBoy\Patches\HelmBoy Factory Presets"; Components: Patches\Factory; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "patches\HelmBoy Factory Presets\*"; DestDir: "{userdocs}\HelmBoy\Patches\HelmBoy Factory Presets"; Components: Patches\Factory; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode

Source: "patches\Historical Factory Presets\*"; DestDir: "{commondocs}\HelmBoy\Patches\Historical Factory Presets"; Components: Patches\Factory; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "patches\Historical Factory Presets\*"; DestDir: "{userdocs}\HelmBoy\Patches\Historical Factory Presets"; Components: Patches\Factory; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode

Source: "patches\Generated\*"; DestDir: "{commondocs}\HelmBoy\Patches\Generated"; Components: Patches\Factory; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "patches\Generated\*"; DestDir: "{userdocs}\HelmBoy\Patches\Generated"; Components: Patches\Factory; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode

Source: "patches\Databroth\*"; DestDir: "{commondocs}\HelmBoy\Patches\Databroth"; Components: Patches\Supplementary; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "patches\Databroth\*"; DestDir: "{userdocs}\HelmBoy\Patches\Databroth"; Components: Patches\Supplementary; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode

Source: "patches\Sonic Sirius\*"; DestDir: "{commondocs}\HelmBoy\Patches\Sonic Sirius"; Components: Patches\Supplementary; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "patches\Sonic Sirius\*"; DestDir: "{userdocs}\HelmBoy\Patches\Sonic Sirius"; Components: Patches\Supplementary; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode

Source: "patches\TSL\*"; DestDir: "{commondocs}\HelmBoy\Patches\TSL"; Components: Patches\Supplementary; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "patches\TSL\*"; DestDir: "{userdocs}\HelmBoy\Patches\TSL"; Components: Patches\Supplementary; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode

Source: "Devices Profiles\*"; DestDir: "{commondocs}\HelmBoy\Devices Profiles"; Components: DevicesProfiles; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsAdminInstallMode
Source: "Devices Profiles\*"; DestDir: "{userdocs}\HelmBoy\Devices Profiles"; Components: DevicesProfiles; Flags: ignoreversion recursesubdirs createallsubdirs; Check: not IsAdminInstallMode


[Icons]
Name: "{group}\HelmBoy Synthesizer"; Filename: "{app}\HelmBoy.exe"; Components: standalone
Name: "{group}\HelmBoy Readme"; Filename: "{commondocs}\HelmBoy\README.md"
Name: "{group}\HelmBoy License"; Filename: "{commondocs}\HelmBoy\COPYING"
Name: "{group}\{cm:UninstallProgram,HelmBoy}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\HelmBoy Synthesizer"; Filename: "{app}\HelmBoy.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; Components: standalone

[Run]
Filename: "{app}\HelmBoy.exe"; Description: "{cm:LaunchProgram,HelmBoy Synthesizer}"; Flags: nowait postinstall skipifsilent; Components: standalone