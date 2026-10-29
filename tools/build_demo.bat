@echo off
rem Build the chiptune preview tool (not part of the game project).
rem Usage: run from project root ->  tools\build_demo.bat
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 (
  echo [x] vcvars32.bat not found - check VS 2026 install path
  exit /b 1
)
cl /nologo /EHsc /std:c++17 /utf-8 /O2 /W3 /I"%CD%" tools\chiptune_demo.cpp Chiptune.cpp /Fe:tools\chiptune_demo.exe /Fo:tools\
if errorlevel 1 (
  echo [x] build failed
  exit /b 1
)
echo [ok] tools\chiptune_demo.exe
endlocal
