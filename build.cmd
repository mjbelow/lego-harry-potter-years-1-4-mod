@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

if not exist objects (
  mkdir objects
)

REM assemble
ml /Foobjects/ /c assembly.asm

if %errorlevel% neq 0 (
  pause
  exit
)

REM compile
cl /c /I include main.cpp src\*.cpp /Foobjects/ /link /libpath:"lib" user32.lib d3d9.lib MinHook.x86.lib

if %errorlevel% neq 0 (
  pause
  exit
)

if not exist dll (
  mkdir dll
)

cd dll

REM link objects into a DLL, which needs to be injected
link /DLL ../objects/*.obj /libpath:"../lib" MinHook.x86.lib /out:InjectedDll.dll

if %errorlevel% neq 0 (
  pause
) else (
  REM copy DLL's to game's directory
  copy "InjectedDll.dll" "D:\Program Files (x86)\WB Games\LEGO Harry Potter"
  copy "..\bin\MinHook.x86.dll" "D:\Program Files (x86)\WB Games\LEGO Harry Potter"
)