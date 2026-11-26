; Compiled by scripts/package-desktop.py, never from a developer build directory.
#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef Publisher
  #error Publisher is required
#endif
#ifndef StageDir
  #error StageDir is required
#endif
#ifndef AppIcon
  #error AppIcon is required
#endif

[Setup]
; Stable identity so future versions upgrade the same per-user installation.
AppId={{D749919D-FCEB-49D9-BA79-A922D60EDC54}
AppName=Selah
AppVersion={#AppVersion}
AppPublisher={#Publisher}
VersionInfoVersion={#AppVersion}.0
DefaultDirName={localappdata}\Programs\Selah
DefaultGroupName=Selah
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
SetupIconFile={#AppIcon}
UninstallDisplayIcon={app}\bin\Selah.exe
UninstallDisplayName=Selah
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
AppMutex=Selah.Desktop.Running
SetupMutex=Selah.Desktop.Setup
; Do not automatically terminate a service or relaunch into live output.
CloseApplications=no
RestartApplications=no
#ifdef SignedRelease
SignTool=SelahRelease
SignedUninstaller=yes
#else
SignedUninstaller=no
#endif

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{userprograms}\Selah"; Filename: "{app}\bin\Selah.exe"; WorkingDir: "{app}\bin"
Name: "{userdesktop}\Selah"; Filename: "{app}\bin\Selah.exe"; WorkingDir: "{app}\bin"; Tasks: desktopicon

; Deliberately no Run, UninstallDelete or profile cleanup entries. Libraries and
; preferences live outside {app} and survive upgrade/uninstall.
