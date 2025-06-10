@echo off
setlocal enabledelayedexpansion

:: Configuration --------------------------
set "BUILD_DIR=build"
set "GENERATOR=Unix Makefiles"
set "JOBS=128"
set "SCRIPT_DIR=%~dp0"

:: Check for CMakeLists.txt ----------------
if not exist "%SCRIPT_DIR%CMakeLists.txt" (
    echo [ERROR] CMakeLists.txt not found in root directory!
    exit /b 1
)

:: Argument parsing ------------------------
set "clean_mode=0"
set "fresh_mode=0"

:arg_loop
if "%~1" neq "" (
    if /i "%~1" == "clean" (
        set "clean_mode=1"
    ) else if /i "%~1" == "fresh" (
        set "fresh_mode=1"
    ) else if "%~1" == "-j" (
        shift
        set "JOBS=%~1"
    )
    shift
    goto arg_loop
)

:: Clean mode handling ---------------------
if %clean_mode% equ 1 (
    if exist "%BUILD_DIR%" (
        echo [INFO] Cleaning build...
        cd "%BUILD_DIR%"
        make clean || exit /b 1
        cd ..
        if %fresh_mode% equ 1 (
            echo [INFO] Removing build directory...
            rmdir /s /q "%BUILD_DIR%"
        )
    ) else (
        echo [INFO] Build directory does not exist, nothing to clean
    )
    exit /b 0
)

:: Build directory setup -------------------
if not exist "%BUILD_DIR%" (
    echo [INFO] Creating build directory...
    mkdir "%BUILD_DIR%" || (
        echo [ERROR] Failed to create build directory
        exit /b 1
    )
    echo [INFO] Generating build system...
    cd "%BUILD_DIR%"
    cmake .. -G "%GENERATOR%" || (
        echo [ERROR] CMake configuration failed
        cd ..
        rmdir /s /q "%BUILD_DIR%"
        exit /b 1
    )
    cd ..
)

:: Build execution -------------------------
cd "%BUILD_DIR%"
if %fresh_mode% equ 1 (
    echo [INFO] Re-generating build system...
    cmake .. -G "%GENERATOR%" || (
        echo [ERROR] CMake configuration failed
        exit /b 1
    )
)

echo [INFO] Starting build (Threads: %JOBS%)...
if "%GENERATOR%" == "Unix Makefiles" (
    make -j%JOBS% || exit /b 1
) else (
    cmake --build . --config Release -- /m:%JOBS% || exit /b 1
)

echo [SUCCESS] Build completed successfully
endlocal