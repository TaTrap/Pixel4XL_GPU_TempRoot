@echo off
setlocal DisableDelayedExpansion
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\su.ps1" %*
exit /b %ERRORLEVEL%
