@echo off

REM change code page so that you can copy to directory with unicode characters
chcp 65001

REM set path directories
set vcvars32="C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
set game_directory="D:\Program Files (x86)\WB Games\LEGO® Harry Potter™"

call %vcvars32%

if not exist objects (
  mkdir objects
)

REM assemble
ml /Foobjects/ /c src\assembly.asm

if %errorlevel% neq 0 (
  pause
  exit
)

REM compile
cl /c /I include src\main.cpp /Foobjects/

if %errorlevel% neq 0 (
  pause
  exit
)

if not exist dll (
  mkdir dll
)

cd dll

REM link objects into a DLL, which needs to be injected
link /DLL ../objects/*.obj "../lib/MinHook.x86.lib" /out:InjectedDll.dll

if %errorlevel% neq 0 (
  pause
) else (
  REM copy DLL's to game's directory
  copy "InjectedDll.dll" %game_directory%
  copy "..\bin\MinHook.x86.dll" %game_directory%
)