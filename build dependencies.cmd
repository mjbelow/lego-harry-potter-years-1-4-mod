@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvars32.bat"

if not exist objects (
  mkdir objects
)

REM compile
cl /c /I include src\*.cpp /Foobjects/

if %errorlevel% neq 0 (
  pause
)