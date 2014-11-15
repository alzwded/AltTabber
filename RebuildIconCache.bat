@echo off

IF #%1# == #-y# GOTO :ANYWAY
IF #%1# == #/y# GOTO :ANYWAY
IF #%1# == #/?# GOTO :HELP
IF #%1# == #-?# GOTO :HELP
IF #%1# == #-h# GOTO :HELP
choice /m "This will restart explorer. Continue?"
IF ERRORLEVEL 2 GOTO :EOF

:ANYWAY
ie4uinit -ClearIconCache
DEL "%localappdata%\IconCache.db"
taskkill /f /im explorer.exe
REM wait 5 seconds
ping 127.0.0.1 -n 5 -w %1000 > nul
explorer
GOTO :EOF

:HELP
echo "Force rebuilds the icon cache. This script restarts explorer"
echo "/y    does not ask for confirmation"
echo "/?    shows this message"
GOTO :EOF
