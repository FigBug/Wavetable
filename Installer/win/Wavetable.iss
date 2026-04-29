; Wavetable installer (Inno Setup)

#define MyAppName "Wavetable"
#define MyAppCompany "SocaLabs"
#define MyAppPublisher "SocaLabs"
#define MyAppCopyright "2026 SocaLabs"
#define MyAppURL "https://socalabs.com/"
#define MyAppVersion GetStringFileInfo("bin\VST3\Wavetable.vst3\Contents\x86_64-win\Wavetable.vst3", "ProductVersion")
#define MyDefaultDirName "{commoncf64}\VST3"

[Setup]
AppID={{FB36B27A-9328-4438-8612-3B4E73021845}
AppName={#MyAppCompany} {#MyAppName} {#MyAppVersion}
AppVerName={#MyAppCompany} {#MyAppName} {#MyAppVersion}
AppVersion={#MyAppVersion}
AppCopyright={#MyAppCopyright}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={#MyDefaultDirName}
DisableProgramGroupPage=yes
OutputDir=.\bin
OutputBaseFilename=Wavetable
Compression=lzma/ultra
SolidCompression=true
ShowLanguageDialog=auto
LicenseFile=..\EULA.rtf
InternalCompressLevel=ultra
MinVersion=0,6.1.7600
FlatComponentsList=false
AppendDefaultDirName=false
AlwaysShowDirOnReadyPage=yes
DirExistsWarning=no
DisableDirPage=yes
DisableWelcomePage=no
DisableReadyPage=no
DisableReadyMemo=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoCopyright={#MyAppCopyright}
VersionInfoProductName={#MyAppCompany} {#MyAppName} {#MyAppVersion} (64-bit)
VersionInfoProductVersion={#MyAppVersion}
VersionInfoProductTextVersion={#MyAppVersion}
UsePreviousGroup=False
Uninstallable=no
PrivilegesRequired=admin

[Languages]
Name: english; MessagesFile: compiler:Default.isl

[Components]
Name: "vst";       Description: "VST plug-in";                    Types: full custom; Flags: checkablealone
Name: "vst3";      Description: "VST3 plug-in";                   Types: full custom; Flags: checkablealone
Name: "clap";      Description: "CLAP plug-in";                   Types: full custom; Flags: checkablealone
Name: "resources"; Description: "Factory wavetables and presets"; Types: full custom; Flags: fixed

[InstallDelete]
Type: files;          Name: "{commoncf64}\VST\Wavetable.dll";                    Components: vst
Type: filesandordirs; Name: "{commoncf64}\VST3\Wavetable.vst3";                  Components: vst3
Type: files;          Name: "{commoncf64}\CLAP\Wavetable.clap";                  Components: clap
Type: filesandordirs; Name: "{commonappdata}\SocaLabs\Wavetable\Presets";        Components: resources
Type: filesandordirs; Name: "{commonappdata}\SocaLabs\Wavetable\Wavetables";     Components: resources

[Files]
; Plug-in formats
Source: "bin\VST\Wavetable.dll";    DestDir: "{commoncf64}\VST";                    Flags: ignoreversion overwritereadonly; Components: vst
Source: "bin\VST3\Wavetable.vst3\*"; DestDir: "{commoncf64}\VST3\Wavetable.vst3\";  Flags: ignoreversion overwritereadonly recursesubdirs; Components: vst3
Source: "bin\CLAP\Wavetable.clap";   DestDir: "{commoncf64}\CLAP";                  Flags: ignoreversion overwritereadonly; Components: clap

; Factory content (Wavetables, Presets) → C:\ProgramData\SocaLabs\Wavetable\
Source: "..\..\plugin\Resources\Presets\*.xml";          DestDir: "{commonappdata}\SocaLabs\Wavetable\Presets\";    Flags: ignoreversion;                              Components: resources
Source: "..\..\plugin\Resources\WavetablesFLAC\*.wt2048"; DestDir: "{commonappdata}\SocaLabs\Wavetable\Wavetables\"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: resources
