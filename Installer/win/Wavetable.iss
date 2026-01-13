; Wavetable Installer Script for Inno Setup

#define MyAppName "Wavetable"
#define MyAppCompany "SocaLabs"
#define MyAppPublisher "SocaLabs"
#define MyAppURL "https://socalabs.com"
#define MyAppVersion GetEnv('VERSION')
#if MyAppVersion == ""
#define MyAppVersion "1.0.0"
#endif

[Setup]
AppID={{B8F4E3A2-9C7D-4B5E-8A1F-6D2E3C4B5A6F}
AppName={#MyAppCompany} {#MyAppName}
AppVerName={#MyAppCompany} {#MyAppName} {#MyAppVersion}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf64}\VstPlugins
DisableProgramGroupPage=yes
OutputDir=.\output
OutputBaseFilename=Wavetable_{#MyAppVersion}_Win
Compression=lzma/ultra
SolidCompression=true
ShowLanguageDialog=auto
InternalCompressLevel=ultra
MinVersion=6.1.7600
FlatComponentsList=false
AppendDefaultDirName=false
AlwaysShowDirOnReadyPage=yes
DirExistsWarning=no
DisableDirPage=no
DisableWelcomePage=False
DisableReadyPage=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoProductName={#MyAppCompany} {#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
UsePreviousGroup=False
Uninstallable=yes
UninstallDisplayName={#MyAppCompany} {#MyAppName}

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst2"; Description: "VST2 Plugin"; Types: full
Name: "vst3"; Description: "VST3 Plugin"; Types: full
Name: "data"; Description: "Wavetables and Presets"; Types: full; Flags: fixed

[Files]
; VST2 Plugin
Source: "bin\vst\Wavetable.dll"; DestDir: "{app}"; Components: vst2; Flags: ignoreversion

; VST3 Plugin
Source: "bin\vst3\Wavetable.vst3\*"; DestDir: "{commoncf64}\VST3\Wavetable.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs

; Wavetables
Source: "bin\Wavetables\*"; DestDir: "{commonappdata}\SocaLabs\Wavetable\Wavetables"; Components: data; Flags: ignoreversion recursesubdirs

; Presets
Source: "bin\Presets\*"; DestDir: "{commonappdata}\SocaLabs\Wavetable\Presets"; Components: data; Flags: ignoreversion recursesubdirs

[InstallDelete]
; Clean up old VST3 installation
Type: filesandordirs; Name: "{commoncf64}\VST3\Wavetable.vst3"
