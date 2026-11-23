@echo off
setlocal EnableExtensions
cd /d "%~dp0"

if not defined NDK_CLANG if defined ANDROID_NDK_HOME set "NDK_CLANG=%ANDROID_NDK_HOME%\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe"
if not defined NDK_CLANG (
    echo [LOI] Dat NDK_CLANG thanh duong dan toi Android NDK clang.exe hoac dat ANDROID_NDK_HOME.
    exit /b 1
)
if not exist "%NDK_CLANG%" (
    echo [LOI] Khong tim thay "%NDK_CLANG%".
    exit /b 1
)
if not exist "bin" mkdir bin

"%NDK_CLANG%" --target=aarch64-linux-android29 -static -O2 -Wall -Wextra -Werror -pthread -o "bin\coral_root" "src\coral_kgsl_root.c"
if errorlevel 1 exit /b 1

echo [OK] Da build bin\coral_root
