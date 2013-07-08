SET SCRIPTDIR=%~dp0
"%SCRIPTDIR%awesomium_pak_utility" assets "%SCRIPTDIR%assets" "%SCRIPTDIR%..\additional"
COPY "%SCRIPTDIR%..\additional\assets.pak" "%SCRIPTDIR%..\..\build\bin"