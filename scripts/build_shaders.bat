@echo off
setlocal enabledelayedexpansion

echo.
echo ====================================
echo  Compiling Shaders
echo ====================================
echo.

REM Save current directory
set SCRIPT_DIR=%cd%

REM Navigate to shaders directory (one level up from scripts, then into shaders)
cd ..\shaders
if %ERRORLEVEL% neq 0 (
    echo ERROR: Could not find shaders directory
    cd %SCRIPT_DIR%
    exit /b 1
)

REM Check if glslc exists
where glslc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: glslc not found. Please install Vulkan SDK.
    echo Download from: https://vulkan.lunarg.com/
    cd %SCRIPT_DIR%
    exit /b 1
)

REM Compile vertex shader
if exist textured_quad.vert (
    echo Compiling textured_quad.vert...
    glslc textured_quad.vert -o textured_quad.vert.spv
    if %ERRORLEVEL% neq 0 (
        echo ERROR: Failed to compile vertex shader
        cd %SCRIPT_DIR%
        exit /b 1
    )
    echo   - textured_quad.vert.spv created
) else (
    echo WARNING: textured_quad.vert not found, skipping
)

REM Compile fragment shader
if exist textured_quad.frag (
    echo Compiling textured_quad.frag...
    glslc textured_quad.frag -o textured_quad.frag.spv
    if %ERRORLEVEL% neq 0 (
        echo ERROR: Failed to compile fragment shader
        cd %SCRIPT_DIR%
        exit /b 1
    )
    echo   - textured_quad.frag.spv created
) else (
    echo WARNING: textured_quad.frag not found, skipping
)

echo.
echo Shaders compiled successfully!
echo.

REM Return to scripts directory
cd %SCRIPT_DIR%


