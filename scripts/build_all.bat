@echo off
echo Building shaders and application...
call build_shaders.bat
if %ERRORLEVEL% neq 0 exit /b 1

call build.bat
if %ERRORLEVEL% neq 0 exit /b 1

echo.
echo ====================================
echo  Build Complete!
echo ====================================