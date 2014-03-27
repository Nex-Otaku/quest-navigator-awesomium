; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Quest Navigator"
#define MyAppVersion "0.0.17-test"
#define MyAppPublisher "QSP"
#define MyAppURL "http://qsp.su"
#define MyAppExeName "QuestNavigator.exe"
#define QnCacheFolder "Quest Navigator Cache"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{C4B77AB6-A47B-45A1-AE39-DB7391E462C2}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\QSP\Quest Navigator
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=D:\dev\repos\quest-navigator-awesomium\Quest Navigator\installer\release
OutputBaseFilename=QuestNavigator-setup-{#MyAppVersion}
SetupIconFile=D:\dev\QSP\qsp_svn\players\classic\qspgui\misc\icons\logo.ico
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1
Name: associate; Description: "��������� � ���������� ����� "".qsp"""; GroupDescription: "�������� ����������:"

[Files]
Source: "D:\dev\repos\quest-navigator-awesomium\Quest Navigator\installer\files\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\dev\repos\quest-navigator-awesomium\Quest Navigator\installer\files\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\awesomium.log"
; ����� "Program Files(x86)\QSP\QuestNavigator" � "Program Files(x86)\QSP"
Type: dirifempty; Name: "{app}"
Type: dirifempty; Name: "{app}\.."
; ����� � ����� ���
Type: filesandordirs; Name: "{userappdata}\{#QnCacheFolder}"

[Registry]
Root: HKCU; Subkey: "Software\{#MyAppPublisher}\{#MyAppName}\WinSparkle"; Flags: uninsdeletekey
; �������� ������ � ����������� .qsp
Root: HKCR; Subkey: ".qsp"; ValueType: string; ValueName: ""; ValueData: "QspGameFile"; Flags: uninsdeletevalue; Tasks: associate
Root: HKCR; Subkey: "qsp_auto_file"; ValueType: none; ValueName: ""; ValueData: ""; Flags: deletekey; Tasks: associate
Root: HKCR; Subkey: "QspGameFile"; ValueType: string; ValueName: ""; ValueData: "���� ���� QSP"; Flags: uninsdeletekey; Tasks: associate
Root: HKCR; Subkey: "QspGameFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: associate
Root: HKCR; Subkey: "QspGameFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: associate