
#ifndef AppVersion
  #define AppVersion "0.12.0"
#endif

[Setup]
AppId=NetScope.GUI
AppName=NetScope
AppVersion={#AppVersion}
AppPublisher=NetScope
DefaultDirName={autopf}\NetScope
DefaultGroupName=NetScope
UninstallDisplayIcon={app}\netscope-gui.exe
OutputDir=..\dist\installer
OutputBaseFilename=NetScope-v{#AppVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "..\dist\NetScope-GUI\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\NetScope"; Filename: "{app}\netscope-gui.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\NetScope"; Filename: "{app}\netscope-gui.exe"; WorkingDir: "{app}"

[Run]
Filename: "{app}\netscope-gui.exe"; Description: "Launch NetScope"; Flags: nowait postinstall skipifsilent
