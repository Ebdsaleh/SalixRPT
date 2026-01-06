@echo off
setlocal

:: 1. Define Project Names
set BUILD_DIR=build_release
set DEPLOY_DIR=Deploy
set DLL_NAME=reaper_salix_rpt-x64.dll

echo ========================================================
echo  SalixRPT Release Builder
echo ========================================================

:: 2. Clean previous builds to prevent "Stale DLL" issues
if exist %BUILD_DIR% (
    echo [INFO] Cleaning old build directory...
    rmdir /s /q %BUILD_DIR%
)
if exist %DEPLOY_DIR% (
    echo [INFO] Cleaning old deploy directory...
    rmdir /s /q %DEPLOY_DIR%
)

:: 3. Configure CMake (Generator: Visual Studio)
echo [INFO] Configuring Project...
cmake -S . -B %BUILD_DIR% 
if %errorlevel% neq 0 goto :error

:: 4. Build in RELEASE mode
echo [INFO] Building Release Configuration...
cmake --build %BUILD_DIR% --config Release
if %errorlevel% neq 0 goto :error

:: 5. Create Deploy Folder
mkdir %DEPLOY_DIR%

:: 6. Locate and Copy the DLL
:: (MSVC places the output in /bin/Release/ because of the --config flag)
echo [INFO] Copying Artifacts...

if exist "%BUILD_DIR%\bin\Release\%DLL_NAME%" (
    copy "%BUILD_DIR%\bin\Release\%DLL_NAME%" "%DEPLOY_DIR%\%DLL_NAME%"
) else (
    echo [ERROR] Could not find compiled DLL in expected path!
    goto :error
)

echo.
echo ========================================================
echo  SUCCESS! 
echo  Your file is ready in: %CD%\%DEPLOY_DIR%
echo ========================================================
goto :end

:error
echo.
echo [ERROR] Build Failed. Check the logs above.

:end
pause