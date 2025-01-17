@echo off
setlocal
set DIR=%~dp0
powershell -Command "Start-Process '%DIR%dcomjarabel.exe' -Verb RunAs"
endlocal
