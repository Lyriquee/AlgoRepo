@echo off
setlocal enabledelayedexpansion

echo ===========================================
echo       AlgoRepo CLI - Auto Build Script
echo ===========================================

:: 检查 CMake 是否可用
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake is not installed or not added to PATH.
    echo Please install CMake first.
    exit /b 1
)

set BUILD_DIR=build
set CONFIG=Release

:: 检查是否传入了 clean 参数
if "%1"=="clean" (
    echo [INFO] Cleaning old build directory...
    if exist %BUILD_DIR% rmdir /s /q %BUILD_DIR%
)

:: 配置 CMake 项目
echo [INFO] Configuring project with CMake...
cmake -B %BUILD_DIR% -S . -DCMAKE_BUILD_TYPE=%CONFIG%
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: 执行编译
echo [INFO] Building project (%CONFIG%)...
cmake --build %BUILD_DIR% --config %CONFIG%
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [SUCCESS] Build completed successfully.
echo [INFO] Target binaries are in ./%BUILD_DIR%/%CONFIG%/ or ./%BUILD_DIR%/
echo.

pause
