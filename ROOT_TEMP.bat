@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion
cd /d "%~dp0"

where adb >nul 2>&1 || (
    echo [LOI] Khong tim thay adb. Cai Android platform-tools va them adb vao PATH.
    pause
    exit /b 1
)
if not exist "bin\coral_root" (
    echo [LOI] Thieu bin\coral_root. Chay build.bat hoac khoi phuc binary cua release.
    pause
    exit /b 1
)

set "TARGET_SER="
for /f "tokens=1,2" %%A in ('adb devices') do (
    if "%%B"=="device" (
        for /f "delims=" %%D in ('adb -s %%A shell getprop ro.product.device 2^>nul') do (
            if /i "%%D"=="coral" set "TARGET_SER=%%A"
        )
    )
)
if not defined TARGET_SER (
    echo [LOI] Khong tim thay Pixel 4 XL ^(coral^) dang bat ADB.
    pause
    exit /b 1
)

set "BUILD_ID="
set "KERNEL_ID="
set "BOOT_STATE="
set "FLASH_LOCKED="
for /f "delims=" %%B in ('adb -s !TARGET_SER! shell getprop ro.build.display.id') do set "BUILD_ID=%%B"
for /f "delims=" %%K in ('adb -s !TARGET_SER! shell uname -r') do set "KERNEL_ID=%%K"
for /f "delims=" %%S in ('adb -s !TARGET_SER! shell getprop ro.boot.verifiedbootstate') do set "BOOT_STATE=%%S"
for /f "delims=" %%L in ('adb -s !TARGET_SER! shell getprop ro.boot.flash.locked') do set "FLASH_LOCKED=%%L"
if /i not "!BUILD_ID!"=="TP1A.221005.002.B2" goto :unsupported
if not "!KERNEL_ID!"=="4.14.276-g6ef255005cea-ab9062920" goto :unsupported
if /i not "!BOOT_STATE!"=="green" goto :unsupported
if not "!FLASH_LOCKED!"=="1" goto :unsupported

echo [*] Pixel 4 XL !TARGET_SER! - !BUILD_ID!
echo [*] Kernel !KERNEL_ID! - locked=!FLASH_LOCKED! - !BOOT_STATE!
echo [*] Khai thac KGSL tren RAM; khong mo bootloader, khong flash.
adb -s !TARGET_SER! push "bin\coral_root" /data/local/tmp/coral_root
if errorlevel 1 goto :failed
adb -s !TARGET_SER! shell "chmod 700 /data/local/tmp/coral_root; /data/local/tmp/coral_root 10 10"
if errorlevel 1 goto :failed

echo.
echo [*] Kiem chung bang shell that:
call "%~dp0KIEM_TRA.bat"
if errorlevel 1 goto :failed
adb -s !TARGET_SER! shell "ls -lZ /data/local/tmp/coral_root_proof"

echo.
echo [OK] Temp-root san sang. Dung SU.bat de chay lenh root.
echo     Hash active boot stock: dc076aa4a0f324592336572e87de070b1c0120596cbac76d5f6c2d40e28afcd4
echo     Reboot thuong se xoa temp-root.
pause
exit /b 0

:unsupported
echo.
echo [LOI] Thiet bi khong dung trang thai duoc ho tro.
echo       Can coral / TP1A.221005.002.B2 / 4.14.276-g6ef255005cea-ab9062920.
echo       Can bootloader locked=1 va verifiedbootstate=green.
echo       Hien tai: !BUILD_ID! / !KERNEL_ID! / locked=!FLASH_LOCKED! / !BOOT_STATE!
pause
exit /b 1

:failed
echo.
echo [LOI] Chuoi exploit hoac kiem chung that bai. Neu may van boot binh thuong, reboot roi chay lai.
pause
exit /b 1
