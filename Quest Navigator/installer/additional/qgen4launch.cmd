SET GAMEDIR=%~dp1
SET SCRIPTDIR=%~dp0

::Does string have a trailing slash? if so remove it 
IF %GAMEDIR:~-1%==\ SET GAMEDIR=%GAMEDIR:~0,-1%

"%SCRIPTDIR%QuestNavigator.exe" "%GAMEDIR%"