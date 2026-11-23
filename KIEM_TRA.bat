@echo off
chcp 65001 >nul
call "%~dp0SU.bat" "id; getenforce; echo verifiedbootstate=$(getprop ro.boot.verifiedbootstate); echo flash.locked=$(getprop ro.boot.flash.locked); touch /data/local/tmp/coral_root_proof; sha256sum /dev/block/by-name/boot$(getprop ro.boot.slot_suffix)"
exit /b %ERRORLEVEL%
