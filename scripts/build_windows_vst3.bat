@echo off
setlocal
cd /d "%~dp0.."
where cmake >nul 2>nul || (echo ERROR: Install CMake 3.22+ and add it to PATH.& pause & exit /b 1)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (echo ERROR: Visual Studio 2022 with C++ tools was not found.& pause & exit /b 1)
for /f "usebackq tokens=*" %%V in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%V"
if not defined VSROOT (echo ERROR: Install Visual Studio 2022 Desktop development with C++.& pause & exit /b 1)
echo Configuring project; JUCE will be downloaded by CMake on first run.
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || goto :error
cmake --build build --config Release --target ClubAudioAnalyzerEQ_VST3 --parallel || goto :error
echo.
echo Build completed. Find the .vst3 bundle under build\ClubAudioAnalyzerEQ_artefacts\Release\VST3\
pause
exit /b 0
:error
echo.
echo Build failed. Review the error messages above.
pause
exit /b 1
