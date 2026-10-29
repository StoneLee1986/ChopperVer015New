@echo off
rem Build the audio preview tool (not part of the game project).
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 (
  echo [x] vcvars32.bat not found
  exit /b 1
)
cl /nologo /EHsc /std:c++17 /utf-8 /O2 /W3 /I"%CD%" tools\audio_preview.cpp Chiptune.cpp /Fe:tools\audio_preview.exe /Fo:tools\
if errorlevel 1 (
  echo [x] build failed
  exit /b 1
)
echo [ok] tools\audio_preview.exe
endlocal
