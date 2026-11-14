@echo off
setlocal
set "plugin=%CommonProgramFiles%\VST3\LAssassin-76.vst3\Contents\x86_64-win\LAssassin-76.vst3"
set "report=%TEMP%\LAssassin-diagnostic.txt"
"%~dp0LAssassinProbe.exe" "%plugin%" > "%report%" 2>&1
set "probeResult=%ERRORLEVEL%"
echo Probe exit code: %probeResult% >> "%report%"
echo. >> "%report%"
echo --- Ableton log entries mentioning this plugin --- >> "%report%"
powershell -NoProfile -Command "Get-ChildItem ($env:APPDATA+'\Ableton\Live*\Preferences\Log.txt') -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1 | Get-Content | Select-String 'LAssassin' -Context 2,2" >> "%report%" 2>&1
type "%report%"
echo.
echo Report saved to: %report%
pause
