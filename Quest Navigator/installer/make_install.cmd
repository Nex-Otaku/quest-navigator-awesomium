SET BINDIR=%~1
SET SCRIPTDIR=%~dp0
SET OUTDIR=%SCRIPTDIR%files
SET ADDDIR=%SCRIPTDIR%additional
SET ISSDIR=C:\Program Files (x86)\Inno Setup 5

DEL /Q /S "%OUTDIR%\*.*"

COPY "%BINDIR%*.exe" "%OUTDIR%"
COPY "%BINDIR%*.dll" "%OUTDIR%"
REM Копируем всё кроме .gitignore
ROBOCOPY "%ADDDIR%" "%OUTDIR%" /XF .gitignore

"%ISSDIR%\iscc" "%SCRIPTDIR%buildinstaller.iss"