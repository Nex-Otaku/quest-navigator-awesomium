SET PAKDIR=%~1\
SET BINDIR=%~2
SET SCRIPTDIR=%~dp0
SET OUTDIR=%SCRIPTDIR%files
SET ISSDIR=C:\Program Files (x86)\Inno Setup 5

DEL /Q /S "%OUTDIR%\*.*"

COPY "%PAKDIR%*.pak" "%OUTDIR%"
COPY "%BINDIR%*.exe" "%OUTDIR%"
COPY "%BINDIR%*.dll" "%OUTDIR%"

"%ISSDIR%\iscc" "%SCRIPTDIR%buildinstaller.iss"