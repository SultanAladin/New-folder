@echo off
REM ================================================================
REM  CompileShaders.bat — Compile GLSL shaders to SPIR-V
REM ================================================================

setlocal enabledelayedexpansion

REM ---- Locate glslangValidator ----
set "GLSLANG="

REM Try VK_SDK_PATH first
if defined VK_SDK_PATH (
    if exist "%VK_SDK_PATH%\Bin\glslangValidator.exe" (
        set "GLSLANG=%VK_SDK_PATH%\Bin\glslangValidator.exe"
    )
)

REM Try VULKAN_SDK
if not defined GLSLANG (
    if defined VULKAN_SDK (
        if exist "%VULKAN_SDK%\Bin\glslangValidator.exe" (
            set "GLSLANG=%VULKAN_SDK%\Bin\glslangValidator.exe"
        )
    )
)

REM Try common install paths
if not defined GLSLANG (
    for %%D in (
        "C:\VulkanSDK"
    ) do (
        if exist %%D (
            for /f "delims=" %%V in ('dir /b /ad /o-n %%D 2^>nul') do (
                if exist "%%~D\%%V\Bin\glslangValidator.exe" (
                    set "GLSLANG=%%~D\%%V\Bin\glslangValidator.exe"
                    goto :found_glslang
                )
            )
        )
    )
)

:found_glslang
if not defined GLSLANG (
    echo ERROR: Cannot find glslangValidator.exe. Set VK_SDK_PATH or VULKAN_SDK.
    exit /b 1
)

echo Using: %GLSLANG%

REM ---- Set directories ----
set "SHADER_SRC=%~dp0Internal\ShaderSubsystem"
set "SHADER_OUT=%~dp0..\..\BuildArtifacts\Filament\Shaders"

if "%~1" neq "" set "SHADER_OUT=%~1"

if not exist "%SHADER_OUT%" mkdir "%SHADER_OUT%"

set "ERRORS=0"

REM ---- Compile vertex shaders ----
for %%F in ("%SHADER_SRC%\*.vert") do (
    echo Compiling: %%~nxF
    "%GLSLANG%" -V "%%F" -o "%SHADER_OUT%\%%~nxF.spv"
    if errorlevel 1 (
        echo   FAILED: %%~nxF
        set /a ERRORS+=1
    )
)

REM ---- Compile fragment shaders ----
for %%F in ("%SHADER_SRC%\*.frag") do (
    echo Compiling: %%~nxF
    "%GLSLANG%" -V "%%F" -o "%SHADER_OUT%\%%~nxF.spv"
    if errorlevel 1 (
        echo   FAILED: %%~nxF
        set /a ERRORS+=1
    )
)

REM ---- Compile compute shaders ----
for %%F in ("%SHADER_SRC%\*.comp") do (
    echo Compiling: %%~nxF
    "%GLSLANG%" -V "%%F" -o "%SHADER_OUT%\%%~nxF.spv"
    if errorlevel 1 (
        echo   FAILED: %%~nxF
        set /a ERRORS+=1
    )
)

if %ERRORS% gtr 0 (
    echo.
    echo *** %ERRORS% shader(s) failed to compile. ***
    exit /b 1
)

echo.
echo All shaders compiled successfully.
exit /b 0
